#ifndef CORO_CORO_SCHED_HPP_
#define CORO_CORO_SCHED_HPP_
#include <poll.h>
#include <stdint.h>
#include <atomic>
#include <map>
#include <pthread.h>

#include "coro/wait_object.hpp"

namespace indecorous {

class CoroDispatcher;

class CoroScheduler {
public:
  CoroScheduler(size_t numThreads);
  ~CoroScheduler();

  // TODO: add a way to schedule coroutines before `run`

  // This function will not return until all coroutines (and children thereof) return
  void run();

  // This function will return the current thread id, or -1 if run from another thread
  static size_t getThreadId();

  class Thread {
  public:
    static void addTimer(wait_callback_t* timer, uint32_t timeout);
    static void updateTimer(wait_callback_t* timer, uint32_t timeout);
    static void removeTimer(wait_callback_t* timer);

    static bool addFileWait(int fd, int eventMask, wait_callback_t* waiter);
    static bool removeFileWait(int fd, int eventMask, wait_callback_t* waiter);

    Thread(CoroScheduler* parent, size_t threadId, pthread_barrier_t *barrier);

  private:
    friend class CoroScheduler;

    void shutdown();
    static Thread& getInstance();

    static void* threadHook(void* param);
    void threadMain();

    void doWait();
    void buildPollSet(struct pollfd* pollArray);
    void notifyWaiters(size_t size, struct pollfd* pollArray);
    void notifyWaitersForEvent(struct pollfd* pollEvent, int eventMask);

    int getWaitTimeout();
    static uint64_t getEndTime(uint32_t timeout);

    friend class coro_timer_t;

    struct file_wait_info_t {
      int fd;
      int event;

      // TODO: This is probably the wrong way to do whatever this is supposed to do
      bool operator == (const file_wait_info_t& other) const {
        return (fd == other.fd) && (event == other.event);
      }

      bool operator < (const file_wait_info_t& other) const {
        return (fd < other.fd) || (event < other.event);
      }
    };

    CoroScheduler* m_parent;
    bool m_shutdown;
    pthread_t m_pthread;
    pthread_barrier_t *m_barrier;
    CoroDispatcher* m_dispatcher;

    // TODO: make these use intrusive queues or something for performance?
    std::multimap<file_wait_info_t, wait_callback_t*> m_fileWaiters;
    std::multimap<uint64_t, wait_callback_t*> m_timerWaiters;

    static __thread Thread* s_instance;
    static __thread size_t s_threadId;
  };

private:
  friend class CoroDispatcher;
  friend class coro_t;

  static CoroDispatcher& getDispatcher(size_t threadId);

  // TODO: don't use atomic here?
  std::atomic<bool> m_running;
  std::atomic<int32_t> m_active_contexts; // semaphore, maybe?
  pthread_barrier_t m_barrier;
  Thread** m_threads;
  size_t m_numThreads;
  static CoroScheduler* s_instance;
};

} // namespace indecorous

#endif
