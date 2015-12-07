#ifndef CROSS_THREAD_HOME_THREADED_HPP_
#define CROSS_THREAD_HOME_THREADED_HPP_

#include <cstddef>

#include "rpc/id.hpp"

namespace indecorous {

class home_threaded_t {
public:
    home_threaded_t();
    virtual ~home_threaded_t();

    target_id_t home_thread() const;
    void assert_thread() const;
private:
    target_id_t m_home_thread;
};

} // namespace indecorous

#endif // SYNC_HOME_THREADED_HPP_
