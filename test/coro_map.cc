#include "test.hpp"
#include "sync/coro_map.hpp"

using namespace indecorous;

SIMPLE_TEST(coro_map, simple, 4, "[sync][coro_map]") {
    std::vector<size_t> vec({ 12, 13, 14, 15 });

    coro_map(5, 10, [&] (size_t i) { debugf("%zu", i); });
    coro_map(3, [&] (size_t i) { debugf("%zu", i); });
    coro_map(vec, [&] (size_t i) { debugf("%zu", i); });

    throttled_coro_map(5, 10, [&] (size_t i) { debugf("%zu", i); }, 2);
    throttled_coro_map(3, [&] (size_t i) { debugf("%zu", i); }, 2);
    throttled_coro_map(vec, [&] (size_t i) { debugf("%zu", i); }, 2);
}

