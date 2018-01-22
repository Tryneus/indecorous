#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "rpc/handler.hpp"
#include "rpc/target.hpp"

#include "bench.hpp"

using namespace indecorous;

size_t reps = 1000000;

int sum() {
    return 0;
}

template <typename... Args>
int sum(int value, Args &&...args) {
    return value + sum(std::forward<Args>(args)...);
}


template <typename... Args>
void spawn_lambda(Args &&...args) {
    bench_timer_t timer("coro/spawn lambda", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now([] (Args &&...inner_args) {
                return sum(std::forward<Args>(inner_args)...);
            }, std::forward<Args>(args)...);
    }
}

template <typename... Args>
int basic_function(Args &&...args) {
    return sum(std::forward<Args>(args)...);
}

template <typename... Args>
void spawn_basic_function(Args &&...args) {
    bench_timer_t timer("coro/spawn function pointer", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(&basic_function<Args...>, std::forward<Args>(args)...);
    }
}

class spawn_class_t {
public:
    spawn_class_t() { }

    template <typename... Args>
    int member_function(Args &&...args) {
        return sum(std::forward<Args>(args)...);
    }

    template <typename... Args>
    int operator()(Args &&...args) {
        return sum(std::forward<Args>(args)...);
    }
};

// TODO: member functions are broken
/*
template <typename... Args>
void spawn_member_function(Args &&...args) {
    bench_timer_t timer("coro/spawn member function", reps);
    spawn_class_t instance;
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(&spawn_class_t::member_function<Args...>, &instance, std::forward<Args>(args)...);
    }
}
*/

// TODO: member functions are broken
/*
template <typename... Args>
void spawn_member_operator(Args &&...args) {
    bench_timer_t timer("coro/spawn member operator ()", reps);
    spawn_class_t instance;
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(instance, std::forward<Args>(args)...);
    }
}
*/

template <typename... Args>
void spawn_std_function(Args &&...args) {
    bench_timer_t timer("coro/spawn std::function", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(std::function<void(Args&&...)>(&basic_function<Args...>), std::forward<Args>(args)...);
    }
}

/*
template <typename... Args>
void spawn_std_bind(Args &&...args) {
    bench_timer_t timer("coro/spawn std::bind", reps);
    for (size_t i = 0; i < reps; ++i) {
        coro_t::spawn_now(std::bind(&basic_function<Args...>, std::forward<Args>(args)...));
    }
}
*/

struct bench_t {
    template <int... N>
    static void run(std::integer_sequence<int, N...>) {
        debugf("Running with %zu args", sizeof...(N));
        spawn_lambda(N...);
        spawn_basic_function(N...);
        //spawn_member_function(N...);
        //spawn_member_operator(N...);
        spawn_std_function(N...);
        //spawn_std_bind(N...);
    }

    DECLARE_STATIC_RPC(spawn)() -> void;
};

IMPL_STATIC_RPC(bench_t::spawn)() -> void {
    run(std::make_integer_sequence<int, 0>{});
    run(std::make_integer_sequence<int, 1>{});
    run(std::make_integer_sequence<int, 2>{});
    run(std::make_integer_sequence<int, 3>{});
    run(std::make_integer_sequence<int, 4>{});
    run(std::make_integer_sequence<int, 5>{});
    run(std::make_integer_sequence<int, 6>{});
    run(std::make_integer_sequence<int, 7>{});
    run(std::make_integer_sequence<int, 8>{});
    run(std::make_integer_sequence<int, 9>{});
    run(std::make_integer_sequence<int, 10>{});
}

TEST_CASE("coro/spawn", "[coro][spawn]") {
    scheduler_t sched(1, shutdown_policy_t::Eager);
    sched.broadcast_local<bench_t::spawn>();
    sched.run();
}
