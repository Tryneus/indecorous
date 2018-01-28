#include "bench.hpp"

#include "common.hpp"

void timespec_subtract(struct timespec *x,
                       struct timespec *y,
                       struct timespec *result) {
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_nsec < y->tv_nsec) {
        int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
        y->tv_nsec -= 1000000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_nsec - y->tv_nsec > 1000000000) {
        int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
        y->tv_nsec += 1000000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_nsec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_nsec = x->tv_nsec - y->tv_nsec;
}

bench_timer_t::bench_timer_t(std::string test_name, size_t count) :
        m_test_name(std::move(test_name)),
        m_count(count),
        m_start_time() {
    int res = clock_gettime(CLOCK_MONOTONIC, &m_start_time);
    GUARANTEE(res == 0);
}

bench_timer_t::~bench_timer_t() {
    struct timespec end_time;
    int res = clock_gettime(CLOCK_MONOTONIC, &end_time);
    GUARANTEE(res == 0);

    struct timespec delta;
    timespec_subtract(&end_time, &m_start_time, &delta);

    float total = static_cast<float>(delta.tv_sec) +
        (static_cast<float>(delta.tv_nsec) / 1000000000.0);

    float per_item = total / static_cast<float>(m_count);
    logDebug("%s | total %.3f s | per item: %.9f s",
             m_test_name.c_str(), total, per_item);
}
