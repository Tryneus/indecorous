#include "coro/coro.hpp"

#include <sys/eventfd.h>
#include <unistd.h>
#include <valgrind/valgrind.h>

#include "sync/event.hpp"
#include "coro/sched.hpp"
#include "coro/thread.hpp"
#include "rpc/target.hpp"
#include "errors.hpp"

namespace indecorous {

size_t dispatcher_t::s_max_swaps_per_loop = 100;

// Used to hand over parameters to a new coroutine - since we can't safely pass pointers
struct handover_params_t {
    void init(coro_t *_self,
              coro_t::hook_fn_t _fn,
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
    coro_t::hook_fn_t fn;
    void *params;
    coro_t *parent;
    bool immediate;
};
thread_local handover_params_t handover_params;

void coro_pull() {
    thread_t *thread = thread_t::self();
    dispatcher_t *dispatch = thread->dispatcher();
    message_hub_t *hub = thread->hub();
    local_target_t *target = hub->self_target();
    try {
        while (true) {
            while (target->handle(hub)) { }
            target->wait(&dispatch->m_close_event);
        }
    } catch (const wait_interrupted_exc_t &ex) {
        // pass
    }
    assert(dispatch->m_close_event.triggered());

    coro_t *self = coro_t::self();
    dispatch->enqueue_release(self);
    self->swap(nullptr);
}

dispatcher_t::dispatcher_t() :
        m_swap_permitted(true),
        m_context_arena(32),
        m_running(nullptr),
        m_release(nullptr),
        m_swap_count(0),
        m_rpc_consumer(m_context_arena.get(this)),
        m_coro_delta(0) {
    // Save the currently running context
    int res = getcontext(&m_main_context);
    assert(res == 0);

    // Set up the rpc consumer coroutine
    makecontext(&m_rpc_consumer->m_context, coro_pull, 0);
    m_run_queue.push_back(m_rpc_consumer);
}

dispatcher_t::~dispatcher_t() {
    // Shutdown should have been performed first
    // TODO: make this happen in the destructor using RAII on the thread::main?
    assert(m_rpc_consumer == nullptr);
}

void dispatcher_t::close() {
    assert(m_running == nullptr);
    assert(m_run_queue.size() == 0);

    m_close_event.set();
    assert(m_run_queue.size() == 1);
    m_running = m_run_queue.pop_front();
    assert(m_running == m_rpc_consumer);

    swapcontext(&m_main_context, &m_rpc_consumer->m_context);

    assert(m_release == m_rpc_consumer);
    m_context_arena.release(m_rpc_consumer);
    m_rpc_consumer = nullptr;
}

int64_t dispatcher_t::run() {
    assert(m_running == nullptr);
    m_swap_count = 0;
    m_coro_delta = 0;
    
    // Kick off the coroutines, they will give us back execution later
    m_running = m_run_queue.pop_front();
    if (m_running != nullptr) {
        swapcontext(&m_main_context, &m_running->m_context);
    }

    if (m_release != nullptr) {
        m_context_arena.release(m_release);
        m_release = nullptr;
    }

    assert(m_running == nullptr);
    return m_coro_delta;
}

void dispatcher_t::note_send() {
    m_coro_delta += 1;
}

void dispatcher_t::enqueue_release(coro_t *coro) {
    assert(m_release == nullptr);
    m_release = coro;
    --m_coro_delta;
}

coro_t::coro_t(dispatcher_t *dispatch) :
        m_dispatch(dispatch),
        m_valgrindStackId(VALGRIND_STACK_REGISTER(m_stack, m_stack + sizeof(m_stack))),
        m_wait_callback(this),
        m_wait_result(wait_result_t::Success) {
    int res = getcontext(&m_context);
    assert(res == 0);

    m_context.uc_stack.ss_sp = m_stack;
    m_context.uc_stack.ss_size = s_stackSize;
    m_context.uc_link = nullptr;
}

coro_t::~coro_t() {
    VALGRIND_STACK_DEREGISTER(m_valgrindStackId);
}

void launch_coro() {
    handover_params.check_valid();

    // Copy out the handover parameters so we can clear them
    coro_t::hook_fn_t fn = handover_params.fn;
    void *params = handover_params.params;
    coro_t *self = handover_params.self;
    coro_t *parent = handover_params.parent;
    bool immediate = handover_params.immediate;

    // Reset the handover parameters for the next coroutine to be spawned
    handover_params.clear();
    (self->*fn)(parent, params, immediate);
    assert(false);
}

void coro_t::begin(coro_t::hook_fn_t fn, coro_t *parent, void *params, bool immediate) {
    // The new coroutine will pick up its parameters from the thread-static variable
    assert(handover_params.fn == nullptr);
    handover_params.init(this, fn, params, parent, immediate);
    makecontext(&m_context, launch_coro, 0);
}

void coro_t::maybe_swap_parent(coro_t *parent, bool immediate) {
    if (immediate) {
        // We're running immediately, put our parent on the back of the run queue
        m_dispatch->m_run_queue.push_back(parent);
    } else {
        // We're delaying our execution, put ourselves on the back of the run queue
        // and swap back in our parent so they can continue
        m_dispatch->m_run_queue.push_back(this);
        swap(parent);
    }
}

coro_t *coro_t::create() {
    dispatcher_t *dispatch = thread_t::self()->dispatcher();
    return dispatch->m_context_arena.get(dispatch);
}

void coro_t::end() {
    m_dispatch->enqueue_release(this);
    swap(nullptr);
    assert(false);
}

void coro_t::swap(coro_t *next) {
    assert(m_dispatch->m_swap_permitted);
    ++m_dispatch->m_swap_count;

    if (next != nullptr) {
        // The current coroutine specified that it needs another coroutine scheduled
        // immediately - skip the queue and run it now.
        m_dispatch->m_running = next;
        swapcontext(&m_context, &next->m_context);
    } else if (m_dispatch->m_run_queue.empty() ||
               m_dispatch->m_swap_count >= dispatcher_t::s_max_swaps_per_loop) {
        m_dispatch->m_running = nullptr;
        swapcontext(&m_context, &m_dispatch->m_main_context);
    } else {
        m_dispatch->m_running = m_dispatch->m_run_queue.pop_front();
        if (m_dispatch->m_running != this) {
            swapcontext(&m_context, &m_dispatch->m_running->m_context);
        }
    }

    if (m_dispatch->m_release != nullptr) {
        m_dispatch->m_context_arena.release(m_dispatch->m_release);
        m_dispatch->m_release = nullptr;
    }

    // Check if we did a wait that failed
    wait_result_t wait_result = m_wait_result;
    m_wait_result = wait_result_t::Success;
    check_wait_result(wait_result);

    assert(m_dispatch->m_running == this);
}

coro_t* coro_t::self() {
    return thread_t::self()->dispatcher()->m_running;
}

void coro_t::wait() {
    assert(this == self());
    swap(nullptr);
}

void coro_t::yield() {
    coro_t *coro = self();
    coro->m_dispatch->m_run_queue.push_back(coro);
    coro->swap(nullptr);
}

void coro_t::notify(wait_result_t result) {
    assert(!in_a_list());
    m_dispatch->m_run_queue.push_back(this);
    m_wait_result = result;
}

wait_callback_t *coro_t::wait_callback() {
    return &m_wait_callback;
}

coro_t::coro_wait_callback_t::coro_wait_callback_t(coro_t *parent) :
    m_parent(parent) { }

void coro_t::coro_wait_callback_t::wait_done(wait_result_t result) {
    assert(!in_a_list());
    m_parent->notify(result);
}

} // namespace indecorous
