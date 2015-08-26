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
#include "rpc/target.hpp"
#include "rpc/serialize_stl.hpp"
#include "sync/timer.hpp"
#include "sync/multiple_wait.hpp"

const size_t num_threads = 8;

using namespace indecorous;

struct coro_test_t {
    DECLARE_STATIC_RPC(coro_test_t, log, std::string, std::string, int) -> void;
    DECLARE_STATIC_RPC(coro_test_t, wait) -> void;
    DECLARE_STATIC_RPC(coro_test_t, suicide, pid_t parent_pid, int signum) -> void;
};

IMPL_STATIC_RPC(coro_test_t::log, std::string a, std::string b, int value) -> void {
    debugf("Got called with %s, %s, %d", a.c_str(), b.c_str(), value);
}

IMPL_STATIC_RPC(coro_test_t::wait) -> void {
    periodic_timer_t timer_a;
    single_timer_t timer_b;
    timer_a.start(10);
    timer_b.start(100);
    wait_any(&timer_a, timer_b);
    wait_all(timer_a, &timer_b);
}

IMPL_STATIC_RPC(coro_test_t::suicide, pid_t parent_pid, int signum) -> void {
    kill(parent_pid, signum);
}

TEST_CASE("coro/empty", "[coro][shutdown]") {
    for (size_t i = 1; i < 16; ++i) {
        scheduler_t sched(i, shutdown_policy_t::Eager);
        sched.run();
        sched.run();
        sched.run();
        sched.run();
    }
}

TEST_CASE("coro/spawn", "[coro][shutdown]") {
    scheduler_t sched(num_threads, shutdown_policy_t::Eager);
    sched.broadcast_local<coro_test_t::log>("foo", "bar", 1);
    sched.run();

    sched.broadcast_local<coro_test_t::log>("", "-", false);
    sched.run();
}

TEST_CASE("coro/wait", "[coro][sync]") {
    scheduler_t sched(num_threads, shutdown_policy_t::Eager);
    sched.broadcast_local<coro_test_t::wait>();
    sched.run();

    sched.broadcast_local<coro_test_t::wait>();
    sched.run();
}

// Disabled by default as it causes complications in a debugger
TEST_CASE("coro/sigint", "[coro][shutdown][hide]") {
    scheduler_t sched(num_threads, shutdown_policy_t::Kill);
    target_t *target = sched.threads().begin()->hub()->self_target();

    target->call_noreply<coro_test_t::suicide>(getpid(), SIGINT);
    sched.run();

    target->call_noreply<coro_test_t::suicide>(getpid(), SIGTERM);
    sched.run();
}
