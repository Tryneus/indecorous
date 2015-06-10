#ifndef CORO_CORO_HPP_
#define CORO_CORO_HPP_

#include <ucontext.h>

#include <atomic>
#include <cassert>
#include <cstddef>

#include "containers/arena.hpp"
#include "containers/intrusive.hpp"
#include "sync/event.hpp"
#include "sync/promise.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

class coro_t;

[[noreturn]] void launch_coro();

// Not thread-safe, exactly one dispatcher_t per thread
class dispatcher_t
{
public:
    dispatcher_t();
    ~dispatcher_t();

    // Used by synchronization primitives with callbacks to fail an assert if the callback attempts
    // to swap coroutines
    bool m_swap_permitted;

    // Returns the delta in active local coroutines
    int64_t run();

    void shutdown();

    // Called when an rpc is sent from this thread
    void note_send();

    void enqueue_release(coro_t *coro);

    arena_t<coro_t> m_context_arena; // arena used to cache context allocations
    intrusive_list_t<coro_t> m_run_queue; // queue of contexts to run

    coro_t *volatile m_running;
    coro_t *m_release; // Recently-finished coro_t to be released

    size_t m_swap_count;
    event_t m_shutdown_event;

    ucontext_t m_main_context; // Used to store the thread's main context

    static size_t s_max_swaps_per_loop;
private:
    coro_t *m_rpc_consumer;
    int64_t m_coro_delta;
};

class coro_t : public intrusive_node_t<coro_t> {
public:
    // Get the running coroutine
    static coro_t* self();

    // Give up execution indefinitely (until notified)
    void wait();

    // Wait temporarily and be rescheduled
    static void yield();

    // Create a coroutine and put it on the queue to run
    template <typename Res, typename... Args>
    static future_t<Res> spawn(Res(*fn)(Args...), Args &&...args) {
        return coro_t::self()->spawn_internal(false, fn, std::forward<Args>(args)...);
    }
    template <typename Res, typename Class, typename... Args>
    static future_t<Res> spawn(Res(Class::*fn)(Args...), Class *instance, Args &&...args) {
        return coro_t::self()->spawn_internal(false, fn, instance, std::forward<Args>(args)...);
    }
    template <typename Res, typename... Args>
    static future_t<Res> spawn_now(Res(*fn)(Args...), Args &&...args) {
        return coro_t::self()->spawn_internal(true, fn, std::forward<Args>(args)...);
    }
    template <typename Res, typename Class, typename... Args>
    static future_t<Res> spawn_now(Res(Class::*fn)(Args...), Class *instance, Args &&...args) {
        return coro_t::self()->spawn_internal(true, fn, instance, std::forward<Args>(args)...);
    }

    typedef void(coro_t::*hook_fn_t)(coro_t*, void*, bool);

    wait_callback_t *wait_callback();

private:
    friend class scheduler_t;
    friend class dispatcher_t;
    friend class arena_t<coro_t>; // To allow instantiation of this class
    friend void launch_coro();
    friend void coro_pull();
    friend class wait_object_t;

    template <typename Res, typename... Args>
    future_t<Res> spawn_internal(bool immediate, Res(*fn)(Args...), Args &&...args) {
        promise_t<Res> promise;
        future_t<Res> res = promise.get_future();
        coro_t *context = coro_t::create();
        std::tuple<promise_t<Res>, Res(*)(Args...), Args...> params(
            { std::move(promise), fn, std::forward<Args>(args)... });
        context->begin(&coro_t::hook<Res, decltype(params), 2>, &params, this, immediate);
        swap(context);
        return res;
    }

    template <typename Res, typename Class, typename... Args>
    future_t<Res> spawn_internal(bool immediate, Res(Class::*fn)(Args...), Class *instance, Args &&...args) {
        promise_t<Res> promise;
        future_t<Res> res = promise.get_future();
        coro_t *context = coro_t::create();
        std::tuple<promise_t<Res>, Res(Class::*)(Args...), Class *, Args...> params(
            std::move(promise), fn, instance, std::forward<Args>(args)... );
        context->begin(&coro_t::hook<Res, decltype(params), 3>, this, &params, immediate);
        swap(context);
        return res;
    }

    void maybe_swap_parent(coro_t *parent, bool immediate);

    template <typename Res, typename Tuple, size_t ArgOffset>
    void hook(coro_t *parent, void *p, bool immediate) {
        Tuple params(std::move(*reinterpret_cast<Tuple *>(p)));
        maybe_swap_parent(parent, immediate);
        runner_t<Res>::run(std::make_index_sequence<std::tuple_size<Tuple>::value - ArgOffset>{}, &params);
        end();
    }

    template <typename Res>
    class runner_t {
    public:
        template <size_t... N, typename... Args>
        static void run(std::integer_sequence<size_t, N...>,
                        std::tuple<promise_t<Res>, Res(*)(Args...), Args...> *args) {
            promise_t<Res> promise(std::get<0>(std::move(args)));
            promise.fulfill(std::get<1>(args)(std::get<N+2>(std::move(args))...));
        }

        template <size_t... N, typename Class, typename... Args>
        static void run(std::integer_sequence<size_t, N...>,
                        std::tuple<promise_t<Res>, Res(Class::*)(Args...), Class *, Args...> *args) {
            promise_t<Res> promise(std::get<0>(std::move(args)));
            Class *instance = std::get<2>(args);
            Res(Class::*fn)(Args...) = std::get<1>(args);
            promise.fulfill((instance->*fn)(std::get<N+3>(std::move(args))...));
        }
    };

    coro_t(dispatcher_t *dispatch);
    ~coro_t();

    void begin(hook_fn_t, coro_t *parent, void *params, bool immediate);
    void swap(coro_t *next);
    [[noreturn]] void end();

    void notify(wait_result_t result);

    static coro_t *create();

    static const size_t s_stackSize = 65535; // TODO: This is probably way too small

    // Use this rather than inherit from it directly to avoid ugly multiple inheritance
    // of intrusive_node_t.
    class coro_wait_callback_t : public wait_callback_t {
    public:
        coro_wait_callback_t(coro_t *parent);
    private:
        void wait_done(wait_result_t result);
        coro_t *m_parent;
    };

    dispatcher_t *m_dispatch;
    ucontext_t m_context;
    char m_stack[s_stackSize];
    int m_valgrindStackId;
    coro_wait_callback_t m_wait_callback;
    wait_result_t m_wait_result;
};

// Specialization for void-returning functions
template <> class coro_t::runner_t<void> {
public:
    template <size_t... N, typename... Args>
    static void run(std::integer_sequence<size_t, N...>,
                    std::tuple<promise_t<void>, void(*)(Args...), Args...> *args) {
        promise_t<void> promise(std::get<0>(std::move(*args)));
        std::get<1>(*args)(std::get<N+2>(std::move(*args))...);
        promise.fulfill();
    }

    template <size_t... N, typename Class, typename... Args>
    static void run(std::integer_sequence<size_t, N...>,
                    std::tuple<promise_t<void>, void(Class::*)(Args...), Class *, Args...> *args) {
        promise_t<void> promise(std::get<0>(std::move(*args)));
        Class *instance = std::get<2>(*args);
        void(Class::*fn)(Args...) = std::get<1>(*args);
        (instance->*fn)(std::get<N+3>(std::move(*args))...);
        promise.fulfill();
    }
};

} // namespace indecorous

#endif
