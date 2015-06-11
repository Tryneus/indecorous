#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
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

struct spawn_callback_t : public handler_t<spawn_callback_t> {
    static void call(std::string a, std::string b, int value) {
        debugf("Got called with %s, %s, %d", a.c_str(), b.c_str(), value);
    }
};
IMPL_UNIQUE_HANDLER(spawn_callback_t);

struct wait_test_callback_t : public handler_t<wait_test_callback_t> {
    static void call() {
        single_timer_t timer_a;
        periodic_timer_t timer_b;
        timer_a.start(10);
        timer_b.start(100);
        wait_any(&timer_a, &timer_b);
    }
};
IMPL_UNIQUE_HANDLER(wait_test_callback_t);

TEST_CASE("coro/spawn", "Test basic coroutine spawning and running") {
    scheduler_t sched(num_threads);
    for (auto &&t : sched.threads()) {
        t.hub()->self_target()->call_noreply<spawn_callback_t>("foo", "bar", 1);
    }
    sched.run(shutdown_policy_t::Eager);
}

TEST_CASE("coro/wait", "Test basic coroutine waiting") {

}
