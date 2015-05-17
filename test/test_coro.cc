#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <atomic>
#include <unistd.h>

#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "coro/errors.hpp"

const uint32_t maxCoroutines = 20000;
const size_t numThreads = 4;

TEST_CASE("coro/spawn", "Test basic coroutine spawning and running") {
    // TODO: reimplement these
}

TEST_CASE("coro/wait", "Test basic coroutine waiting") {

}
