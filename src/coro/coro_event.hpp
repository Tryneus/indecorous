#ifndef CORO_CORO_EVENT_HPP_
#define CORO_CORO_EVENT_HPP_

#include "coro/wait_object.hpp"
#include "coro/queue.hpp"
#include "coro/coro.hpp"

class coro_event_t : public wait_object_t {
public:
  coro_event_t(bool autoReset, bool m_wakeAll);
  ~coro_event_t();

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

#endif
