#ifndef SYNC_EVENT_HPP_
#define SYNC_EVENT_HPP_

#include "sync/wait_object.hpp"
#include "containers/queue.hpp"
#include "coro/coro.hpp"

namespace indecorous {

class event_t : public wait_object_t {
public:
  event_t(bool autoReset, bool m_wakeAll);
  ~event_t();

  bool triggered() const;
  bool set();
  bool reset();
  void wait();

protected:
  void addWait(wait_callback_t* cb);
  void removeWait(wait_callback_t* cb);

private:
  bool m_autoReset;
  bool m_wakeAll;
  bool m_triggered;
  IntrusiveQueue<wait_callback_t> m_waiters;
};

} // namespace indecorous

#endif
