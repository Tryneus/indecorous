#include "catch.hpp"

#include "coro/sched.hpp"
#include "errors.hpp"
#include "sync/promise.hpp"
#include "rpc/hub_data.hpp"

const size_t num_threads = 8;

using namespace indecorous;

class non_movable_t {
public:
    non_movable_t(const non_movable_t &other) : m_value(other.m_value) { }
    ~non_movable_t() { }

    static non_movable_t make(uint64_t value) { return non_movable_t(value); }
private:
    non_movable_t(uint64_t value) : m_value(value) { }
    non_movable_t(non_movable_t &&) = delete;
    uint64_t m_value;
};

class non_movable_test_t {
    static void run() {
        promise_t<non_movable_t> p;
        future_t<non_movable_t> future_base = p.get_future();
        future_t<void> future_void = future_base.then([&] (non_movable_t) { });
        future_t<uint64_t> future_int = future_base.then([&] (const non_movable_t &) -> uint64_t { return 0; });
        p.fulfill(non_movable_t::make(5));
        // ASSERTS
    }
};
IMPL_UNIQUE_HANDLER(non_movable_test_t);

class non_copyable_t {
public:
    non_copyable_t(non_copyable_t &&other) : m_value(std::move(other.m_value)) { }
    ~non_copyable_t() { }

    static non_copyable_t make(uint64_t value) { return non_copyable_t(value); }
private:
    non_copyable_t(uint64_t value) : m_value(value) { }
    non_copyable_t(const non_copyable_t &) = delete;
    uint64_t m_value;
};

class non_copyable_test_t {
    static void run() {
        promise_t<non_copyable_t> p;
        future_t<non_copyable_t> future_base = p.get_future();
        future_t<void> future_void = future_base.then_release([&] (non_copyable_t) { });
        future_t<uint64_t> future_int = future_base.then([&] (const non_copyable_t &) -> uint64_t { return 0; });
        p.fulfill(non_copyable_t::make(5));
        // ASSERTS
    }
};
IMPL_UNIQUE_HANDLER(non_copyable_test_t);

class movable_copyable_t {
public:
    movable_copyable_t(movable_copyable_t &&other) : m_value(std::move(other.m_value)) { }
    movable_copyable_t(const movable_copyable_t &other) : m_value(other.m_value) { }
    ~movable_copyable_t() { }

    static movable_copyable_t make(uint64_t value) { return movable_copyable_t(value); }
private:
    movable_copyable_t(uint64_t value) : m_value(value) { }
    uint64_t m_value;
};

class movable_copyable_test_t {
    static void run() {
        promise_t<movable_copyable_t> p;
        future_t<movable_copyable_t> future_base = p.get_future();
        future_t<void> future_void = future_base.then([&] (movable_copyable_t) { });
        future_t<uint64_t> future_int = future_base.then([&] (const movable_copyable_t &) -> uint64_t { return 0; });
        future_t<char> future_char = future_base.then_release([&] (movable_copyable_t) { return 'a'; });
        p.fulfill(movable_copyable_t::make(5));
        // ASSERTS
    }
};
IMPL_UNIQUE_HANDLER(movable_copyable_test_t);

TEST_CASE("promise", "[sync][promise]") {
    scheduler_t sched(1, shutdown_policy_t::Eager);
    sched.broadcast_local<non_movable_test_t>();
    sched.broadcast_local<non_copyable_test_t>();
    sched.broadcast_local<movable_copyable_test_t>();
    sched.run();
}
