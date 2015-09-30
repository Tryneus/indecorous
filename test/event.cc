#include "test.hpp"
#include "sync/event.hpp"

using namespace indecorous;

SIMPLE_TEST(event, simple, 4, "[sync][event]") {
    event_t event;

    REQUIRE(!event.triggered());
    coro_t::spawn([&] {
            REQUIRE(!event.triggered());
            event.set();
            REQUIRE(event.triggered());
        });
    // Ordering of guarantees isn't guaranteed - event could be triggered or not
    event.wait();
    REQUIRE(event.triggered());

    event.reset();

    REQUIRE(!event.triggered());
    auto promise = coro_t::spawn([&] {
            // Ordering of guarantees isn't guaranteed - event could be triggered or not
            event.wait();
            REQUIRE(event.triggered());
        });
    REQUIRE(!event.triggered());
    event.set();
    REQUIRE(event.triggered());

    promise.wait();
}

