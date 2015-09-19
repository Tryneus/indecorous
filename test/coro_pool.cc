#include "coro/sched.hpp"
#include "sync/coro_pool.hpp"
#include "sync/timer.hpp"
#include "rpc/handler.hpp"
#include "catch.hpp"

using namespace indecorous;

const size_t num_threads = 1;

struct coro_pool_test_t {
    DECLARE_STATIC_RPC(simple)() -> void;
};

IMPL_STATIC_RPC(coro_pool_test_t::simple)() -> void {
    coro_pool_t pool(3);
    size_t in = 100;
    size_t out = 0;

    pool.run(
        [&] (waitable_t *) {
            debugf("producer called");
            if (in == 0) {
                throw wait_interrupted_exc_t();
            }
            return in--;
        },
        [&] (int x, waitable_t *interruptor) {
            debugf("consumer called");
            single_timer_t timeout(10);
            wait_any_interruptible(interruptor, timeout);
            debugf("%d", x);
            ++out;
        });
    REQUIRE(in == 0);
    REQUIRE(out == 100);
}

TEST_CASE("coro_pool/simple", "[sync][coro_pool]") {
    scheduler_t sched(num_threads, shutdown_policy_t::Eager);
    sched.broadcast_local<coro_pool_test_t::simple>();
    sched.run();

    sched.broadcast_local<coro_pool_test_t::simple>();
    sched.run();
}

