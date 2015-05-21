#include "coro/coro.hpp"

#include <stdexcept>
#include <sys/eventfd.h>
#include <valgrind/valgrind.h>
#include <iostream>
#include <unistd.h>

#include "sync/event.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"

namespace indecorous {

uint32_t dispatcher_t::s_max_swaps_per_loop = 100;

__thread dispatcher_t* dispatcher_t::s_instance = nullptr;

// Used to hand over parameters to a new coroutine - since we can't safely pass pointers
struct handover_params_t {
    void init(coro_t *_self,
              void(coro_t::*_fn)(void*, coro_t*, bool),
              void *_params,
              coro_t *_parent,
              bool _immediate) {
        self = _self;
        fn = _fn;
        params = _params;
        parent = _parent;
        immediate = _immediate;
    }

    void clear() {
        self = nullptr;
        fn = nullptr;
        params = nullptr;
        parent = nullptr;
        immediate = false;
    }

    void check_valid() {
        assert(self != nullptr);
        assert(fn != nullptr);
        assert(params != nullptr);
        assert(parent != nullptr);
    }

    coro_t *self;
    void (coro_t::*fn)(void*, coro_t*, bool);
    void *params;
    coro_t *parent;
    bool immediate;
};
__thread handover_params_t handover_params;

dispatcher_t::dispatcher_t() :
    m_self(nullptr),
    m_active_contexts(), // TODO: init this to the number of queued coros
    m_context_arena(256)
{
    assert(s_instance == nullptr);
    s_instance = this;
}

dispatcher_t::~dispatcher_t() {
    assert(m_self == nullptr);
    assert(s_instance == this);
    s_instance = nullptr;
}

dispatcher_t& dispatcher_t::get_instance() {
    if (s_instance == nullptr)
        throw coro_exc_t("dispatcher_t instance not found");
    return *s_instance;
}

uint32_t dispatcher_t::run() {
    assert(s_instance == this);
    assert(m_self == nullptr);
    m_swap_count = 0;

    if (!m_run_queue.empty()) {
        // Save the currently running context
        int res = getcontext(&m_parentContext);
        assert(res == 0);

        // Start the queue of coroutines - they will swap in the next until no more are left
        m_self = m_run_queue.pop_front();
        assert(m_self != nullptr);
        swapcontext(&m_parentContext, &m_self->m_context);
    }

    if (m_release_coro != nullptr) {
        m_context_arena.release(m_release_coro);
        m_release_coro = nullptr;
    }

    assert(m_self == nullptr);
    assert(s_instance == this);
    return m_active_contexts;
}

void dispatcher_t::enqueue_release(coro_t *coro) {
    assert(m_release_coro == nullptr);
    m_release_coro = coro;
    --m_active_contexts;
}

coro_t::coro_t(dispatcher_t* dispatch) :
    m_dispatch(dispatch),
    m_valgrindStackId(VALGRIND_STACK_REGISTER(m_stack, m_stack + sizeof(m_stack))),
    m_wait_result(wait_result_t::Success) { }

coro_t::~coro_t() {
    VALGRIND_STACK_DEREGISTER(m_valgrindStackId);
}

[[noreturn]] void launch_coro() {
    handover_params.check_valid();

    // Copy out the handover parameters so we can clear them
    void (coro_t::*fn)(void*, coro_t*, bool) = handover_params.fn;
    void *params = handover_params.params;
    coro_t *self = handover_params.self;
    coro_t *parent = handover_params.parent;
    bool immediate = handover_params.immediate;

    // Reset the handover parameters for the next coroutine to be spawned
    handover_params.clear();
    (self->*fn)(params, parent, immediate);
    assert(false);
}

void coro_t::begin(void(coro_t::*fn)(void*, coro_t*, bool), void *params, coro_t *parent, bool immediate) {
    int res = getcontext(&m_context);
    assert(res == 0);

    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_stack.ss_size = s_stackSize;
    m_context.uc_link = nullptr;

    // The new coroutine will pick up its parameters from the thread-static variable
    assert(handover_params.fn == nullptr);
    handover_params.init(this, fn, params, parent, immediate);
    makecontext(&m_context, launch_coro, 0);
    swap(this); // Swap in this coroutine immediately
}

void coro_t::end() {
    m_dispatch->enqueue_release(this);
    swap();
    assert(false);
}

void coro_t::swap(coro_t *next) {
    assert(m_dispatch->m_self == this);
    ++m_dispatch->m_swap_count;

    if (next == nullptr) {
        // The current coroutine specified that it needs another coroutine scheduled
        // immediately - skip the queue and run it now.
        m_dispatch->m_self = next;
        swapcontext(&m_context, &next->m_context);
    } else if (m_dispatch->m_run_queue.empty() ||
        m_dispatch->m_swap_count >= dispatcher_t::s_max_swaps_per_loop) {
        m_dispatch->m_self = nullptr;
        swapcontext(&m_context, &m_dispatch->m_parentContext);
    } else {
        m_dispatch->m_self = m_dispatch->m_run_queue.pop_front();
        if (m_dispatch->m_self != this)
            swapcontext(&m_context, &m_dispatch->m_self->m_context);
    }

    if (m_dispatch->m_release_coro != nullptr) {
        m_dispatch->m_context_arena.release(m_dispatch->m_release_coro);
        m_dispatch->m_release_coro = nullptr;
    }

    // If we did a wait that failed, throw an appropriate exception,
    //  so we can require the user to handle it
    wait_result_t waitResult = m_wait_result;
    m_wait_result = wait_result_t::Success;
    switch (waitResult) {
    case wait_result_t::Success:
        break;
    case wait_result_t::Interrupted:
        throw wait_interrupted_exc_t();
    case wait_result_t::ObjectLost:
        throw wait_object_lost_exc_t();
    case wait_result_t::Error:
    default:
        throw wait_error_exc_t("unrecognized error while waiting");
    }

    assert(m_dispatch->m_self == this);
}

coro_t* coro_t::self() {
    return dispatcher_t::get_instance().m_self;
}

void coro_t::wait() {
    dispatcher_t& dispatch = dispatcher_t::get_instance();
    coro_t *coro = self();
    assert(coro != nullptr);
    assert(coro->m_dispatch == &dispatch);
    coro->swap();
}

void coro_t::yield() {
    dispatcher_t& dispatch = dispatcher_t::get_instance();
    coro_t *coro = self();
    assert(coro != nullptr);
    assert(coro->m_dispatch == &dispatch);

    dispatch.m_run_queue.push_back(coro);
    coro->swap();
}

void coro_t::notify(coro_t* coro) {
    assert(coro != nullptr);
    assert(coro != self());
    coro->m_dispatch->m_run_queue.push_back(coro);
}

void coro_t::wait_callback(wait_result_t result) {
    m_wait_result = result;
    notify(this);
}

} // namespace indecorous
