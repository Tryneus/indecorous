#ifndef CONTAINERS_ARENA_HPP_
#define CONTAINERS_ARENA_HPP_
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "containers/intrusive.hpp"

namespace indecorous {

template <typename T>
class arena_t {
private:
    struct node_t : public intrusive_node_t<node_t>
    {
    public:
        node_t() : magic(s_node_magic) { }
        ~node_t() { assert(magic == s_node_magic); }

        template <typename... Args>
        T *construct(Args &&...args) {
            new (buffer) T(std::forward<Args>(args)...);
            return reinterpret_cast<T *>(buffer);
        }

        static node_t *from_buffer(void *p) {
            node_t *temp = reinterpret_cast<node_t*>(p);
            size_t offset = reinterpret_cast<char*>(&temp->buffer) - reinterpret_cast<char*>(temp);
            return reinterpret_cast<node_t*>(reinterpret_cast<char*>(p) - offset);
        }

    private:
        uint32_t magic;
        alignas(T) char buffer[sizeof(T)];
        static const uint32_t s_node_magic = 0x712B4057;
    };

    const size_t m_max_free_nodes;
    size_t m_allocated;
    intrusive_list_t<node_t> m_free_nodes;

public:
    explicit arena_t(size_t max_free_nodes) :
        m_max_free_nodes(max_free_nodes),
        m_allocated(0)
    { }

    ~arena_t() {
        while (!m_free_nodes.empty()) {
            delete m_free_nodes.pop_front();
            --m_allocated;
        }

        assert(m_allocated == 0);
    }

    template <typename... Args>
    T *get(Args &&...args) {
        node_t *selected_node = m_free_nodes.pop_front();
        if (selected_node == nullptr) {
            selected_node = new node_t();
            ++m_allocated;
        }
        return selected_node->construct(std::forward<Args>(args)...);
    }

    void release(T* item) {
        node_t *node = node_t::from_buffer(item);
        item->~T();

        if (m_free_nodes.size() >= m_max_free_nodes) {
            delete node;
            --m_allocated;
        } else {
            m_free_nodes.push_back(node);
        }
    }
};

} // namespace indecorous

#endif // CONTAINERS_ARENA_HPP_
