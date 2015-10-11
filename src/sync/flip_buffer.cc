#include "sync/flip_buffer.hpp"

#include "coro/thread.hpp"
#include "rpc/handler.hpp"

namespace indecorous {

struct flip_buffer_callback_t {
    DECLARE_STATIC_RPC(flip)(flip_buffer_base_t *) -> void;
};

IMPL_STATIC_RPC(flip_buffer_callback_t::flip)(flip_buffer_base_t *instance) -> void {
    instance->flip_internal();
}

flip_buffer_base_t::flip_buffer_base_t() :
    m_current_buffers(thread_t::self()->hub()->local_targets().size(), buffer_id_t::BUFFER_A) { }

flip_buffer_base_t::~flip_buffer_base_t() { }

void flip_buffer_base_t::flip() {
    thread_t::self()->hub()->broadcast_local_sync<flip_buffer_callback_t::flip>(this);
}

flip_buffer_base_t::buffer_id_t flip_buffer_base_t::current_buffer_id() const {
    size_t this_thread = thread_t::self()->id();
    assert(this_thread < m_current_buffers.size());
    return m_current_buffers[this_thread];
}

void flip_buffer_base_t::flip_internal() {
    size_t this_thread = thread_t::self()->id();
    assert(this_thread < m_current_buffers.size());
    auto &buffer_id = m_current_buffers[this_thread];
    buffer_id = static_cast<buffer_id_t>((static_cast<int>(buffer_id) + 1) % 2);
}

} // namespace indecorous
