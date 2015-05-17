#ifndef CORO_CORO_HPP_
#define CORO_CORO_HPP_
#include <atomic>
#include <ucontext.h>
#include <cassert>

#include "coro/arena.hpp"
#include "coro/queue.hpp"
#include "coro/wait_object.hpp"

class CoroDispatcher;
class coro_event_t;

// Pointers to contexts can be used by other modules, but they should not be changed at all
//  The QueueNode part *may* be changed if it is certain that the context is not already enqueued to run
//  otherwise an assert will fail
class coro_t : public wait_callback_t, public QueueNode<coro_t>
{
public:
  // Get the running coroutine
  static coro_t* self();

  // Give up execution indefinitely (until notified)
  static void wait();

  // Wait temporarily and be rescheduled
  static void yield();

  // Queue up another coroutine to be run
  static void notify(coro_t* coro);

  // Queue up another coroutine to be run on another thread
  static void notify_on(size_t threadId, coro_t* coro);

  // Create a coroutine and put it on the queue to run
  static void spawn(void(*fn)(void*), void* p, coro_event_t* done_event);
  static void spawn_now(void(*fn)(void*), void* p, coro_event_t* done_event);

  // Create a context on another thread (redundant with move?)
  static void spawn_on(size_t threadId, void(*fn)(void*), void* p, coro_event_t* done_event);

  // Run a blocking coroutine on a separate thread (to avoid blocking other corouines)
  static void spawn_blocking(void(*fn)(void*), void* p, coro_event_t* done_event);

   // Move a context onto another thread
  void move(size_t threadId);

private:
  friend class CoroScheduler;
  friend class CoroDispatcher;
  friend class Arena<coro_t>; // To allow instantiation of this class

  coro_t(void(*fn)(void*),
         void* param,
         CoroDispatcher *dispatcher,
         coro_event_t* done_event);
  ~coro_t();

  // TODO: RSI: should this be able to return?
  [[noreturn]] static void hook(void* instance);
  void begin();
  void swap();

  void wait_callback(wait_result_t result);

  static const size_t s_stackSize = 65535; // TODO: This is probably way too small

  void* m_param;
  void (*m_fn)(void*);
  ucontext_t m_context;
  char m_stack[s_stackSize];
  CoroDispatcher* m_dispatcher;
  int m_valgrindStackId;
  coro_event_t *m_done_event;
  wait_result_t m_waitResult;
};

// Not thread-safe, exactly one CoroDispatcher per thread
class CoroDispatcher
{
public:
  CoroDispatcher(std::atomic<int32_t>* active_contexts);
  ~CoroDispatcher();

  // Returns the number of outstanding coroutines
  uint32_t run();

private:
  friend class CoroScheduler;
  friend class coro_t;
  static CoroDispatcher& getInstance();

  void moveContext();
  void releaseContext();
  void pushForeignContext(coro_t* context);
  void pullForeignRunQueue();

  static __thread CoroDispatcher* s_instance;

  class foreign_queue_event_t : public wait_callback_t {
  public:
    foreign_queue_event_t();
    virtual ~foreign_queue_event_t();
    void notify();
    void wait_callback(wait_result_t result);
  private:
    int m_event;
  } m_foreign_queue_event;

  coro_t* volatile m_self;
  ucontext_t m_parentContext; // Used to store the parent context, switch to when no coroutines to run
  uint32_t m_swap_count;
  std::atomic<int32_t>* m_active_contexts;
  static uint32_t s_max_swaps_per_loop;

  // Variables used to move a context from this dispatcher to another
  coro_t* m_moveContext;
  CoroDispatcher* m_moveTarget;
  coro_t* m_releaseContext;

  Arena<coro_t> m_contextArena; // Arena used to cache context allocations

  IntrusiveQueue<coro_t> m_runQueue; // Queue of contexts to run
  MpscQueue<coro_t> m_foreignRunQueue; // Queue of contexts to add to our run queue (from other dispatchers)
};

#endif
