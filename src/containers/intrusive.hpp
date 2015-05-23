#ifndef CONTAINERS_INTRUSIVE_HPP_
#define CONTAINERS_INTRUSIVE_HPP_

#include <atomic>
#include <cassert>
#include <stddef.h>
#include <type_traits>
#include <utility>

namespace indecorous {

// TODO: add strong asserts to all this stuff

template <typename T>
class intrusive_list_t;

template <typename T>
class intrusive_node_t {
public:
    intrusive_node_t() : m_next(nullptr), m_prev(nullptr) { }
    intrusive_node_t(intrusive_node_t &&other) :
            m_next(other.m_next),
            m_prev(other.m_prev) {
        m_next->m_prev = this;
        m_prev->m_next = this;
        other.m_next = nullptr;
        other.m_prev = nullptr;
    }
    virtual ~intrusive_node_t() { }

    void clear() { m_next = nullptr; m_prev = nullptr; }

    void set_next_node(intrusive_node_t *next) { m_next = next; }
    void set_prev_node(intrusive_node_t *prev) { m_prev = prev; }
    intrusive_node_t *next_node() const { return m_next; }
    intrusive_node_t *prev_node() const { return m_prev; }

private:
    intrusive_node_t* m_next;
    intrusive_node_t* m_prev;
};

template <typename T>
class intrusive_list_t : public intrusive_node_t<T> {
public:
    intrusive_list_t() :
        m_size(0) { }

    intrusive_list_t(intrusive_list_t<T> &&other) :
            intrusive_node_t<T>(std::move(other)),
            m_size(other.m_size) {
        other.m_size = 0;
    }
    ~intrusive_list_t() {
        assert(m_size == 0);
    }

    size_t size() const {
        return m_size;
    }

    T* front() {
        return get_value(this->next_node());
    }

    T* back() {
        return get_value(this->prev_node());
    }

    T* next(T* item) {
        intrusive_node_t<T> *node = item;
        return get_value(node->next_node());
    }

    T* prev(T* item) {
        intrusive_node_t<T> *node = item;
        return get_value(node->prev_node());
    }

    void push_back(T *item) {
        insert_between(item, this->prev_node(), this);
    }

    void push_front(T *item) {
        insert_between(item, this, this->next_node());
    }

    T *pop_front() {
        T *res = get_value(this->next_node());
        if (res != nullptr) remove_node(res);
        return res;
    }

    T *pop_back() {
        T *res = get_value(this->prev_node());
        if (res != nullptr) remove_node(res);
        return res;
    }

    void remove(T* item) {
        remove_node(item);
    }

    bool empty() const {
        return (this->next_node() == this);
    }

private:
    T *get_value(intrusive_node_t<T> *node) {
        if (node == this) return nullptr;
        return static_cast<T *>(node);
    }

    void insert_between(T *item,
                        intrusive_node_t<T> *before,
                        intrusive_node_t<T> *after) {
        item->intrusive_node_t<T>::set_prev_node(before);
        item->intrusive_node_t<T>::set_next_node(after);
        before->intrusive_node_t<T>::set_next_node(item);
        after->intrusive_node_t<T>::set_prev_node(item);
        ++m_size;
    }

    void remove_node(intrusive_node_t<T> *node) {
        node->prev_node()->set_next_node(node->next_node());
        node->next_node()->set_prev_node(node->prev_node());
        node->clear();
        --m_size;
    }

    size_t m_size;
};

// This is a singly-linked list and does not use 'prev'
// Implementation copied from:
// http://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue
template <typename T>
class mpsc_queue_t : public intrusive_node_t<T> {
public:
    mpsc_queue_t() {
        m_head = this;
        m_tail.store(reinterpret_cast<intptr_t>(this));
    }

    void push(intrusive_node_t<T> *item) {
        intrusive_node_t<T> *old_tail =
            reinterpret_cast<intrusive_node_t<T> *>(m_tail.exchange(
                reinterpret_cast<intptr_t>(item)));
        old_tail->set_next_node(item);
    }

    T *pop() {
        intrusive_node_t<T>* head = m_head;
        intrusive_node_t<T>* next = head->next_node();
        if (head == this) {
            if (next == nullptr) { return nullptr; }
            m_head = next;
            head = next;
            next = next->next_node();
        }
        if (next) {
            m_head = next;
            return static_cast<T *>(head);
        }
        intrusive_node_t<T>* tail =
            reinterpret_cast<intrusive_node_t<T> *>(m_tail.load());
        if (head != tail) { return nullptr; }
        push(this);
        next = head->next_node();
        if (next != nullptr) {
            m_head = next;
            return static_cast<T *>(head);
        }
        return nullptr;
    } 

private:
    intrusive_node_t<T> *m_head;
    std::atomic<intptr_t> m_tail;
};

} // namespace indecorous

#endif // CONTAINERS_INTRUSIVE_HPP_
