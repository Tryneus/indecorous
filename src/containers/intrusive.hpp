#ifndef CONTAINERS_INTRUSIVE_HPP_
#define CONTAINERS_INTRUSIVE_HPP_

#include <atomic>
#include <cassert>
#include <cstddef>
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
    virtual ~intrusive_node_t() {
        assert(m_next == nullptr);
        assert(m_prev == nullptr);
    }

    bool in_a_list() { return (m_next != nullptr) || (m_prev != nullptr); }

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
            m_size(0) {
        this->set_prev_node(this);
        this->set_next_node(this);
    }

    intrusive_list_t(intrusive_list_t<T> &&other) :
            intrusive_node_t<T>(std::move(other)),
            m_size(other.m_size) {
        if (this->next_node() == &other) {
            assert(m_size == 0);
            assert(this->prev_node() == &other);
            this->set_prev_node(this);
            this->set_next_node(this);
        }
        other.set_next_node(&other);
        other.set_prev_node(&other);
        other.m_size = 0;
    }
    ~intrusive_list_t() {
        assert(m_size == 0);
        assert(this->next_node() == this);
        assert(this->prev_node() == this);
        this->set_next_node(nullptr);
        this->set_prev_node(nullptr);
    }

    size_t size() const {
        return m_size;
    }

    template <typename Callable>
    void each(Callable &&c) {
        T *item = front();
        while (item != nullptr) {
            c(item);
            item = next(item);
        }
    }

    template <typename Callable>
    void clear(Callable &&c) {
        while (!empty()) {
            c(pop_front());
        }
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

    void insert_before(T *after, T *item) {
        insert_between(item, after->intrusive_node_t<T>::prev_node(), after);
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
class mpsc_queue_t : private intrusive_node_t<T> {
public:
    mpsc_queue_t() : m_front(this), m_back(reinterpret_cast<intptr_t>(this)) {
        this->set_next_node(nullptr);
    }

    ~mpsc_queue_t() {
        assert(this == m_front);
        assert(this == reinterpret_cast<intrusive_node_t<T> *>(m_back.load()));
        assert(this->next_node() == nullptr);
    }

    void push(T *item) {
        item->set_next_node(nullptr);
        intrusive_node_t<T> *prev = exchange_back(item);
        prev->set_next_node(item);
    }

    T *pop() {
        if (m_front == this) {
            if (this->next_node() == nullptr) { return nullptr; }
            m_front = this->next_node();
            this->set_next_node(nullptr);
        }

        assert(m_front != this);

        intrusive_node_t<T> *node = m_front;
        intrusive_node_t<T> *next = m_front->next_node();

        if (next != nullptr) {
            assert(node != this);
            m_front = next;
            node->set_next_node(nullptr);
            return static_cast<T *>(node);
        }

        if (m_front != reinterpret_cast<intrusive_node_t<T> *>(m_back.load())) {
            return nullptr;
        }

        push_self();
        next = m_front->next_node();

        if (next != nullptr) {
            m_front = next;
            node->set_next_node(nullptr);
            return static_cast<T *>(node);
        }

        return nullptr;
    }

    // This should not be called while other threads could be writing to the queue
    size_t size() const {
        size_t res = 0;
        for (auto cursor = m_front; cursor != nullptr; cursor = cursor->next_node()) {
            if (cursor != this) { ++res; }
        }
        return res;
    }

private:
    void push_self() {
        assert(this->next_node() == nullptr);
        intrusive_node_t<T> *prev = exchange_back(this);
        prev->set_next_node(this);
    }

    intrusive_node_t<T> *exchange_back(intrusive_node_t<T> *item) {
        return reinterpret_cast<intrusive_node_t<T> *>(m_back.exchange(
            reinterpret_cast<intptr_t>(item)));
    }

    intrusive_node_t<T> *m_front;
    std::atomic<intptr_t> m_back; // intrusive_node_t<T> *
};

} // namespace indecorous

#endif // CONTAINERS_INTRUSIVE_HPP_
