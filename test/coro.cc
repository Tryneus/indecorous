#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"
#include "rpc/handler.hpp"
#include "rpc/hub_data.hpp"
#include "rpc/target.hpp"
#include "rpc/serialize_stl.hpp"
#include "sync/timer.hpp"
#include "sync/multiple_wait.hpp"

const size_t num_threads = 8;

using namespace indecorous;

TEST_CASE("coro/empty", "Test empty environment creation/destruction") {
    for (size_t i = 0; i < 16; ++i) {
        scheduler_t sched(i);
        sched.run(shutdown_policy_t::Eager);
        sched.run(shutdown_policy_t::Eager);
        sched.run(shutdown_policy_t::Eager);
        sched.run(shutdown_policy_t::Eager);
    }
}

struct spawn_handler_t : public handler_t<spawn_handler_t> {
    static void call(std::string a, std::string b, int value) {
        debugf("Got called with %s, %s, %d", a.c_str(), b.c_str(), value);
    }
};
IMPL_UNIQUE_HANDLER(spawn_handler_t);

TEST_CASE("coro/spawn", "Test basic coroutine spawning and running") {
    scheduler_t sched(num_threads);
    sched.broadcast_local<spawn_handler_t>("foo", "bar", 1);
    sched.run(shutdown_policy_t::Eager);

    sched.broadcast_local<spawn_handler_t>("", "-", false);
    sched.run(shutdown_policy_t::Eager);
}

struct wait_handler_t : public handler_t<wait_handler_t> {
    static void call() {
        periodic_timer_t timer_a;
        single_timer_t timer_b;
        timer_a.start(10);
        timer_b.start(100);
        wait_any(&timer_a, timer_b);
        wait_all(timer_a, &timer_b);
    }
};
IMPL_UNIQUE_HANDLER(wait_handler_t);

TEST_CASE("coro/wait", "Test basic coroutine waiting") {
    scheduler_t sched(num_threads);
    sched.broadcast_local<wait_handler_t>();
    sched.run(shutdown_policy_t::Eager);

    sched.broadcast_local<wait_handler_t>();
    sched.run(shutdown_policy_t::Eager);
}

struct interrupt_handler_t : public handler_t<interrupt_handler_t> {
    static void call() {
        raise(SIGINT);
    }
};
IMPL_UNIQUE_HANDLER(interrupt_handler_t);

TEST_CASE("coro/sigint", "Test shutdown policy 'kill'") {
    scheduler_t sched(num_threads);
    target_t *target = sched.threads().begin()->hub()->self_target();

    target->call_noreply<interrupt_handler_t>();
    sched.run(shutdown_policy_t::Kill);

    target->call_noreply<interrupt_handler_t>();
    sched.run(shutdown_policy_t::Kill);
}
