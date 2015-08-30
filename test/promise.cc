#include "catch.hpp"

#include "coro/sched.hpp"
#include "errors.hpp"
#include "sync/promise.hpp"
#include "rpc/handler.hpp"

const size_t num_threads = 8;

using namespace indecorous;

struct callable_struct_t {
    static int static_fn() { return 1; }
    int member_fn() { return 2; }
    int operator()() { return 3; }
};

int callable_int() { return 4; }

void test_callable_promise() {
    callable_struct_t s;
    promise_t<void> p;
    future_t<void> f = p.get_future();
    future_t<int> future_a = f.then(callable_struct_t::static_fn);
    future_t<int> future_b = f.then(std::bind(&callable_struct_t::member_fn, &s));
    future_t<int> future_c = f.then(s);
    future_t<int> future_d = f.then(callable_int);
    future_t<int> future_e = f.then([] { return 5; });

    REQUIRE(!p.fulfilled());
    REQUIRE(!f.has());
    REQUIRE(!future_a.has());
    REQUIRE(!future_b.has());
    REQUIRE(!future_c.has());
    REQUIRE(!future_d.has());
    REQUIRE(!future_e.has());

    p.fulfill();

    REQUIRE(p.fulfilled());
    REQUIRE(f.has());
    REQUIRE(future_a.has());
    REQUIRE(future_b.has());
    REQUIRE(future_c.has());
    REQUIRE(future_d.has());
    REQUIRE(future_e.has());

    REQUIRE(future_a.get() == 1);
    REQUIRE(future_a.copy() == 1);
    REQUIRE(future_a.release() == 1);

    REQUIRE(future_b.get() == 2);
    REQUIRE(future_b.copy() == 2);
    REQUIRE(future_b.release() == 2);

    REQUIRE(future_c.get() == 3);
    REQUIRE(future_c.copy() == 3);
    REQUIRE(future_c.release() == 3);

    REQUIRE(future_d.get() == 4);
    REQUIRE(future_d.copy() == 4);
    REQUIRE(future_d.release() == 4);

    REQUIRE(future_e.get() == 5);
    REQUIRE(future_e.copy() == 5);
    REQUIRE(future_e.release() == 5);

    REQUIRE_THROWS_AS(future_a.get(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_a.copy(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_a.release(), wait_object_lost_exc_t);

    REQUIRE_THROWS_AS(future_b.get(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_b.copy(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_b.release(), wait_object_lost_exc_t);

    REQUIRE_THROWS_AS(future_c.get(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_c.copy(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_c.release(), wait_object_lost_exc_t);

    REQUIRE_THROWS_AS(future_d.get(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_d.copy(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_d.release(), wait_object_lost_exc_t);

    REQUIRE_THROWS_AS(future_e.get(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_e.copy(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_e.release(), wait_object_lost_exc_t);
}

void test_nested_promise() {
    {
        promise_t<void> p_a;
        future_t<void> future_a = p_a.get_future();
        promise_t<char> p_b;
        future_t<char> future_b = future_a.then([&] () { return p_b.get_future(); });
        promise_t<void> p_c;
        future_t<void> future_c = future_a.then([&] () { return p_c.get_future(); });
        promise_t<bool> p_d;
        future_t<bool> future_d = future_b.then([&] (char) { return p_d.get_future(); });
        promise_t<void> p_e;
        future_t<void> future_e = future_b.then([&] (char) { return p_e.get_future(); });

        REQUIRE(!p_a.fulfilled());
        REQUIRE(!future_a.has());
        REQUIRE(!p_b.fulfilled());
        REQUIRE(!future_b.has());
        REQUIRE(!p_c.fulfilled());
        REQUIRE(!future_c.has());
        REQUIRE(!p_d.fulfilled());
        REQUIRE(!future_d.has());
        REQUIRE(!p_e.fulfilled());
        REQUIRE(!future_e.has());

        p_a.fulfill();

        REQUIRE(p_a.fulfilled());
        REQUIRE(future_a.has());
        REQUIRE(!p_b.fulfilled());
        REQUIRE(!future_b.has());
        REQUIRE(!p_c.fulfilled());
        REQUIRE(!future_c.has());

        p_b.fulfill('b');

        REQUIRE(!p_c.fulfilled());
        REQUIRE(!future_c.has());
        REQUIRE(!p_d.fulfilled());
        REQUIRE(!future_d.has());
        REQUIRE(!p_e.fulfilled());
        REQUIRE(!future_e.has());
        REQUIRE(p_b.fulfilled());
        REQUIRE(future_b.has());
        REQUIRE(future_b.get() == 'b');
        REQUIRE(future_b.copy() == 'b');
        REQUIRE(future_b.release() == 'b');

        p_c.fulfill();

        REQUIRE(!p_d.fulfilled());
        REQUIRE(!future_d.has());
        REQUIRE(!p_e.fulfilled());
        REQUIRE(!future_e.has());
        REQUIRE(p_c.fulfilled());
        REQUIRE(future_c.has());

        p_d.fulfill(true);

        REQUIRE(!p_e.fulfilled());
        REQUIRE(!future_e.has());
        REQUIRE(p_d.fulfilled());
        REQUIRE(future_d.has());
        REQUIRE(future_d.get());
        REQUIRE(future_d.copy());
        REQUIRE(future_d.release());

        p_e.fulfill();

        REQUIRE(p_e.fulfilled());
        REQUIRE(future_e.has());

        REQUIRE_THROWS_AS(future_b.get(), wait_object_lost_exc_t);
        REQUIRE_THROWS_AS(future_b.copy(), wait_object_lost_exc_t);
        REQUIRE_THROWS_AS(future_b.release(), wait_object_lost_exc_t);

        REQUIRE_THROWS_AS(future_d.get(), wait_object_lost_exc_t);
        REQUIRE_THROWS_AS(future_d.copy(), wait_object_lost_exc_t);
        REQUIRE_THROWS_AS(future_d.release(), wait_object_lost_exc_t);
    }

    {
        promise_t<void> p_a;
        future_t<void> future_a = p_a.get_future();
        promise_t<char> p_b;
        future_t<char> future_b = future_a.then([&] () { return p_b.get_future(); });
        promise_t<void> p_c;
        future_t<void> future_c = future_a.then([&] () { return p_c.get_future(); });
        promise_t<bool> p_d;
        future_t<bool> future_d = future_b.then([&] (char) { return p_d.get_future(); });
        promise_t<void> p_e;
        future_t<void> future_e = future_b.then([&] (char) { return p_e.get_future(); });

        REQUIRE(!p_a.fulfilled());
        REQUIRE(!future_a.has());
        REQUIRE(!p_b.fulfilled());
        REQUIRE(!future_b.has());
        REQUIRE(!p_c.fulfilled());
        REQUIRE(!future_c.has());
        REQUIRE(!p_d.fulfilled());
        REQUIRE(!future_d.has());
        REQUIRE(!p_e.fulfilled());
        REQUIRE(!future_e.has());

        p_e.fulfill();
        REQUIRE(p_e.fulfilled());
        REQUIRE(!future_e.has());
        p_d.fulfill(false);
        REQUIRE(p_d.fulfilled());
        REQUIRE(!future_d.has());
        REQUIRE(!future_e.has());
        p_c.fulfill();
        REQUIRE(p_c.fulfilled());
        REQUIRE(!future_c.has());
        REQUIRE(!future_d.has());
        REQUIRE(!future_e.has());
        p_b.fulfill('a');
        REQUIRE(p_b.fulfilled());
        REQUIRE(!future_b.has());
        REQUIRE(!future_c.has());
        REQUIRE(!future_d.has());
        REQUIRE(!future_e.has());
        p_a.fulfill();
        REQUIRE(p_a.fulfilled());

        REQUIRE(future_a.has());
        REQUIRE(future_b.has());
        REQUIRE(future_c.has());
        REQUIRE(future_d.has());
        REQUIRE(future_e.has());

        REQUIRE(future_b.get() == 'a');
        REQUIRE(future_b.copy() == 'a');
        REQUIRE(future_b.release() == 'a');

        REQUIRE(!future_d.get());
        REQUIRE(!future_d.copy());
        REQUIRE(!future_d.release());

        REQUIRE_THROWS_AS(future_b.get(), wait_object_lost_exc_t);
        REQUIRE_THROWS_AS(future_b.copy(), wait_object_lost_exc_t);
        REQUIRE_THROWS_AS(future_b.release(), wait_object_lost_exc_t);

        REQUIRE_THROWS_AS(future_d.get(), wait_object_lost_exc_t);
        REQUIRE_THROWS_AS(future_d.copy(), wait_object_lost_exc_t);
        REQUIRE_THROWS_AS(future_d.release(), wait_object_lost_exc_t);
    }
}

void test_void_promise() {
    promise_t<void> p;
    future_t<void> future_a = p.get_future();
    future_t<void> future_b = future_a.then([&] () { });
    future_t<uint64_t> future_c = future_a.then([&] () -> uint64_t { return 3; });
    future_t<char> future_d = future_c.then([&] (uint64_t) { return 'd'; });
    REQUIRE(!p.fulfilled());
    REQUIRE(!future_a.has());
    REQUIRE(!future_b.has());
    REQUIRE(!future_c.has());
    REQUIRE(!future_d.has());

    p.fulfill();

    REQUIRE(p.fulfilled());
    REQUIRE(future_a.has());
    REQUIRE(future_b.has());
    REQUIRE(future_c.has());
    REQUIRE(future_d.has());

    future_t<void> future_e = future_c.then([&] (uint64_t) { });
    future_t<size_t> future_f = future_d.then([&] (const char &) -> size_t { return 6; });

    REQUIRE(future_e.has());
    REQUIRE(future_f.has());

    REQUIRE(future_c.get() == 3);
    REQUIRE(future_c.copy() == 3);
    REQUIRE(future_c.release() == 3);

    REQUIRE(future_d.get() == 'd');
    REQUIRE(future_d.copy() == 'd');
    REQUIRE(future_d.release() == 'd');

    REQUIRE(future_f.get() == 6);
    REQUIRE(future_f.copy() == 6);
    REQUIRE(future_f.release() == 6);

    REQUIRE_THROWS_AS(future_c.get(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_c.copy(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_c.release(), wait_object_lost_exc_t);

    REQUIRE_THROWS_AS(future_d.get(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_d.copy(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_d.release(), wait_object_lost_exc_t);

    REQUIRE_THROWS_AS(future_f.get(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_f.copy(), wait_object_lost_exc_t);
    REQUIRE_THROWS_AS(future_f.release(), wait_object_lost_exc_t);
}

template <typename T>
void test_copy_promise() {
    promise_t<T> p;
    future_t<T> future_a = p.get_future();
    future_t<void> future_b = future_a.then([&] (T) { });
    future_t<uint64_t> future_c = future_a.then([&] (const T &) -> uint64_t { return 3; });
    future_t<char> future_d = future_c.then([&] (uint64_t) { return 'd'; });

    REQUIRE(!p.fulfilled());
    REQUIRE(!future_a.has());
    REQUIRE(!future_b.has());
    REQUIRE(!future_c.has());
    REQUIRE(!future_d.has());

    p.fulfill(T::make(5));

    REQUIRE(p.fulfilled());
    REQUIRE(future_a.has());
    REQUIRE(future_b.has());
    REQUIRE(future_c.has());
    REQUIRE(future_d.has());

    REQUIRE(future_a.get().value() == 5);
    REQUIRE(future_a.copy().value() == 5);

    future_t<size_t> future_e = future_d.then([&] (const char &) -> size_t { return 6; });
    REQUIRE(future_e.has());

    REQUIRE(future_c.get() == 3);
    REQUIRE(future_d.get() == 'd');
    REQUIRE(future_e.get() == 6);
}

template <typename T>
void test_move_promise() {
    promise_t<T> p;
    future_t<T> future_a = p.get_future();
    future_t<void> future_b = future_a.then([&] (const T &) { });
    future_t<uint64_t> future_c = future_a.then_release([&] (T) -> uint64_t { return 3; });
    future_t<char> future_d = future_c.then([&] (uint64_t) { return 'd'; });

    REQUIRE(!p.fulfilled());
    REQUIRE(!future_a.has());
    REQUIRE(!future_b.has());
    REQUIRE(!future_c.has());
    REQUIRE(!future_d.has());

    p.fulfill(T::make(5));

    // TODO: check other promises
}

class non_movable_t {
public:
    non_movable_t(const non_movable_t &other) : m_value(other.m_value) { }
    ~non_movable_t() { }

    static non_movable_t make(uint64_t _value) { return non_movable_t(_value); }
    uint64_t value() const { return m_value; }
private:
    non_movable_t(uint64_t _value) : m_value(_value) { }
    uint64_t m_value;
};

class non_copyable_t {
public:
    non_copyable_t(non_copyable_t &&other) : m_value(std::move(other.m_value)) { }
    ~non_copyable_t() { }

    static non_copyable_t make(uint64_t _value) { return non_copyable_t(_value); }
    uint64_t value() const { return m_value; }
private:
    non_copyable_t(uint64_t _value) : m_value(_value) { }
    non_copyable_t(const non_copyable_t &) = delete;
    uint64_t m_value;
};

class movable_copyable_t {
public:
    movable_copyable_t(movable_copyable_t &&other) : m_value(std::move(other.m_value)) { }
    movable_copyable_t(const movable_copyable_t &other) : m_value(other.m_value) { }
    ~movable_copyable_t() { }

    static movable_copyable_t make(uint64_t _value) { return movable_copyable_t(_value); }
    uint64_t value() const { return m_value; }
private:
    movable_copyable_t(uint64_t _value) : m_value(_value) { }
    uint64_t m_value;
};

struct promise_test_t {
    DECLARE_STATIC_RPC(non_movable)() -> void;
    DECLARE_STATIC_RPC(non_copyable)() -> void;
    DECLARE_STATIC_RPC(movable_copyable)() -> void;
    DECLARE_STATIC_RPC(common)() -> void;
};

IMPL_STATIC_RPC(promise_test_t::non_movable)() -> void {
    test_void_promise();
    test_nested_promise();
    test_copy_promise<non_movable_t>();
}

IMPL_STATIC_RPC(promise_test_t::non_copyable)() -> void {
    test_move_promise<non_copyable_t>();
};

IMPL_STATIC_RPC(promise_test_t::movable_copyable)() -> void {
    test_copy_promise<movable_copyable_t>();
    test_move_promise<movable_copyable_t>();
}

IMPL_STATIC_RPC(promise_test_t::common)() -> void {
    test_void_promise();
    test_nested_promise();
    test_callable_promise();
}

TEST_CASE("promise", "[sync][promise]") {
    scheduler_t sched(1, shutdown_policy_t::Eager);
    sched.broadcast_local<promise_test_t::common>();
    sched.broadcast_local<promise_test_t::non_movable>();
    sched.broadcast_local<promise_test_t::non_copyable>();
    sched.broadcast_local<promise_test_t::movable_copyable>();
    sched.run();
}
