#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include "test.hpp"

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
    DECLARE_STATIC_RPC(log)(std::string, std::string, int) -> void;
    DECLARE_STATIC_RPC(wait)() -> void;
    DECLARE_STATIC_RPC(suicide)(pid_t parent_pid, int signum) -> void;
};

IMPL_STATIC_RPC(coro_test_t::log)(std::string a, std::string b, int value) -> void {
    debugf("Got called with %s, %s, %d", a.c_str(), b.c_str(), value);
}

IMPL_STATIC_RPC(coro_test_t::wait)() -> void {
    periodic_timer_t timer_a;
    single_timer_t timer_b;
    timer_a.start(10);
    timer_b.start(100);
    wait_any(&timer_a, timer_b);
    wait_all(timer_a, &timer_b);
}

IMPL_STATIC_RPC(coro_test_t::suicide)(pid_t parent_pid, int signum) -> void {
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
    target_t *target = sched.local_targets()[0];

    target->call_noreply<coro_test_t::suicide>(getpid(), SIGINT);
    sched.run();

    target->call_noreply<coro_test_t::suicide>(getpid(), SIGTERM);
    sched.run();
}

class construction_count_t {
public:
    construction_count_t() {
        s_new_count += 1;
    }

    construction_count_t(const construction_count_t &) {
        s_copy_count += 1;
    }

    construction_count_t(construction_count_t &&) {
        s_move_count += 1;
    }

    static void reset() {
        s_new_count = 0;
        s_copy_count = 0;
        s_move_count = 0;
    }

    static size_t s_new_count;
    static size_t s_copy_count;
    static size_t s_move_count;
};

size_t construction_count_t::s_new_count = 0;
size_t construction_count_t::s_copy_count = 0;
size_t construction_count_t::s_move_count = 0;

SIMPLE_TEST(coro, move_count, 1, "[coro][move_count]") {
    {
        construction_count_t::reset();
        construction_count_t count;

        coro_t::spawn_now([](construction_count_t) {}, count);
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 1);
        CHECK(construction_count_t::s_move_count == 1);
    }

    {
        construction_count_t::reset();

        coro_t::spawn_now([](construction_count_t) {}, construction_count_t());
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 0);
        CHECK(construction_count_t::s_move_count == 2);
    }

    {
        construction_count_t::reset();
        construction_count_t count;

        coro_t::spawn_now([](construction_count_t) {}, std::ref(count));
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 1);
        CHECK(construction_count_t::s_move_count == 0);
    }

    {
        construction_count_t::reset();
        construction_count_t count;

        coro_t::spawn_now([](construction_count_t) {}, std::move(count));
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 0);
        CHECK(construction_count_t::s_move_count == 2);
    }

    {
        construction_count_t::reset();
        construction_count_t count;

        coro_t::spawn_now([](const construction_count_t &) {}, std::cref(count));
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 0);
        CHECK(construction_count_t::s_move_count == 0);
    }

    {
        construction_count_t::reset();
        construction_count_t count;

        coro_t::spawn_now([](const construction_count_t &) {}, count);
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 1);
        CHECK(construction_count_t::s_move_count == 0);
    }

    {
        construction_count_t::reset();

        coro_t::spawn_now([](const construction_count_t &) {}, construction_count_t());
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 0);
        CHECK(construction_count_t::s_move_count == 1);
    }

    {
        construction_count_t::reset();
        construction_count_t count;

        coro_t::spawn_now([](construction_count_t &) {}, std::ref(count));
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 0);
        CHECK(construction_count_t::s_move_count == 0);
    }

    {
        construction_count_t::reset();

        coro_t::spawn_now([](construction_count_t &&) {}, construction_count_t());
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 0);
        CHECK(construction_count_t::s_move_count == 1);
    }

    {
        construction_count_t::reset();
        construction_count_t count;

        coro_t::spawn_now([](construction_count_t &&) {}, std::move(count));
        CHECK(construction_count_t::s_new_count == 1);
        CHECK(construction_count_t::s_copy_count == 0);
        CHECK(construction_count_t::s_move_count == 1);
    }
}

class param_t {
public:
    param_t(int init_val) : value(init_val) { }
    param_t(const param_t &other) : value(other.value) { }
    param_t(param_t &&other) : value(other.value) { other.value = -1; }

    void set(int new_val) { value = new_val; }

    int get() const { return value; }
private:
    int value;
};

