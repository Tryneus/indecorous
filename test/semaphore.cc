#include "test.hpp"
#include "sync/semaphore.hpp"

using namespace indecorous;

SIMPLE_TEST(semaphore, simple, 4, "[sync][semaphore]") {
    semaphore_t sem(10);
    REQUIRE(sem.capacity() == 10);
    REQUIRE(sem.available() == 10);

    // First acq should be immediately available
    semaphore_acq_t acq1 = sem.start_acq(6);
    REQUIRE(acq1.owned() == 6);
    REQUIRE(acq1.pending() == 0);
    REQUIRE(sem.capacity() == 10);
    REQUIRE(sem.available() == 4);

    // Second acq should be pending
    semaphore_acq_t acq2 = sem.start_acq(6);
    REQUIRE(acq2.owned() == 0);
    REQUIRE(acq2.pending() == 6);
    REQUIRE(sem.capacity() == 10);
    REQUIRE(sem.available() == 4); // TODO should this be 0?  -2?

    acq1.release(3);
    REQUIRE(acq1.owned() == 3);
    REQUIRE(acq1.pending() == 0);
    REQUIRE(acq2.owned() == 6);
    REQUIRE(acq2.pending() == 0);
    REQUIRE(sem.capacity() == 10);
    REQUIRE(sem.available() == 1);

    acq1.release(3);
    acq2.release(6);
    REQUIRE(acq1.owned() == 0);
    REQUIRE(acq1.pending() == 0);
    REQUIRE(acq2.owned() == 0);
    REQUIRE(acq2.pending() == 0);

    REQUIRE(sem.capacity() == 10);
    REQUIRE(sem.available() == 10);
}

