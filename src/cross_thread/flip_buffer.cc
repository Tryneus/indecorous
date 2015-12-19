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
        m_thread_data() {
    const std::vector<target_t *> &local_targets = thread_t::self()->hub()->local_targets();

    for (auto &&t : local_targets) {
        m_thread_data[t->id()].current_buffer = buffer_id_t::BUFFER_A;
    }
}

flip_buffer_base_t::~flip_buffer_base_t() { }

void flip_buffer_base_t::flip() {
    thread_t::self()->hub()->broadcast_local_sync<flip_buffer_callback_t::flip>(
            reinterpret_cast<uint64_t>(this));
}

flip_buffer_base_t::buffer_id_t flip_buffer_base_t::current_buffer_id() const {
    return m_thread_data.at(thread_t::self()->target()->id()).current_buffer;
}

void flip_buffer_base_t::flip_internal() {
    auto &thread_data = m_thread_data.at(thread_t::self()->target()->id());
    thread_data.current_buffer =
        static_cast<buffer_id_t>((static_cast<int>(thread_data.current_buffer) + 1) % 2);

    assert_no_swap_t no_swap;
    thread_data.subscriptions.each([&] (auto const &s) {
            s->m_base->on_flip(thread_data.current_buffer);
        });
}

flip_buffer_base_t::subscription_t::subscription_t(flip_buffer_base_t *parent,
                                              std::unique_ptr<base_t> base) :
        m_parent(parent),
        m_base(std::move(base)),
        m_lock(m_parent->m_drainer.lock()) {
    target_id_t id = thread_t::self()->target()->id();
    m_parent->m_thread_data[id].subscriptions.push_back(this);
}

flip_buffer_base_t::subscription_t::subscription_t(subscription_t &&other) :
        m_parent(other.m_parent),
        m_base(std::move(other.m_base)),
        m_lock(std::move(other.m_lock)) {
    other.m_parent = nullptr;
}

flip_buffer_base_t::subscription_t::~subscription_t() {
    if (m_parent != nullptr) {
        target_id_t id = thread_t::self()->target()->id();
        m_parent->m_thread_data[id].subscriptions.remove(this);
    }
}

} // namespace indecorous