SIMPLE_TEST(coro, spawn_params, 2, "[coro][spawn_params]") {
    // Passing a naked value
    {
        // Should copy the object (on the stack) and move the copy to the coroutine
        param_t param(1);
        coro_result_t<void> done = coro_t::spawn([&] (param_t p) {
                CHECK(p.get() == 1);
                p.set(0);
            }, param);
        CHECK(param.get() == 1);
        done.wait();
        CHECK(param.get() == 1);
    }

    {
        // Should copy the object and pass it on
        param_t param(0);
        coro_t::spawn_now([&] (const param_t &p) { CHECK(p.get() == 0); }, param);
        coro_result_t<void> done = coro_t::spawn([&] (const param_t &p) { CHECK(p.get() == 0); }, param);
        CHECK(param.get() == 0);
        param.set(1);
        done.wait();
        CHECK(param.get() == 1);
    }

    {
        // Can't pass a normal variable as a reference (?)
        // param_t param(0);
        // coro_result_t<void> done = coro_t::spawn([&] (param_t &p) { CHECK(p.get() == 2); }, std::ref(param));
        // param.set(2);
        // done.wait();
    }

    {
        // Can't call an r-value reference with a normal variable
        // param_t param(0);
        // coro_result_t<void> done = coro_t::spawn([&] (param_t &&p) { CHECK(p.get() == 0); }, param);
        // done.wait();
    }

    // Source is a const reference
    {
        // Should pass a const-reference which is copied when the coroutine starts
        param_t param(0);
        coro_result_t<void> done = coro_t::spawn([&] (param_t p) { CHECK(p.get() == 1); }, std::cref(param));
        CHECK(param.get() == 0);
        param.set(1);
        CHECK(param.get() == 1);
        done.wait();
        CHECK(param.get() == 1);
    }

    {
        // Should pass a const reference to the object
        param_t param(0);
        coro_result_t<void> done = coro_t::spawn([&] (const param_t &p) { CHECK(p.get() == 1); }, std::cref(param));
        CHECK(param.get() == 0);
        param.set(1);
        CHECK(param.get() == 1);
        done.wait();
        CHECK(param.get() == 1);
    }

    {
        // Can't call a reference using a const reference
        // param_t param(0);
        // coro_result_t<void> done = coro_t::spawn([&] (param_t &p) { CHECK(p.get() == 1); }, std::cref(param));
        // done.wait();
    }

    {
        // Can't call an r-value reference using a const reference
        // param_t param(0);
        // coro_result_t<void> done = coro_t::spawn([&] (param_t &&p) { CHECK(p.get() == 1); }, std::cref(param));
        // done.wait();
    }

    // Source is a reference
    {
        // Should pass a reference which is copied when the coroutine starts
        param_t param(0);
        coro_result_t<void> done = coro_t::spawn([&] (param_t p) {
                CHECK(p.get() == 1);
                p.set(2);
            }, std::ref(param));
        CHECK(param.get() == 0);
        param.set(1);
        CHECK(param.get() == 1);
        done.wait();
        CHECK(param.get() == 1);
    }

    {
        param_t param(0);
        coro_result_t<void> done = coro_t::spawn([&] (const param_t &p) { CHECK(p.get() == 1); }, std::ref(param));
        CHECK(param.get() == 0);
        param.set(1);
        CHECK(param.get() == 1);
        done.wait();
        CHECK(param.get() == 1);
    }

    {
        param_t param(0);
        coro_t::spawn_now([&] (param_t &p) {
                CHECK(p.get() == 0);
                p.set(1);
            }, std::ref(param));
        CHECK(param.get() == 1);
        coro_result_t<void> done = coro_t::spawn([&] (param_t &p) {
                CHECK(p.get() == 2);
                p.set(3);
            }, std::ref(param));
        CHECK(param.get() == 1);
        param.set(2);
        CHECK(param.get() == 2);
        done.wait();
        CHECK(param.get() == 3);
    }

    {
        // Can't call an r-value reference with a normal reference
        // param_t param(0);
        // coro_result_t<void> done = coro_t::spawn([&] (param_t &&p) { CHECK(p.get() == 1); }, std::ref(param));
        // done.wait();
    }

    // Source is an r-value reference
    {
        // Should move the object all the way to the coroutine
        param_t param(0);
        coro_result_t<void> done = coro_t::spawn([&] (param_t p) { CHECK(p.get() == 0); }, std::move(param));
        CHECK(param.get() == -1);
        param.set(1);
        CHECK(param.get() == 1);
        done.wait();
        CHECK(param.get() == 1);
    }

    {
        // Should move the param to the coroutine
        param_t param(0);
        coro_result_t<void> done = coro_t::spawn([&] (const param_t &p) { CHECK(p.get() == 0); }, std::move(param));
        CHECK(param.get() == -1);
        param.set(1);
        CHECK(param.get() == 1);
        done.wait();
        CHECK(param.get() == 1);
    }

    {
        // Can't pass an r-value reference to a non-const reference I guess
        // param_t param(0);
        // coro_result_t<void> done = coro_t::spawn([&] (param_t &p) {
        //         CHECK(p.get() == 1);
        //         p.set(2);
        //     }, std::move(param));
        // CHECK(param.get() == 0);
        // param.set(1);
        // CHECK(param.get() == 1);
        // done.wait();
        // CHECK(param.get() == 2);
    }

    {
        // Should move the object to the coroutine
        param_t param(0);
        coro_result_t<void> done = coro_t::spawn([&] (param_t &&p) {
                CHECK(p.get() == 0);
                param_t movee(std::move(p));
                CHECK(movee.get() == 0);
                CHECK(p.get() == -1);
            }, std::move(param));
        CHECK(param.get() == -1);
        param.set(1);
        CHECK(param.get() == 1);
        done.wait();
        CHECK(param.get() == 1);
    }
}
