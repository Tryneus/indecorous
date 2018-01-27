#ifndef CORO_CORO_HPP_
#define CORO_CORO_HPP_

#include <ucontext.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <type_traits>

#include "common.hpp"
#include "containers/intrusive.hpp"
#include "sync/promise.hpp"
#include "sync/drainer.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

struct coro_start_t;
class coro_t;
class dispatcher_t;
class interruptor_t;
class shutdown_t;

typedef void(coro_t::*hook_fn_t)(void*);

[[noreturn]] void launch_coro(coro_start_t *);

// Used internally for handing over data when spawning a new coroutine
struct coro_start_t {
    coro_start_t(coro_t *_self,
            hook_fn_t _hook,
            void *_params,
            coro_t *_parent,
            bool _immediate,
            drainer_lock_t &&_lock) :
        self(_self),
        hook(_hook),
        params(_params),
        parent(_parent),
        immediate(_immediate),
        lock(std::move(_lock)) { }

    coro_t *self;
    hook_fn_t hook;
    void *params;
    coro_t *parent;
    bool immediate;
    drainer_lock_t lock;

    DISABLE_COPYING(coro_start_t);
};

class coro_cache_t {
public:
    coro_cache_t(size_t max_cache_size,
                 dispatcher_t *dispatch);
    ~coro_cache_t();

    size_t extant() const { return m_extant; }

    coro_t *get();
    void release(coro_t *stack);
private:
    const size_t m_max_cache_size;
    dispatcher_t *m_dispatch;
    size_t m_extant;
    intrusive_list_t<coro_t> m_cache;

    DISABLE_COPYING(coro_cache_t);
};

// Not thread-safe, exactly one dispatcher_t per thread
class dispatcher_t
{
public:
    dispatcher_t(shutdown_t *shutdown,
                 std::function<void()> initial_fn);
    ~dispatcher_t();

    void run();

    // Called when an rpc is sent from this thread
    void note_new_task();
    void note_accepted_task();

    void enqueue_release(coro_t *coro);

    shutdown_t * const m_shutdown;

    // Used by synchronization primitives with callbacks to fail an assert if the callback attempts
    // to swap coroutines
    bool m_swap_permitted;

    coro_cache_t m_coro_cache; // arena used to cache coro allocations
    intrusive_list_t<coro_t> m_run_queue; // queue of contexts to run

    coro_t * volatile m_running;
    coro_t *m_release; // Recently-finished coro_t to be released

    size_t m_swap_count;
    ucontext_t m_main_context; // Used to store the thread's main context

    static size_t s_max_swaps_per_loop;
private:
    friend class ignore_coro_for_shutdown_t;

    static void run_initial_coro();

    coro_t *m_initial_coro;
    std::function<void()> m_initial_fn;
    int64_t m_coro_delta;

    DISABLE_COPYING(dispatcher_t);
};

template <typename Res>
class coro_result_t : public future_t<Res> {
public:
    coro_result_t(coro_result_t &&other) :
            future_t<Res>(std::move(other)),
            m_drainer(std::move(other.m_drainer)) { }

    drainer_t detach() {
        return std::move(m_drainer);
    }

private:
    friend class coro_t;
    coro_result_t(future_t<Res> &&future) :
            future_t<Res>(std::move(future)),
            m_drainer() { }

    drainer_t m_drainer;

    DISABLE_COPYING(coro_result_t);
};

class coro_t : public intrusive_node_t<coro_t> {
public:
    // Give up execution indefinitely (until notified)
    void wait();

    // Wait temporarily and be rescheduled
    static void yield();

    // Create a coroutine and put it on the queue to run
    template <typename Callable, typename... Args>
    static auto spawn(Callable &&cb, Args &&...args) {
        return coro_t::self()->spawn_internal(false, std::forward<Callable>(cb),
                                              std::forward<Args>(args)...);
    }
    template <typename Callable, typename... Args>
    static auto spawn_now(Callable &&cb, Args &&...args) {
        return coro_t::self()->spawn_internal(true, std::forward<Callable>(cb),
                                              std::forward<Args>(args)...);
    }

    // These are typically used by synchronization primitives
    static coro_t* self(); // Get the currently running coroutine on this thread
    wait_callback_t *wait_callback();

private:
    friend class scheduler_t;
    friend class dispatcher_t;
    friend void launch_coro(coro_start_t *);
    friend class waitable_t;
    friend class coro_cache_t;

    template <typename Callable, typename... Args,
              typename Res = typename std::result_of<Callable(Args...)>::type,
              typename Tuple =
                   std::tuple<promise_t<Res>,
                       typename std::remove_reference<Callable>::type,
                       typename std::remove_reference<Args>::type... >>
    typename std::enable_if<!std::is_member_function_pointer<Callable>::value, coro_result_t<Res> >::type
    spawn_internal(bool immediate, Callable &&cb, Args &&...args) {
        promise_t<Res> promise;
        coro_t *coro = coro_t::create();
        coro_result_t<Res> res(promise.get_future());
        // TODO: put this somewhere in the stack for the new coroutine - it'll save an allocation each call
        // TODO: two allocations here is shitty af
        coro_start_t *start = new coro_start_t(
            coro,
            &coro_t::hook<Res, Tuple, 2>,
            new Tuple(std::move(promise), std::forward<Callable>(cb), std::forward<Args>(args)...),
            this,
            immediate,
            res.m_drainer.lock()
        );

        coro->begin(start);

        if (immediate) {
            swap(coro);
        } else {
            m_dispatch->m_run_queue.push_back(coro);
        }
        return res;
    }

