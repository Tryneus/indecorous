#ifndef SYNC_SEMAPHORE_HPP_
#define SYNC_SEMAPHORE_HPP_

#include "sync/wait_object.hpp"
#include "containers/queue.hpp"
#include "coro/coro.hpp"

namespace indecorous {

class semaphore_t : public wait_object_t {
public:
  semaphore_t(size_t initial, size_t max);
  ~semaphore_t();

  void wait();
  void unlock(size_t count);

private:
  void addWait(wait_callback_t* cb);
  void removeWait(wait_callback_t* cb);

  const size_t m_max;
  size_t m_count;
  IntrusiveQueue<wait_callback_t> m_waiters;
};

} // namespace indecorous

#endif
