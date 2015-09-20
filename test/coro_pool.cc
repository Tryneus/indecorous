#include "test.hpp"
#include "sync/coro_pool.hpp"
#include "sync/timer.hpp"

using namespace indecorous;

SIMPLE_TEST(coro_pool, simple, 4, "[sync][coro_pool]") {
    coro_pool_t pool(3);
    size_t in = 100;
    size_t out = 0;
    pool.run(
        [&] (waitable_t *) {
            if (in == 0) {
                throw wait_interrupted_exc_t();
            }
            --in;
        },
        [&] (waitable_t *interruptor) {
            single_timer_t timeout(1);
            wait_any_interruptible(interruptor, timeout);
            ++out;
        });
    REQUIRE(in == 0);
    REQUIRE(out == 100);
}

