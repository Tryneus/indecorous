#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "rpc/handler.hpp"
#include "rpc/target.hpp"

#include "bench.hpp"

using namespace indecorous;

size_t reps = 10000;

int sum() {
    return 0;
}

template <typename... Args>
int sum(int value, Args &&...args) {
    return value + sum(std::forward<Args>(args)...);
}


template <typename... Args>
void spawn_now_lambda(int expected, Args &&...args) {
    int res = 0;
    bench_timer_t timer("coro/spawn_now lambda", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now([&res] (Args &&...inner_args) {
                res += sum(std::forward<Args>(inner_args)...);
            }, std::forward<Args>(args)...);
    }
    CHECK(res == expected);
}

template <typename... Args>
void spawn_deferred_lambda(int expected, Args &&...args) {
    int res = 0;
    std::vector<future_t<void>> futures;
    futures.reserve(reps);
    bench_timer_t timer("coro/spawn_deferred lambda", reps);
    for (size_t i = 0; i < reps; ++i) {
        futures.push_back(
            coro_t::spawn([&res] (Args &&...inner_args) {
                    res += sum(std::forward<Args>(inner_args)...);
                }, std::forward<Args>(args)...)
        );
    }

    for (size_t i = 0; i < reps; ++i) {
        futures[i].wait();
    }
    CHECK(res == expected);
}

template <typename... Args>
void basic_function(int *out, Args &&...args) {
    *out += sum(std::forward<Args>(args)...);
}

template <typename... Args>
void spawn_now_basic_function(int expected, Args &&...args) {
    int res = 0;
    bench_timer_t timer("coro/spawn_now function pointer", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(&basic_function<Args...>, &res, std::forward<Args>(args)...);
    }

    CHECK(res == expected);
}

template <typename... Args>
void spawn_deferred_basic_function(int expected, Args &&...args) {
    int res = 0;
    std::vector<future_t<void>> futures;
    futures.reserve(reps);
    bench_timer_t timer("coro/spawn_deferred function pointer", reps);
    for (size_t i = 0; i < reps; ++i) {
        futures.push_back(
            coro_t::spawn(&basic_function<Args...>, &res, std::forward<Args>(args)...)
        );
    }


    for (size_t i = 0; i < reps; ++i) {
        futures[i].wait();
    }
    CHECK(res == expected);
}

class spawn_class_t {
public:
    spawn_class_t() : res(0) { }

