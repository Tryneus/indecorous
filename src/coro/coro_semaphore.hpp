#ifndef CORO_CORO_SEMAPHORE_HPP_
#define CORO_CORO_SEMAPHORE_HPP_

#include "coro/wait_object.hpp"
#include "coro/queue.hpp"
#include "coro/coro.hpp"

class coro_semaphore_t : public wait_object_t {
public:
  coro_semaphore_t(size_t initial, size_t max);
  ~coro_semaphore_t();

  void wait();
  void unlock(size_t count);

private:
  void addWait(wait_callback_t* cb);
  void removeWait(wait_callback_t* cb);

  const size_t m_max;
  size_t m_count;
  IntrusiveQueue<wait_callback_t> m_waiters;
};

#endif
