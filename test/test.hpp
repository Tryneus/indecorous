#ifndef TEST_HPP_
#define TEST_HPP_

#include "coro/sched.hpp"
#include "rpc/handler.hpp"
#include "catch.hpp"

// Note that catch assertions are not thread-safe, so we only run one test at a time
#define SIMPLE_TEST(suite, name, repeat, tags) \
    struct suite ## _ ## name ## _test_t { \
        DECLARE_STATIC_RPC(name)() -> void; \
    }; \
    TEST_CASE(#suite "/" #name, tags) { \
        const size_t count = (repeat); \
        indecorous::scheduler_t sched(1, indecorous::shutdown_policy_t::Eager); \
        for (size_t i = 0; i < count; ++i) { \
            sched.broadcast_local<suite ## _ ## name ## _test_t::name>(); \
            sched.run(); \
        } \
    } \
    IMPL_STATIC_RPC(suite ## _ ## name ## _test_t::name)() -> void

#endif // TEST_HPP_
