#ifndef CORO_SYNC_HPP_
#define CORO_SYNC_HPP_
#include <set>

#include "coro/wait_object.hpp"
#include "coro/queue.hpp"
#include "coro/arena.hpp"

class multiple_wait_t {
public:
  multiple_wait_t();
  ~multiple_wait_t();

  bool add(wait_object_t* obj);
  bool remove(wait_object_t* obj);
  void clear();
  wait_object_t* wait_one();
  void wait_all();

private:
  void multiple_wait_callback(wait_object_t* obj,
                              wait_result_t result);
  void wait_internal();

  class multiple_wait_callback_t : public wait_callback_t {
  public:
    multiple_wait_callback_t(multiple_wait_t* multiple_waiter, wait_object_t* target) :
      m_multiple_waiter(multiple_waiter), m_target(target) { }

    void wait_callback(wait_result_t result) {
      m_multiple_waiter->multiple_wait_callback(m_target, result);
    }

    multiple_wait_t* m_multiple_waiter;
    wait_object_t* m_target;
  };

  class multiple_wait_callback_lt {
  public:
    bool operator () (const multiple_wait_callback_t* left, const multiple_wait_callback_t* right) {
      return left->m_target < right->m_target;
    }
  };

  bool m_waitAll;
  wait_callback_t* m_waiter; // Only one waiter allowed per set
  std::set<wait_object_t*> m_waitSuccesses;
  std::set<multiple_wait_callback_t*, multiple_wait_callback_lt> m_waits;
  Arena<multiple_wait_callback_t> m_cb_arena;
};

#endif
