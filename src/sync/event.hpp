#ifndef SYNC_EVENT_HPP_
#define SYNC_EVENT_HPP_

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class event_t : public wait_object_t {
public:
    event_t();
    ~event_t();

    bool triggered() const;
    void set();
    void reset();

protected:
    void add_wait(wait_callback_t* cb);
    void remove_wait(wait_callback_t* cb);

private:
    bool m_triggered;
    intrusive_list_t<wait_callback_t> m_waiters;
};

} // namespace indecorous

#endif
