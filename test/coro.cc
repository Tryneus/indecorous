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

TEST_CASE("coro/empty", "[coro][shutdown]") {
    for (size_t i = 1; i < 16; ++i) {
        scheduler_t sched(i, shutdown_policy_t::Eager);
        sched.run();
        sched.run();
        sched.run();
        sched.run();
    }
}

struct spawn_handler_t : public handler_t<spawn_handler_t> {
    static void call(std::string a, std::string b, int value) {
        debugf("Got called with %s, %s, %d", a.c_str(), b.c_str(), value);
    }
};
IMPL_UNIQUE_HANDLER(spawn_handler_t);

TEST_CASE("coro/spawn", "[coro][shutdown]") {
    scheduler_t sched(num_threads, shutdown_policy_t::Eager);
    sched.broadcast_local<spawn_handler_t>("foo", "bar", 1);
    sched.run();

    sched.broadcast_local<spawn_handler_t>("", "-", false);
    sched.run();
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

TEST_CASE("coro/wait", "[coro][sync]") {
    scheduler_t sched(num_threads, shutdown_policy_t::Eager);
    sched.broadcast_local<wait_handler_t>();
    sched.run();

    sched.broadcast_local<wait_handler_t>();
    sched.run();
}

struct suicide_handler_t : public handler_t<suicide_handler_t> {
    static void call(pid_t parent_pid, int signum) {
        kill(parent_pid, signum);
    }
};
IMPL_UNIQUE_HANDLER(suicide_handler_t);

// Disabled by default as it causes complications in a debugger
TEST_CASE("coro/sigint", "[coro][shutdown][hide]") {
    scheduler_t sched(num_threads, shutdown_policy_t::Kill);
    target_t *target = sched.threads().begin()->hub()->self_target();

    target->call_noreply<suicide_handler_t>(getpid(), SIGINT);
    sched.run();

    target->call_noreply<suicide_handler_t>(getpid(), SIGTERM);
    sched.run();
}
