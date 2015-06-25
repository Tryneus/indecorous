#include "catch.hpp"

#include "coro/sched.hpp"
#include "errors.hpp"
#include "sync/promise.hpp"
#include "rpc/hub_data.hpp"

const size_t num_threads = 8;

using namespace indecorous;

void test_void_promise() {
    promise_t<void> p;
    future_t<void> future_a = p.get_future();
    future_t<void> future_b = future_a.then([&] () { });
    future_t<uint64_t> future_c = future_a.then([&] () -> uint64_t { return 3; });
    future_t<char> future_d = future_c.then([&] (uint64_t) { return 'd'; });
    // TODO: check promise states
    p.fulfill();
    // TODO: check other promises
}

template <typename T>
void test_copy_promise() {
    promise_t<T> p;
    future_t<T> future_a = p.get_future();
    future_t<void> future_b = future_a.then([&] (T) { });
    future_t<uint64_t> future_c = future_a.then([&] (const T &) -> uint64_t { return 3; });
    future_t<char> future_d = future_c.then([&] (uint64_t) { return 'd'; });
    // TODO: check promise states
    p.fulfill(T::make(5));
    // TODO: check other promises
}

template <typename T>
void test_move_promise() {
    promise_t<T> p;
    future_t<T> future_a = p.get_future();
    future_t<void> future_b = future_a.then([&] (const T &) { });
    future_t<uint64_t> future_c = future_a.then_release([&] (T) -> uint64_t { return 3; });
    future_t<char> future_d = future_c.then([&] (uint64_t) { return 'd'; });
    // TODO: check promise states
    p.fulfill(T::make(5));
    // TODO: check other promises
}

class non_movable_t {
public:
    non_movable_t(const non_movable_t &other) : m_value(other.m_value) { }
    ~non_movable_t() { }

    static non_movable_t make(uint64_t value) { return non_movable_t(value); }
private:
    non_movable_t(uint64_t value) : m_value(value) { }
    uint64_t m_value;
};

struct non_movable_test_t : public handler_t<non_movable_test_t> {
    static void call() {
        test_void_promise();
        test_copy_promise<non_movable_t>();
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

struct non_copyable_test_t : public handler_t<non_copyable_test_t> {
    static void call() {
        test_move_promise<non_copyable_t>();
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

struct movable_copyable_test_t : public handler_t<movable_copyable_test_t> {
    static void call() {
        test_copy_promise<movable_copyable_t>();
        test_move_promise<movable_copyable_t>();
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
