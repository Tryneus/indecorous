#include "cross_thread/flip_buffer.hpp"

#include "coro/thread.hpp"
#include "rpc/handler.hpp"

namespace indecorous {

class flip_buffer_callback_t {
public:
    DECLARE_STATIC_RPC(flip)(uint64_t) -> void;
};

// TODO: this is really stupid and unsafe, but should we really allow sending pointers?
IMPL_STATIC_RPC(flip_buffer_callback_t::flip)(uint64_t unsafe_ptr) -> void {
    flip_buffer_base_t *instance = reinterpret_cast<flip_buffer_base_t *>(unsafe_ptr);
    instance->flip_internal();
}

flip_buffer_base_t::flip_buffer_base_t() :
        m_current_buffers() {
    const std::vector<target_t *> &local_targets = thread_t::self()->hub()->local_targets();

    for (auto &&t : local_targets) {
        m_current_buffers[t->id()] = buffer_id_t::BUFFER_A;
    }
}

flip_buffer_base_t::~flip_buffer_base_t() { }

void flip_buffer_base_t::flip() {
    thread_t::self()->hub()->broadcast_local_sync<flip_buffer_callback_t::flip>(
            reinterpret_cast<uint64_t>(this));
}

flip_buffer_base_t::buffer_id_t flip_buffer_base_t::current_buffer_id() const {
    return m_current_buffers.at(thread_t::self()->target()->id());
}

void flip_buffer_base_t::flip_internal() {
    auto &buffer_id = m_current_buffers.at(thread_t::self()->target()->id());
    buffer_id = static_cast<buffer_id_t>((static_cast<int>(buffer_id) + 1) % 2);
}

} // namespace indecorous
