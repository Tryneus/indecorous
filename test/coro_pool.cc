#include "test.hpp"
#include "sync/coro_pool.hpp"
#include "sync/timer.hpp"

using namespace indecorous;

SIMPLE_TEST(coro_pool, simple, 4, "[sync][coro_pool]") {
    coro_pool_t pool(3);
    size_t in = 100;
    size_t out = 0;
    pool.run(
        [&] {
            if (in == 0) {
                throw wait_interrupted_exc_t();
            }
            --in;
        },
        [&] {
            single_timer_t(1).wait();
            ++out;
        });
    REQUIRE(in == 0);
    REQUIRE(out == 100);
}

