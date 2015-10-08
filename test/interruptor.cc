#include "test.hpp"

#include "coro/coro.hpp"
#include "errors.hpp"
#include "sync/interruptor.hpp"
#include "sync/event.hpp"
#include "sync/multiple_wait.hpp"
#include "sync/timer.hpp"

using namespace indecorous;

SIMPLE_TEST(interruptor, basic, 4, "[sync][interruptor]") {
    debugf("basic interruptor test started, coro_t::self(): %p", coro_t::self());
    single_timer_t timer(5);
    interruptor_t interruptor(&timer);
    event_t outer_event;

    debugf("launching coro_any coroutine");
    auto coro_any = coro_t::spawn([&] {
            try {
                event_t inner_event;
                wait_any(outer_event, inner_event);
                FAIL("wait was not interrupted");
            } catch (const wait_interrupted_exc_t &) { debugf("coro_any coroutine interrupted"); }
        });

    debugf("launching coro_all coroutine");
    auto coro_all = coro_t::spawn([&] {
            try {
                wait_all(outer_event, coro_any);
                FAIL("wait was not interrupted");
            } catch (const wait_interrupted_exc_t &) { debugf("coro_all coroutine interrupted"); }
        });

    try {
        wait_all(outer_event, coro_all, coro_any);
        FAIL("wait was not interrupted");
    } catch (const wait_interrupted_exc_t &) { debugf("main coroutine interrupted"); }

    coro_t::clear_interruptors();
    coro_any.wait();
    coro_all.wait();
    debugf("returning");
}

SIMPLE_TEST(interruptor, preemptive, 4, "[sync][interruptor]") {
    event_t preempt;
    preempt.set();
    interruptor_t interruptor(&preempt);
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
    debugf("preemptive test, outer_event(%p), coro_any(%p)", &outer_event, &coro_any);

    try {
        wait_all(outer_event, coro_all, coro_any);
        FAIL("wait was not interrupted");
    } catch (const wait_interrupted_exc_t &) { }

    coro_t::clear_interruptors();
    coro_any.wait();
    coro_all.wait();
    debugf("preemptive test returning");
}
