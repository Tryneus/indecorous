#ifndef CONTAINERS_ARENA_HPP_
#define CONTAINERS_ARENA_HPP_
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "containers/queue.hpp"

namespace indecorous {

template <typename T>
class Arena {
private:
  static const uint32_t s_node_magic = 0x712B4057;
  // linked list of unallocated items
  struct node_t : public QueueNode<node_t>
  {
    uint32_t magic;
    alignas(T) char buffer[sizeof(T)];
  };

  const size_t m_max_free_nodes;
  size_t m_free_node_count;
  IntrusiveQueue<node_t> m_free_nodes;

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

  size_t free_count() const {
    return m_free_node_count;
  }

  template <typename... Args>
  T *get(Args &&...args) {
    node_t *selected_node = m_free_nodes.pop();
    if (selected_node == nullptr) {
        selected_node = new node_t;
        selected_node->magic = s_node_magic;
    } else {
        --m_free_node_count;
    }
    new (selected_node->buffer) T(std::forward<Args>(args)...);
    return reinterpret_cast<T *>(selected_node->buffer);
  }

  void release(T* item) {
    node_t *dummy = reinterpret_cast<node_t*>(item);
    size_t offset = reinterpret_cast<char*>(&dummy->buffer) - reinterpret_cast<char*>(dummy);
    node_t *node = reinterpret_cast<node_t*>(reinterpret_cast<char*>(item) - offset);

    assert(node->magic == s_node_magic);
    item->~T();

    if (m_free_node_count >= m_max_free_nodes) {
      delete node;
    } else {
      m_free_nodes.push(node);
      ++m_free_node_count;
    }
  }
};

} // namespace indecorous

#endif // CONTAINERS_ARENA_HPP_
