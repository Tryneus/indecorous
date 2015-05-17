#ifndef CORO_CORO_QUEUE_HPP_
#define CORO_CORO_QUEUE_HPP_

#include <cassert>
#include <stddef.h>
#include <type_traits>

template <typename T>
class QueueNode {
public:
  QueueNode() : m_queueNodeNext(nullptr), m_queueNodeParent(nullptr) { }
  virtual ~QueueNode() { }
  T* m_queueNodeNext;
  T* m_queueNodePrev;
  void* m_queueNodeParent;
};

template <typename T>
class IntrusiveQueue {
public:
  IntrusiveQueue() : m_front(nullptr), m_back(nullptr), m_size(0) { };
  ~IntrusiveQueue() { assert(m_front == nullptr); assert(m_back == nullptr); assert(m_size == 0); };

  size_t size() const {
    return m_size;
  }

  void push(T* item) {
    assert(item->QueueNode<T>::m_queueNodeParent == nullptr);
    assert((m_front == nullptr) == (m_back == nullptr));
    item->QueueNode<T>::m_queueNodeParent = this;
    item->QueueNode<T>::m_queueNodeNext = nullptr;
    item->QueueNode<T>::m_queueNodePrev = m_back;

    if (m_back == nullptr)
      m_front = m_back = item;
    else {
      m_back->QueueNode<T>::m_queueNodeNext = item;
      m_back = item;
    }
    ++m_size;
  }

  void push_front(T* item) {
    assert(item->QueueNode<T>::m_queueNodeParent == nullptr);
    assert((m_front == nullptr) == (m_back == nullptr));
    item->QueueNode<T>::m_queueNodeParent = this;
    item->QueueNode<T>::m_queueNodeNext = m_front;
    item->QueueNode<T>::m_queueNodePrev = nullptr;

    if (m_front == nullptr)
      m_front = m_back = item;
    else {
      m_front->QueueNode<T>::m_queueNodePrev = item;
      m_front = item;
    }
    ++m_size;
  }

  T* pop() {
    assert((m_front == nullptr) == (m_back == nullptr));
    T* retval = m_front;

    m_front = retval->QueueNode<T>::m_queueNodeNext;

    if (m_back == retval) {
      assert(retval->QueueNode<T>::m_queueNodeNext == nullptr);
      m_back = nullptr;
    }

    retval->QueueNode<T>::m_queueNodeNext = nullptr;
    retval->QueueNode<T>::m_queueNodePrev = nullptr;
    retval->QueueNode<T>::m_queueNodeParent = nullptr;
    --m_size;
    return retval;
  }

  void remove(T* item) {
    assert(item->QueueNode<T>::m_queueNodeParent == this);
    assert((m_front == nullptr) == (m_back == nullptr));
    if (m_front == item)
      m_front = item->QueueNode<T>::m_queueNodeNext;
    if (m_back == item)
      m_back = item->QueueNode<T>::m_queueNodePrev;
    if (item->QueueNode<T>::m_queueNodeNext != nullptr)
      item->QueueNode<T>::m_queueNodeNext->QueueNode<T>::m_queueNodePrev = item->QueueNode<T>::m_queueNodePrev;
    if (item->QueueNode<T>::m_queueNodePrev != nullptr)
      item->QueueNode<T>::m_queueNodePrev->QueueNode<T>::m_queueNodeNext = item->QueueNode<T>::m_queueNodeNext;
    item->QueueNode<T>::m_queueNodeNext = nullptr;
    item->QueueNode<T>::m_queueNodePrev = nullptr;
    item->QueueNode<T>::m_queueNodeParent = nullptr;
    --m_size;
  }

  T* front() {
    assert((m_front == nullptr) == (m_back == nullptr));
    return m_front;
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

// Based off of the implementation of a mpsc queue from 1024cores.net
template <typename T>
class MpscQueue {
private:
  struct DummyNode {
    T* m_next;
  };
public:
  // This is super ugly, but it should work
  MpscQueue() : m_front((T*)&m_dummy), m_back((T*)&m_dummy) { m_dummy.m_next = nullptr; }

  ~MpscQueue() {
    assert((void*)m_front == (void*)&m_dummy);
    assert((void*)m_back == (void*)&m_dummy);
    assert(m_dummy.m_next == nullptr);
  }

  void push(T* item) {
    assert(item->QueueNode<T>::m_queueNodeParent == nullptr);
    item->QueueNode<T>::m_queueNodeNext = nullptr;
    T* prev = __sync_lock_test_and_set(&m_back, item);

    if ((void*)prev == (void*)&m_dummy)
      m_dummy.m_next = item;
    else
      prev->QueueNode<T>::m_queueNodeNext = item;
  }

  void pushDummy() {
    assert(m_dummy.m_next == nullptr);
    T* prev = __sync_lock_test_and_set(&m_back, (T*)&m_dummy);

    // It should not be possible to have two dummy nodes in the list
    assert((void*)prev != (void*)&m_dummy);
    prev->QueueNode<T>::m_queueNodeNext = (T*)&m_dummy;
  }

  T* pop() {
    if ((void*)m_front == (void*)&m_dummy) {
      if (m_dummy.m_next == nullptr)
        return nullptr;
      m_front = m_dummy.m_next;
      m_dummy.m_next = nullptr;
    }

    assert((void*)m_front != (void*)&m_dummy);

    T* node = m_front;
    T* next = m_front->QueueNode<T>::m_queueNodeNext;

    if (next != nullptr) {
      assert((void*)node != (void*)&m_dummy);
      m_front = next;
      return node;
    }

    if (m_back != m_front)
      return nullptr;

    pushDummy();
    next = m_front->QueueNode<T>::m_queueNodeNext;

    if (next != nullptr) {
      assert((void*)node != (void*)&m_dummy);
      m_front = next;
      node->QueueNode<T>::m_queueNodeNext = nullptr;
      return node;
    }

    return nullptr;
  }

  T* front() {
    T* node = m_front;
    if ((void*)m_front == (void*)&m_dummy) {
      if (m_dummy.m_next == nullptr)
        return nullptr;
      node = m_dummy.m_next;
    }
    assert((void*)node != (void*)&m_dummy);
    return node;
  }

  bool empty() const {
    if ((void*)m_front == (void*)&m_dummy) {
      return m_dummy.m_next == nullptr;
    }
    return false;
  }

private:
  DummyNode m_dummy;
  T* m_front;
  T* m_back;
};

#endif
