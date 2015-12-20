#include "cross_thread/flip_buffer.hpp"

#include "coro/thread.hpp"
#include "rpc/handler.hpp"

namespace indecorous {

SERIALIZABLE_POINTER(flip_buffer_base_t);

class flip_buffer_callback_t {
public:
    DECLARE_STATIC_RPC(flip)(flip_buffer_base_t *instance) -> void;
};

IMPL_STATIC_RPC(flip_buffer_callback_t::flip)(flip_buffer_base_t *instance) -> void {
    instance->flip_internal();
}

flip_buffer_base_t::flip_buffer_base_t() :
        m_current_buffer(buffer_id_t::BUFFER_A) { }

flip_buffer_base_t::~flip_buffer_base_t() { }

void flip_buffer_base_t::flip() {
    thread_t::self()->hub()->broadcast_local_sync<flip_buffer_callback_t::flip>(this);
}

void flip_buffer_base_t::flip_internal() {
    m_current_buffer.get() =
        static_cast<buffer_id_t>((static_cast<int>(m_current_buffer.get()) + 1) % 2);
    on_flip();
}

} // namespace indecorous
