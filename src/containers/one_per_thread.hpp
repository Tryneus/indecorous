#ifndef CONTAINERS_ONE_PER_THREAD_HPP_
#define CONTAINERS_ONE_PER_THREAD_HPP_

#include <unordered_map>

#include "coro/thread.hpp"
#include "rpc/hub.hpp"
#include "rpc/id.hpp"
#include "rpc/target.hpp"

namespace indecorous {

template <class T>
class one_per_thread_t {
public:
    one_per_thread_t(T initial_value) :
            m_thread_data() {
        for (auto &&t : thread_t::self()->hub()->local_targets()) {
            m_thread_data[t->id()] = initial_value;
        }
    }

    T &get() {
        return m_thread_data[thread_t::self()->target()->id()];
    }

    const T &get() const {
        return m_thread_data[thread_t::self()->target()->id()];
    }

private:
    std::unordered_map<target_id_t, T> m_thread_data;
};

} // namespace indecorous

#endif // CONTAINERS_ONE_PER_THREAD_HPP_
