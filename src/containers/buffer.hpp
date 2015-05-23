#ifndef CONTAINERS_BUFFER_HPP_
#define CONTAINERS_BUFFER_HPP_

#include <cstddef>

#include "containers/intrusive.hpp"

namespace indecorous {

// This class does super dangerous stuff with memory management, be careful using it
// You should probably not use it for anything other than a char array because it has
// no alignment rules.
class linkable_buffer_t : public intrusive_node_t<linkable_buffer_t> {
public:
    static linkable_buffer_t *create(size_t capacity) {
        char *buffer = new char[sizeof(linkable_buffer_t) + capacity];
        return new (buffer) linkable_buffer_t(capacity);
    }

    static void destroy(linkable_buffer_t *buffer) {
        buffer->~linkable_buffer_t();
        char *c = reinterpret_cast<char *>(buffer);
        delete [] c;
    }

    size_t capacity() const {
        return m_capacity;
    }

    char *data() {
        return m_data;
    }

    const char *data() const {
        return m_data;
    }
private:
    friend class buffer_owner_t;
    linkable_buffer_t(size_t cap) : m_capacity(cap) { }
    ~linkable_buffer_t() { }

    size_t m_capacity;
    char m_data[0];
};

class buffer_owner_t {
    enum class alloc_info_t { HEAP, ARRAY };
public:
    buffer_owner_t(size_t cap) :
        m_alloc(alloc_info_t::HEAP), m_buffer(linkable_buffer_t::create(cap)) { }
    buffer_owner_t(buffer_owner_t &&other) :
        m_alloc(other.m_alloc), m_buffer(other.release()) { }

    ~buffer_owner_t() {
        if (m_buffer != nullptr) {
            switch (m_alloc) {
            case alloc_info_t::HEAP: linkable_buffer_t::destroy(m_buffer); break;
            case alloc_info_t::ARRAY: m_buffer->~linkable_buffer_t(); break;
            default: assert(false);
            }
        }
    }

    static buffer_owner_t empty() {
        return buffer_owner_t(nullptr, alloc_info_t::HEAP);
    }

    // Creates a linkable_buffer_t inside an existing data array.  It will be
    // destructed when ownership ceases, but will not be deallocated as the
    // allocation's lifetime depends on the lifetime of the surrounding data array.
    static buffer_owner_t from_array(char *buffer, size_t buffer_size,
                                     size_t cap) {
        assert(buffer_size >= sizeof(linkable_buffer_t) + cap);
        return buffer_owner_t(new (buffer) linkable_buffer_t(cap), alloc_info_t::ARRAY);
    }

    static buffer_owner_t from_heap(linkable_buffer_t *buffer) {
        return buffer_owner_t(buffer, alloc_info_t::HEAP);
    }

    linkable_buffer_t *release() {
        assert(has());
        linkable_buffer_t *res = m_buffer;
        m_buffer = nullptr;
        return res;
    }

    bool has() const {
        return m_buffer != nullptr;
    }

    size_t capacity() const {
        assert(has());
        return m_buffer->capacity();
    }

    char *data() {
        assert(has());
        return m_buffer->data();
    }

    const char *data() const {
        assert(has());
        return m_buffer->data();
    }
private:
    buffer_owner_t(linkable_buffer_t *buffer, alloc_info_t alloc) :
        m_alloc(alloc), m_buffer(buffer) { }

    alloc_info_t m_alloc;
    linkable_buffer_t *m_buffer;
};

} // namespace indecorous

#endif // CONTAINERS_BUFFER_HPP_
