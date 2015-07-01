#include "bench.hpp"

#include "common.hpp"

bench_timer_t::bench_timer_t(std::string test_name, size_t count) :
        m_test_name(std::move(test_name)),
        m_count(count) {
    int res = gettimeofday(&m_start_time, nullptr);
    GUARANTEE(res == 0);
}

bench_timer_t::~bench_timer_t() {
    struct timeval end_time;
    int res = gettimeofday(&end_time, nullptr);
    GUARANTEE(res == 0);

    struct timeval delta;
    timersub(&end_time, &m_start_time, &delta);

    float total = static_cast<float>(delta.tv_sec) +
        (static_cast<float>(delta.tv_usec) / 1000000.0);

    float per_item = total / static_cast<float>(m_count);

    debugf("%s | total %.3f s | per item: %.6f s",
           m_test_name.c_str(), total, per_item);
}
