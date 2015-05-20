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

// Not thread-safe, exactly one dispatcher_t per thread
class dispatcher_t
{
public:
    dispatcher_t();
    ~dispatcher_t();

    // Returns the number of outstanding coroutines
    uint32_t run();

private:
    friend class scheduler_t;
    friend class thread_t;
    friend class coro_t;
    static dispatcher_t& get_instance();

    void enqueue_release(coro_t *coro);

    static __thread dispatcher_t* s_instance;

    coro_t *volatile m_self;
    coro_t *m_release_coro; // Recently-finished coro_t to be released
    ucontext_t m_parentContext; // Used to store the parent context, switch to when no coroutines to run
    uint32_t m_swap_count;
    int32_t m_active_contexts;
    static uint32_t s_max_swaps_per_loop;

    Arena<coro_t> m_context_arena; // Arena used to cache context allocations

    intrusive_queue_t<coro_t> m_run_queue; // Queue of contexts to run
};

class coro_t : public wait_callback_t, public intrusive_node_t<coro_t>
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
    static future_t<Res> spawn(Res(*fn)(Args...), Args &&...args) {
        return coro_t::self()->spawn_internal(false, fn, std::forward<Args>(args)...);
    }
    template <typename Res, typename... Args>
    static future_t<Res> spawn_now(Res(*fn)(Args...), Args &&...args) {
        return coro_t::self()->spawn_internal(true, fn, std::forward<Args>(args)...);
    }

private:
    friend class scheduler_t;
    friend class dispatcher_t;
    friend class Arena<coro_t>; // To allow instantiation of this class
    friend void launch_coro();

    template <typename Res, typename... Args>
    future_t<Res> spawn_internal(bool immediate, Res(*fn)(Args...), Args &&...args) {
        promise_t<Res> promise;
        future_t<Res> res = promise.get_future();
        coro_t *context = m_dispatch->m_context_arena.get(m_dispatch);
        std::tuple<promise_t<Res>, Res(*)(Args...), Args...> params(
            { std::move(promise), fn, std::forward<Args>(args)... });

        // Start the other coroutine - it will give us execution back after it has 
        // copied the parameters
        context->begin(hook<Res, Args...>, &params, this, immediate);
        return res;
    }

    template <typename Res, typename... Args>
    [[noreturn]] void hook(void *p, coro_t *parent, bool immediate) {
        typedef std::tuple<coro_t *, coro_t *, promise_t<Res>, Res(*)(Args...), Args...> params_t;
        params_t params = std::move(*reinterpret_cast<params_t *>(p));

        if (immediate) {
            // We're running immediately, put our parent on the back of the run queue
            m_dispatch->m_run_queue.push(parent);
        } else {
            // We're delaying our execution, put ourselves on the back of the run queue
            // and swap back in our parent so they can continue
            m_dispatch->m_run_queue.push(this);
            swap(parent);
        }

        promise_t<Res> *promise = &std::get<0>(params);
        promise->fulfill(hook_internal(std::index_sequence_for<Args...>{}, params));
        end();
    }

    template <size_t... N, typename Res, typename... Args>
    static Res hook_internal(std::integer_sequence<size_t, N...>, std::tuple<Args...> &args) {
        return std::get<1>(args)(std::get<N+2>(std::move(args))...);
    }

    coro_t(dispatcher_t *dispatch);
    ~coro_t();

    void begin(void(coro_t::*fn)(void*, coro_t*, bool), void *params, coro_t *parent, bool immediate);
    [[noreturn]] void end();
    void swap(coro_t *next = nullptr);

    void wait_callback(wait_result_t result);

    static const size_t s_stackSize = 65535; // TODO: This is probably way too small

    dispatcher_t* m_dispatch;
    ucontext_t m_context;
    char m_stack[s_stackSize];
    int m_valgrindStackId;
    wait_result_t m_wait_result;
};

} // namespace indecorous

#endif