    template <typename... Args>
    void member_function(Args &&...args) {
        res += sum(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void operator()(Args &&...args) {
        res += sum(std::forward<Args>(args)...);
    }

    int res;
};

template <typename... Args>
void spawn_now_member_function(int expected, Args &&...args) {
    spawn_class_t instance;
    bench_timer_t timer("coro/spawn_now member function", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(&spawn_class_t::member_function<Args...>, &instance, std::forward<Args>(args)...);
    }
    CHECK(instance.res == expected);
}

template <typename... Args>
void spawn_deferred_member_function(int expected, Args &&...args) {
    spawn_class_t instance;
    std::vector<future_t<void>> futures;
    futures.reserve(reps);
    bench_timer_t timer("coro/spawn_deferred member function", reps);
    for (size_t i = 0; i < reps; ++i) {
        futures.push_back(
            coro_t::spawn(&spawn_class_t::member_function<Args...>, &instance, std::forward<Args>(args)...)
        );
    }

    for (size_t i = 0; i < reps; ++i) {
        futures[i].wait();
    }
    CHECK(instance.res == expected);
}

template <typename... Args>
void spawn_now_member_operator(int expected, Args &&...args) {
    bench_timer_t timer("coro/spawn_now member operator ()", reps);
    spawn_class_t instance;
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(std::ref(instance), std::forward<Args>(args)...);
    }
    CHECK(instance.res == expected);
}

template <typename... Args>
void spawn_deferred_member_operator(int expected, Args &&...args) {
    spawn_class_t instance;
    std::vector<future_t<void>> futures;
    futures.reserve(reps);
    bench_timer_t timer("coro/spawn_deferred member operator ()", reps);
    for (size_t i = 0; i < reps; ++i) {
        futures.push_back(
            coro_t::spawn(std::ref(instance), std::forward<Args>(args)...)
        );
    }

    for (size_t i = 0; i < reps; ++i) {
        futures[i].wait();
    }
    CHECK(instance.res == expected);
}

template <typename... Args>
void spawn_now_std_function(int expected, Args &&...args) {
    int res = 0;
    bench_timer_t timer("coro/spawn_now std::function", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(std::function<void(int *, Args&&...)>(&basic_function<Args...>), &res, std::forward<Args>(args)...);
    }

    CHECK(res == expected);
}

template <typename... Args>
void spawn_deferred_std_function(int expected, Args &&...args) {
    int res = 0;
    std::vector<future_t<void>> futures;
    futures.reserve(reps);
    bench_timer_t timer("coro/spawn_deferred std::function", reps);
    for (size_t i = 0; i < reps; ++i) {
        futures.push_back(
            coro_t::spawn(std::function<void(int *, Args&&...)>(&basic_function<Args...>), &res, std::forward<Args>(args)...)
        );
    }

    for (size_t i = 0; i < reps; ++i) {
        futures[i].wait();
    }
    CHECK(res == expected);
}

/*
template <typename... Args>
void spawn_now_std_bind(int expected, Args &&...args) {
    int res = 0;
    bench_timer_t timer("coro/spawn_now std::bind", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(std::bind(&basic_function<Args...>, &res, std::forward<Args>(args)...));
    }
    CHECK(res == expected);
}

template <typename... Args>
void spawn_deferred_std_bind(int expected, Args &&...args) {
    int res = 0;
    std::vector<future_t<void>> futures;
    futures.reserve(reps);
    bench_timer_t timer("coro/spawn_deferred std::bind", reps);
    for (size_t i = 0; i < reps; ++i) {
        futures.push_back(
            coro_t::spawn(std::bind(&basic_function<Args...>, &res, std::forward<Args>(args)...))
        );
    }

    for (size_t i = 0; i < reps; ++i) {
        futures[i].wait();
    }
    CHECK(res == expected);
}
*/

struct bench_t {
    template <int... N>
    static void run_now(int expected, std::integer_sequence<int, N...>) {
        debugf("Running with %zu args", sizeof...(N));
        spawn_now_lambda(expected, N...);
        spawn_now_basic_function(expected, N...);
        spawn_now_member_function(expected, N...);
        spawn_now_member_operator(expected, N...);
        spawn_now_std_function(expected, N...);
        //spawn_now_std_bind(expected, N...);
    }

    template <int... N>
    static void run_deferred(int expected, std::integer_sequence<int, N...>) {
        debugf("Running with %zu args", sizeof...(N));
        spawn_deferred_lambda(expected, N...);
        spawn_deferred_basic_function(expected, N...);
        spawn_deferred_member_function(expected, N...);
        spawn_deferred_member_operator(expected, N...);
        spawn_deferred_std_function(expected, N...);
        //spawn_deferred_std_bind(expected, N...);
    }

    DECLARE_STATIC_RPC(spawn_now)() -> void;
    DECLARE_STATIC_RPC(spawn_deferred)() -> void;
};

IMPL_STATIC_RPC(bench_t::spawn_now)() -> void {
    run_now(0, std::make_integer_sequence<int, 0>{});
    run_now(30000, std::make_integer_sequence<int, 3>{});
    run_now(450000, std::make_integer_sequence<int, 10>{});
}

IMPL_STATIC_RPC(bench_t::spawn_deferred)() -> void {
    run_deferred(0, std::make_integer_sequence<int, 0>{});
    run_deferred(30000, std::make_integer_sequence<int, 3>{});
    run_deferred(450000, std::make_integer_sequence<int, 10>{});
}

TEST_CASE("coro/spawn", "[coro][spawn]") {
    scheduler_t sched_now(1, shutdown_policy_t::Eager);
    sched_now.broadcast_local<bench_t::spawn_now>();
    sched_now.run();

    scheduler_t sched_deferred(1, shutdown_policy_t::Eager);
    sched_deferred.broadcast_local<bench_t::spawn_deferred>();
    sched_deferred.run();
}
