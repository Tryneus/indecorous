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

void seed_rand() {
  uint32_t seed;
  int urand = open("/dev/urandom", O_RDONLY);
  // This could theoretically be interrupted by a signal and screw up, but this is just a unit test
  REQUIRE(read(urand, &seed, sizeof(seed)) == sizeof(seed));
  srand(seed);
}

void testCoroutine(void* param) {
  std::atomic<uint32_t>* numCoroutines = reinterpret_cast<std::atomic<uint32_t>*>(param);
  // Randomly spawn more coroutines across all threads
  for (size_t i = 0; i < numThreads; ++i) {
    if (numCoroutines->fetch_add(1) < maxCoroutines)
      coro_t::spawn_on(i, testCoroutine, param, nullptr);
    else 
      numCoroutines->fetch_sub(1);
  }
}

TEST_CASE("coro/spawn", "Test basic coroutine spawning and running") {
  // seed_rand();
  std::atomic<uint32_t> numCoroutines(0);
  CoroScheduler sched(numThreads);
  for (size_t reps = 0; reps < 20; ++reps) {
    numCoroutines = 0;
    for (size_t i = 0; i < numThreads; ++i) {
      sched.spawn(i, testCoroutine, &numCoroutines);
      ++numCoroutines;
    }
    CHECK_THROWS_AS(sched.spawn(numThreads, testCoroutine, &numCoroutines), coro_exc_t);
    CHECK_THROWS_AS(sched.spawn(-1, testCoroutine, &numCoroutines), coro_exc_t);
    sched.run();
  }
}

TEST_CASE("coro/wait", "Test basic coroutine waiting") {

}

TEST_CASE("coro/spawn_on", "Test spawning coroutines on other dispatchers") {

}

void moveCoroutine(void* param) {
  size_t target_thread = (size_t)param;
  coro_t::self()->move(target_thread);
}

void moveSpawner(void* param) {
  // Spawn n sub-coroutines that will move to the same or another thread
  size_t numCoroutines = (size_t)param;
  for (size_t i = 0; i < numCoroutines; ++i) {
    size_t target_thread = i % numThreads;
    coro_t::spawn(moveCoroutine, (void*)target_thread, nullptr);
  }
}

TEST_CASE("coro/move", "Test moving a running coroutine to another dispatcher") {
  CoroScheduler sched(numThreads);
  size_t target_thread = 3;

  // Make sure moving a single coroutine works
  sched.spawn(0, moveCoroutine, (void*)target_thread);
  sched.run();

  // Test two coroutines from the same thread to another
  sched.spawn(0, moveCoroutine, (void*)target_thread);
  sched.spawn(0, moveCoroutine, (void*)target_thread);
  sched.run();

  // Test two coroutines from different threads to a third
  sched.spawn(1, moveCoroutine, (void*)target_thread);
  sched.spawn(0, moveCoroutine, (void*)target_thread);
  sched.run();

  // Now test a bunch of them
  size_t numCoroutines = 2000;
  for (size_t i = 0; i < numThreads; ++i) {
    sched.spawn(i, moveSpawner, (void*)numCoroutines);
    ++numCoroutines;
  }
  sched.run();
}

TEST_CASE("coro/move-one-way", "Test moving a lot of coroutines from one specific dispatcher to another") {
  CoroScheduler sched(numThreads);
  size_t numCoroutines = 5000;
  for (size_t i = 0; i < 20; ++i) {
    sched.spawn(0, moveSpawner, (void*)numCoroutines);
    sched.run();
  }
}

TEST_CASE("coro/spawn-and-move", "Test moving a coroutine before it has started") {

}

TEST_CASE("coro/stress", "Test a wide combination of coroutine behaviors") {

}

