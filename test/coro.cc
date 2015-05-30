#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"
#include "rpc/handler.hpp"
#include "rpc/target.hpp"
#include "rpc/serialize_stl.hpp"
#include "sync/timer.hpp"
#include "sync/multiple_wait.hpp"

const size_t num_threads = 4;

using namespace indecorous;

class test_callback_t {
public:
    static void call(std::string a, std::string b, int value) {
        printf("Got called with %s, %s, %d\n", a.c_str(), b.c_str(), value);
    }
};

class wait_test_callback_t {
public:
    static void call() {
        single_timer_t timer_a;
        periodic_timer_t timer_b;
        timer_a.start(10);
        timer_b.start(100);
        wait_any(&timer_a, &timer_b);
    }
};

id_generator_t<handler_id_t> handler_id_gen;

template<>
const handler_id_t unique_handler_t<test_callback_t>::unique_id = handler_id_gen.next();

TEST_CASE("coro/spawn", "Test basic coroutine spawning and running") {
    scheduler_t sched(num_threads);
    handler_t<test_callback_t> test_handler(sched.message_hub());
    std::unordered_set<target_id_t> threads = sched.all_threads();
    // Perform an RPC into each thread to start
    for (auto const &id : threads) {
        sched.message_hub()->target(id)->noreply_call<test_callback_t>("foo", "bar", 1);
    }
    sched.run(shutdown_policy_t::Eager);
}

TEST_CASE("coro/wait", "Test basic coroutine waiting") {

}
