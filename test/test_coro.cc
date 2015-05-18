#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <atomic>
#include <unistd.h>

#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"
#include "rpc/serialize_stl.hpp"

const size_t num_threads = 4;

using namespace indecorous;

class test_callback_t {
public:
    static void call(std::string a, std::string b, int value) {
        printf("Got called with %s, %s, %d\n", a.c_str(), b.c_str(), value);
    }
};

template<>
const handler_id_t unique_handler_t<test_callback_t>::unique_id = handler_id_t::assign();

TEST_CASE("coro/spawn", "Test basic coroutine spawning and running") {
    scheduler_t sched(num_threads);
    std::set<target_id_t> threads = sched.all_threads();
    // Perform an RPC into each thread to start
    for (auto const &id : threads) {
        sched.message_hub()->target(id)->noreply_call<test_callback_t>("foo", "bar", 1);
    }
    sched.run();
}

TEST_CASE("coro/wait", "Test basic coroutine waiting") {

}
