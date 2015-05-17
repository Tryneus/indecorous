#ifndef CORO_CORO_TIMER_HPP_
#define CORO_CORO_TIMER_HPP_

#include "coro/wait_object.hpp"
#include "coro/queue.hpp"
#include "coro/coro.hpp"

class coro_timer_t : public wait_object_t, public wait_callback_t {
public:
  coro_timer_t();
  ~coro_timer_t();

  void set(uint32_t timeoutMs, bool autoReset, bool wakeAll);
  void reset();
  bool stop();
  void wait();

private:
  void addWait(wait_callback_t* cb);
  void removeWait(wait_callback_t* cb);

  friend class CoroScheduler;
  void wait_callback(wait_result_t result);

  bool m_wakeAll;
  bool m_autoReset;
  bool m_running;
  uint32_t m_timeout;

  IntrusiveQueue<wait_callback_t> m_waiters;
};

#endif
