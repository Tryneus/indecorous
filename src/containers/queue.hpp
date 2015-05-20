#ifndef CONTAINERS_QUEUE_HPP_
#define CONTAINERS_QUEUE_HPP_

#include <cassert>
#include <stddef.h>
#include <type_traits>

namespace indecorous {

template <typename T>
class intrusive_node_t {
public:
  intrusive_node_t() : m_intrusive_node_next(nullptr), m_intrusive_node_parent(nullptr) { }
  intrusive_node_t(intrusive_node_t &&other) :
      m_intrusive_node_parent(other.m_intrusive_node_parent)
  {
      // TODO: get this working
    other.m_intrusive_node_next = nullptr;
    other.m_intrusive_node_prev = nullptr;
    other.m_intrusive_node_parent = nullptr;
  }
  virtual ~intrusive_node_t() { }
  T* m_intrusive_node_next;
  T* m_intrusive_node_prev;
  void* m_intrusive_node_parent;
};

template <typename T>
class intrusive_queue_t {
public:
  intrusive_queue_t() :
      m_front(nullptr), m_back(nullptr), m_size(0) { }
  intrusive_queue_t(intrusive_queue_t<T> &&other) {

  }
  ~intrusive_queue_t() {
      assert(m_front == nullptr);
      assert(m_back == nullptr);
      assert(m_size == 0);
  }

  size_t size() const {
    return m_size;
  }

  void push(T* item) {
    assert(item->intrusive_node_t<T>::m_intrusive_node_parent == nullptr);
    assert((m_front == nullptr) == (m_back == nullptr));
    item->intrusive_node_t<T>::m_intrusive_node_parent = this;
    item->intrusive_node_t<T>::m_intrusive_node_next = nullptr;
    item->intrusive_node_t<T>::m_intrusive_node_prev = m_back;

    if (m_back == nullptr)
      m_front = m_back = item;
    else {
      m_back->intrusive_node_t<T>::m_intrusive_node_next = item;
      m_back = item;
    }
    ++m_size;
  }

  void push_front(T* item) {
    assert(item->intrusive_node_t<T>::m_intrusive_node_parent == nullptr);
    assert((m_front == nullptr) == (m_back == nullptr));
    item->intrusive_node_t<T>::m_intrusive_node_parent = this;
    item->intrusive_node_t<T>::m_intrusive_node_next = m_front;
    item->intrusive_node_t<T>::m_intrusive_node_prev = nullptr;

    if (m_front == nullptr)
      m_front = m_back = item;
    else {
      m_front->intrusive_node_t<T>::m_intrusive_node_prev = item;
      m_front = item;
    }
    ++m_size;
  }

  T* pop() {
    assert((m_front == nullptr) == (m_back == nullptr));
    T* retval = m_front;

    m_front = retval->intrusive_node_t<T>::m_intrusive_node_next;

    if (m_back == retval) {
      assert(retval->intrusive_node_t<T>::m_intrusive_node_next == nullptr);
      m_back = nullptr;
    }

    retval->intrusive_node_t<T>::m_intrusive_node_next = nullptr;
    retval->intrusive_node_t<T>::m_intrusive_node_prev = nullptr;
    retval->intrusive_node_t<T>::m_intrusive_node_parent = nullptr;
    --m_size;
    return retval;
  }

  void remove(T* item) {
    assert(item->intrusive_node_t<T>::m_intrusive_node_parent == this);
    assert((m_front == nullptr) == (m_back == nullptr));
    if (m_front == item)
      m_front = item->intrusive_node_t<T>::m_intrusive_node_next;
    if (m_back == item)
      m_back = item->intrusive_node_t<T>::m_intrusive_node_prev;
    if (item->intrusive_node_t<T>::m_intrusive_node_next != nullptr)
      item->intrusive_node_t<T>::m_intrusive_node_next->intrusive_node_t<T>::m_intrusive_node_prev =
          item->intrusive_node_t<T>::m_intrusive_node_prev;
    if (item->intrusive_node_t<T>::m_intrusive_node_prev != nullptr)
      item->intrusive_node_t<T>::m_intrusive_node_prev->intrusive_node_t<T>::m_intrusive_node_next =
          item->intrusive_node_t<T>::m_intrusive_node_next;
    item->intrusive_node_t<T>::m_intrusive_node_next = nullptr;
    item->intrusive_node_t<T>::m_intrusive_node_prev = nullptr;
    item->intrusive_node_t<T>::m_intrusive_node_parent = nullptr;
    --m_size;
  }

  T* front() {
    assert((m_front == nullptr) == (m_back == nullptr));
    return m_front;
  }

  T* next(T* node) {
    assert(node->m_intrusive_node_parent == this);
    return node->m_intrusive_node_next;
  }

  bool empty() const {
    assert((m_front == nullptr) == (m_back == nullptr));
    return (m_front == nullptr);
  }

private:
  T* m_front;
  T* m_back;
  size_t m_size;
};

} // namespace indecorous

#endif // CONTAINERS_QUEUE_HPP_
