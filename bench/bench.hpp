#ifndef BENCH_HPP_
#define BENCH_HPP_

#include <time.h>
#include <string>

class bench_timer_t {
public:
    bench_timer_t(std::string test_name, size_t count);
    ~bench_timer_t();
private:
    const std::string m_test_name;
    const size_t m_count;
    struct timespec m_start_time;
};

#endif // BENCH_HPP_