    template <typename Callable, typename... Args,
              typename Res = typename std::result_of<Callable(Args...)>::type,
              typename Tuple =
                   std::tuple<promise_t<Res>,
                       typename std::remove_reference<Callable>::type,
                       typename std::remove_reference<Args>::type... >>
    typename std::enable_if<std::is_member_function_pointer<Callable>::value, coro_result_t<Res> >::type
    spawn_internal(bool immediate, Callable &&cb, Args &&...args) {
        promise_t<Res> promise;
        coro_t *coro = coro_t::create();
        coro_result_t<Res> res(promise.get_future());
        // TODO: put this somewhere in the stack for the new coroutine - it'll save an allocation each call
        // TODO: two allocations here is shitty
        coro_start_t *start = new coro_start_t(
            coro,
            &coro_t::hook<Res, Tuple, 3>,
            new Tuple(std::move(promise), std::forward<Callable>(cb), std::forward<Args>(args)...),
            this,
            immediate,
            res.m_drainer.lock()
        );

        coro->begin(start);

        if (immediate) {
            swap(coro);
        } else {
            m_dispatch->m_run_queue.push_back(coro);
        }
        return res;
    }

    template <typename Res, typename Tuple, size_t ArgOffset>
    void hook(void *p) {
        auto params = reinterpret_cast<Tuple *>(p);
        runner_t<Res>::run(std::make_index_sequence<std::tuple_size<Tuple>::value - ArgOffset>{}, params);
        delete params;
    }

    template <typename Res>
    class runner_t {
    public:
        template <size_t... N, typename Callable, typename... Args>
        static typename std::enable_if<!std::is_member_function_pointer<Callable>::value, void>::type
        run(std::integer_sequence<size_t, N...>, std::tuple<promise_t<Res>, Callable, Args...> *args) {
            promise_t<Res> promise(std::get<0>(std::move(*args)));
            promise.fulfill(std::get<1>(*args)(std::get<N+2>(std::move(*args))...));
        }

        template <size_t... N, typename Callable, typename... Args>
        static typename std::enable_if<std::is_member_function_pointer<Callable>::value, void>::type
        run(std::integer_sequence<size_t, N...>, std::tuple<promise_t<Res>, Callable, Args...> *args) {
            promise_t<Res> promise(std::get<0>(std::move(*args)));
            promise.fulfill((std::get<2>(std::move(*args))->*std::get<1>(std::move(*args)))(std::get<N+3>(std::move(*args))...));
        }
    };

    explicit coro_t(dispatcher_t *dispatch);
    ~coro_t();

    void begin(coro_start_t *start);
    void swap(coro_t *next);

    void notify(wait_result_t result);

    static coro_t *create();

    static const size_t s_stackSize = 65535; // TODO: This is probably way too small

    // Interface for interruptors to register/deregister themselves
    friend class interruptor_t;
    interruptor_t *add_interruptor(interruptor_t *interruptor);
    void remove_interruptor(interruptor_t *interruptor);

    // When waiting on things, we need to check the current interruptors
    friend class multiple_waiter_t;
    interruptor_t *get_interruptor();

    // Use this rather than inherit from it directly to avoid ugly multiple inheritance
    // of intrusive_node_t.
    class coro_wait_callback_t : public wait_callback_t {
    public:
        explicit coro_wait_callback_t(coro_t *parent);
    private:
        void wait_done(wait_result_t result);
        void object_moved(waitable_t *new_ptr);
        coro_t *m_parent;

        DISABLE_COPYING(coro_wait_callback_t);
    };

    static const size_t s_page_size;
    static const size_t s_stack_size;

    dispatcher_t *m_dispatch;
    ucontext_t m_context;
    char *m_stack;
    int m_valgrind_stack_id;
    coro_wait_callback_t m_wait_callback;
    wait_result_t m_wait_result;
    intrusive_list_t<interruptor_t> m_interruptors;

    DISABLE_COPYING(coro_t);
};

// Specialization for void-returning functions
template <> class coro_t::runner_t<void> {
public:
    template <size_t... N, typename Callable, typename... Args>
    static typename std::enable_if<!std::is_member_function_pointer<Callable>::value, void>::type
    run(std::integer_sequence<size_t, N...>, std::tuple<promise_t<void>, Callable, Args...> *args) {
        promise_t<void> promise(std::get<0>(std::move(*args)));
        std::get<1>(std::move(*args))(std::get<N+2>(std::move(*args))...);
        promise.fulfill();
    }

    template <size_t... N, typename Callable, typename... Args>
    static typename std::enable_if<std::is_member_function_pointer<Callable>::value, void>::type
    run(std::integer_sequence<size_t, N...>, std::tuple<promise_t<void>, Callable, Args...> *args) {
        promise_t<void> promise(std::get<0>(std::move(*args)));
        (std::get<2>(std::move(*args))->*std::get<1>(std::move(*args)))(std::get<N+3>(std::move(*args))...);
        promise.fulfill();
    }
};

} // namespace indecorous

#endif
