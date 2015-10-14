#ifndef CROSS_THREAD_HOME_THREADED_HPP_
#define CROSS_THREAD_HOME_THREADED_HPP_

#include <cstddef>

namespace indecorous {

class home_threaded_t {
public:
    home_threaded_t();
    virtual ~home_threaded_t();

    size_t home_thread() const;
    void assert_thread() const;
private:
    size_t m_home_thread;
};

} // namespace indecorous

#endif // SYNC_HOME_THREADED_HPP_
