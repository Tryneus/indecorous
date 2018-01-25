#include "test.hpp"

#include "coro/coro.hpp"
#include "errors.hpp"
#include "sync/interruptor.hpp"
#include "sync/event.hpp"
#include "sync/multiple_wait.hpp"
#include "sync/timer.hpp"
#include "sync/drainer.hpp"

using namespace indecorous;

SIMPLE_TEST(interruptor, basic, 4, "[sync][interruptor]") {
    single_timer_t timer(5);
    interruptor_t interruptor(&timer);
    event_t outer_event;

    auto coro_any = coro_t::spawn([&] {
            try {
                event_t inner_event;
                wait_any(outer_event, inner_event);
                FAIL("wait was not interrupted");
            } catch (const wait_interrupted_exc_t &) { }
        });

    auto coro_all = coro_t::spawn([&] {
            try {
                wait_all(outer_event, coro_any);
                FAIL("wait was not interrupted");
            } catch (const wait_interrupted_exc_t &) { }
        });

    try {
        wait_all(outer_event, coro_all, coro_any);
        FAIL("wait was not interrupted");
    } catch (const wait_interrupted_exc_t &) { }

    coro_t::clear_interruptors();
    coro_any.wait();
    coro_all.wait();
}

SIMPLE_TEST(interruptor, preemptive, 4, "[sync][interruptor]") {
    event_t preempt;
    preempt.set();
    interruptor_t interruptor(&preempt);
    single_timer_t timeout(100);

    auto coro_any = coro_t::spawn([&] {
            try {
                event_t event;
                wait_any(event, timeout);
                FAIL("wait was not interrupted");
            } catch (const wait_interrupted_exc_t &) { debugf("coro_any interrupted"); }
        });

    auto coro_all = coro_t::spawn([&] {
            try {
                wait_all(coro_any, timeout);
                FAIL("wait was not interrupted");
            } catch (const wait_interrupted_exc_t &) { debugf("coro_all interrupted"); }
        });

    try {
        wait_all(coro_any, coro_all, timeout);
        FAIL("wait was not interrupted");
    } catch (const wait_interrupted_exc_t &) { debugf("parent coro interrupted"); }

    // TODO: children coroutines should subscribe to the parent's interruptor
    // which should be a persistent and constantly-updated object.  Right now
    // it appears to just take what the parent has when the child initializes,
    // which is flakey and harder to reason about.
    coro_t::clear_interruptors();
    coro_any.wait();
    coro_all.wait();
}

SIMPLE_TEST(interruptor, drainer, 4, "[sync][interruptor]") {
    drainer_t outer_drainer;
    interruptor_t interruptor(&outer_drainer);

    // Because we need the drainer to destroy before the interruptor
    drainer_t drainer(std::move(outer_drainer));

    coro_t::spawn([&] (drainer_lock_t) {
            try {
                event_t inner_event;
                inner_event.wait();
                FAIL("wait was not interrupted");
            } catch (const wait_interrupted_exc_t &) { }
        }, drainer.lock());

    coro_t::spawn([&] (drainer_lock_t) {
            try {
                event_t inner_event;
                inner_event.wait();
                FAIL("wait was not interrupted");
            } catch (const wait_interrupted_exc_t &) { }
        }, drainer.lock());
}
