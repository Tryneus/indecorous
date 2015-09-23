#include "test.hpp"
#include "sync/coro_map.hpp"

using namespace indecorous;

auto make_map_fn(std::set<size_t> &out) {
    out.clear();
    return [&] (size_t i) {
            auto res = out.insert(i);
            CHECK(res.second);
        };
}

SIMPLE_TEST(coro_map, simple, 4, "[sync][coro_map]") {
    std::set<size_t> params({ 12, 13, 14, 15 });
    std::set<size_t> actual;

    coro_map(5, 10, make_map_fn(actual));
    CHECK(actual == std::set<size_t>({5, 6, 7, 8, 9}));

    throttled_coro_map(5, 10, make_map_fn(actual), 2);
    CHECK(actual == std::set<size_t>({5, 6, 7, 8, 9}));

    coro_map(3, make_map_fn(actual));
    CHECK(actual == std::set<size_t>({0, 1, 2}));

    throttled_coro_map(3, make_map_fn(actual), 2);
    CHECK(actual == std::set<size_t>({0, 1, 2}));

    coro_map(params, make_map_fn(actual));
    CHECK(actual == params);

    throttled_coro_map(params, make_map_fn(actual), 2);
    CHECK(actual == params);
}

