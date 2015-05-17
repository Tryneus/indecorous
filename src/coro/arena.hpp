#ifndef CORO_ARENA_HPP_
#define CORO_ARENA_HPP_
#include <assert.h>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>
#include <atomic>
#include <iostream>

#include "common.hpp"
#include "coro/sched.hpp"
#include "coro/queue.hpp"

template <typename T>
class Arena {
private:
  static const uint32_t s_node_magic = 0x712B4057;
  // linked list of unallocated items
  struct node_t : public QueueNode<node_t>
  {
    uint32_t magic;
    char buffer[sizeof(T)];
  };

  const size_t m_max_free_nodes;
  std::atomic<size_t> m_free_node_count;
  MpscQueue<node_t> m_free_nodes;

public:
  //  max_free_nodes - maximum number of unused buffers to keep around before releasing
  // TODO: consider adding parameters:
  //  chunk_size - number of buffers to allocate with one alloc call
  //  initial_size - start with a static buffer of some number of T buffers
  Arena(size_t max_free_nodes) :
    m_max_free_nodes(max_free_nodes),
    m_free_node_count(0)
  { }

  ~Arena() {
    while (!m_free_nodes.empty()) {
      delete m_free_nodes.pop();
      --m_free_node_count;
    }

    assert(m_free_node_count == 0);
  }

  // TODO: consider adding the ability to rebalance between arenas,
  //  perhaps using mpscqueues for nodes

  size_t free_count() const {
    return m_free_node_count;
  }

  static void rebalance(Arena<T>* left, Arena<T>* right) {
    size_t left_size = left->m_free_node_count;
    size_t right_size = right->m_free_node_count;

    // We are supposed to be on left's thread, so we can only pop from left
    if (left_size > right_size) {
      size_t to_move = (left_size - right_size) / 2;
      assert(to_move < left_size);

      while (to_move > 0) {
        // We should be guaranteed that this thread is not doing anything else 
        node_t* temp_node = left->m_free_nodes.pop();
        --left->m_free_node_count;
        --to_move;
        right->m_free_nodes.push(temp_node);
        ++right->m_free_node_count;
      }
    }
  }

#define _ARENA_GET_FN_BODY(args...)                \
           do {                                    \
             node_t *selected_node = m_free_nodes.pop(); \
             if (selected_node == nullptr) {       \
               selected_node = new node_t;         \
               selected_node->magic = s_node_magic;\
             } else {                              \
               --m_free_node_count;                \
             }                                     \
             new (&selected_node->buffer) T(args); \
             return (T*)selected_node->buffer;     \
           } while(0)                              \

  template <typename P1>
  T* get(P1 p1) { _ARENA_GET_FN_BODY(p1); }

  template <typename P1, typename P2>
  T* get(P1 p1, P2 p2) { _ARENA_GET_FN_BODY(p1, p2); }

  template <typename P1, typename P2, typename P3>
  T* get(P1 p1, P2 p2, P3 p3) { _ARENA_GET_FN_BODY(p1, p2, p3); }

  template <typename P1, typename P2, typename P3, typename P4>
  T* get(P1 p1, P2 p2, P3 p3, P4 p4) { _ARENA_GET_FN_BODY(p1, p2, p3, p4); }

  template <typename P1, typename P2, typename P3, typename P4, typename P5>
  T* get(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { _ARENA_GET_FN_BODY(p1, p2, p3, p4, p5); }

  template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
  T* get(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) { _ARENA_GET_FN_BODY(p1, p2, p3, p4, p5, p6); }

  template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
  T* get(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) { _ARENA_GET_FN_BODY(p1, p2, p3, p4, p5, p6, p7); }

  template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
  T* get(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8) { _ARENA_GET_FN_BODY(p1, p2, p3, p4, p5, p6, p7, p8); }

  template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
  T* get(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9) { _ARENA_GET_FN_BODY(p1, p2, p3, p4, p5, p6, p7, p8, p9); }

#undef _ARENA_GET_FN_BODY

  void release(T* item) {
    node_t *dummy = reinterpret_cast<node_t*>(item);
    size_t offset = reinterpret_cast<char*>(&dummy->buffer) - reinterpret_cast<char*>(dummy);
    node_t *node = reinterpret_cast<node_t*>(reinterpret_cast<char*>(item) - offset);

    if (node->magic != s_node_magic)
      throw std::runtime_error("item released to arena either invalid or buffer overflowed");
    item->~T();

    if (m_free_node_count >= m_max_free_nodes) {
      delete node;
    } else {
      m_free_nodes.push(node);
      ++m_free_node_count;
    }
  }
};

#endif
