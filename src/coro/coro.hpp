#ifndef CORO_CORO_HPP_
#define CORO_CORO_HPP_
#include <atomic>
#include <ucontext.h>
#include <cassert>

#include "containers/arena.hpp"
#include "containers/queue.hpp"
#include "sync/wait_object.hpp"
#include "sync/promise.hpp"

namespace indecorous {

class coro_t;

// Not thread-safe, exactly one CoroDispatcher per thread
class CoroDispatcher
{
public:
    CoroDispatcher();
    ~CoroDispatcher();

    // Returns the number of outstanding coroutines
    uint32_t run();

private:
    friend class CoroScheduler;
    friend class coro_t;
    static CoroDispatcher& getInstance();

    void enqueue_release(coro_t *coro);

    static __thread CoroDispatcher* s_instance;

    coro_t *volatile m_self;
    coro_t *m_release_coro; // Recently-finished coro_t to be released
    ucontext_t m_parentContext; // Used to store the parent context, switch to when no coroutines to run
    uint32_t m_swap_count;
    int32_t m_active_contexts;
    static uint32_t s_max_swaps_per_loop;

    Arena<coro_t> m_context_arena; // Arena used to cache context allocations

    IntrusiveQueue<coro_t> m_runQueue; // Queue of contexts to run
};

// Pointers to contexts can be used by other modules, but they should not be changed at all
//  The QueueNode part *may* be changed if it is certain that the context is not already enqueued to run
//  otherwise an assert will fail
class coro_t : public wait_callback_t, public QueueNode<coro_t>
{
public:
    // Get the running coroutine
    static coro_t* self();

    // Give up execution indefinitely (until notified)
    static void wait();

    // Wait temporarily and be rescheduled
    static void yield();

    // Queue up another coroutine to be run
    static void notify(coro_t* coro);

    // Create a coroutine and put it on the queue to run
    template <typename Res, typename... Args>
    static promise_t<Res> spawn(Res(*fn)(Args...), Args &&...args) {
        return spawn_internal(false, fn, std::forward<Args>(args)...);
    }
    template <typename Res, typename... Args>
    static promise_t<Res> spawn_now(Res(*fn)(Args...), Args &&...args) {
        return spawn_internal(true, fn, std::forward<Args>(args)...);
    }

private:
    friend class CoroScheduler;
    friend class CoroDispatcher;
    friend class Arena<coro_t>; // To allow instantiation of this class

    template <typename Res, typename... Args>
    static promise_t<Res> spawn_internal(bool immediate, Res(*fn)(Args...), Args &&...args) {
        promise_t<Res> promise;
        coro_t *self = coro_t::self();
        coro_t *context = self->m_dispatch->m_context_arena.get(self->m_dispatch, immediate, hook<Res, Args...>);
        // TODO: this tuple stuff is ugly as hell
        std::tuple<coro_t *, coro_t *, promise_t<Res>, Res(*)(Args...), Args...> params(
            { coro_t::self(), context, promise, fn, std::forward<Args>(args)... });

        // Start the other coroutine - it will return once it has copied the parameters
        context->begin(&params);
        return promise;
    }

    template <typename Res, typename... Args>
    [[noreturn]] static void hook(void *p) {
        typedef std::tuple<coro_t *, coro_t *, promise_t<Res>, Res(*)(Args...), Args...> params_t;
        params_t params = std::move(*reinterpret_cast<params_t *>(p));
        coro_t *parent = std::get<0>(params);
        coro_t *self = std::get<1>(params);

        if (self->m_immediate) {
            // We're running immediately, put our parent on the back of the run queue
            self->m_dispatch->m_runQueue.push(parent);
        } else {
            // We're delaying our execution, put ourselves on the back of the run queue
            // and swap back in our parent so they can continue
            self->m_dispatch->m_runQueue.push(self);
            self->swap(parent);
        }

        promise_t<Res> *promise = &std::get<2>(params);
        promise->fulfill(hook_internal(std::index_sequence_for<Args...>{}, params));
        
        // TODO: would be nice to move this out of the header
        self->m_dispatch->enqueue_release(self);
        self->swap();
    }

    template <size_t... N, typename Res, typename... Args>
    static Res hook_internal(std::integer_sequence<size_t, N...>, std::tuple<Args...> &args) {
        return std::get<3>(args)(std::forward<Args>(std::get<N+4>(args))...);
    }

    coro_t(CoroDispatcher *dispatch, bool immediate);
    ~coro_t();

    void begin(void(*fn)(void*), void *params);
    void swap(coro_t *next = nullptr);

    void wait_callback(wait_result_t result);

    static const size_t s_stackSize = 65535; // TODO: This is probably way too small

    CoroDispatcher* m_dispatch;
    bool m_immediate;
    ucontext_t m_context;
    char m_stack[s_stackSize];
    int m_valgrindStackId;
    wait_result_t m_wait_result;
};

} // namespace indecorous

#endif
