#include "rpc/target.hpp"

#include "coro/thread.hpp"
#include "rpc/hub.hpp"

namespace indecorous {

target_t::target_t() :
    target_id(target_id_t::assign()) { }

target_t::~target_t() { }

target_id_t target_t::id() const {
    return target_id;
}

void target_t::wait(wait_object_t *interruptor) {
    stream()->wait(interruptor);
}

void target_t::note_send() const {
    thread_t *t = thread_t::self();
    if (t != nullptr) {
        t->dispatcher()->note_send(); 
    }
}

void target_t::send_reply(write_message_t &&msg) {
    // Replies do not have to note a send - there should already be a coroutine waiting
    // to receive the reply - if not it gets discarded.
    stream()->write(std::move(msg));
}

future_t<read_message_t> target_t::get_response(request_id_t request_id) {
    return thread_t::self()->hub()->get_response(request_id);
}

template <> void target_t::parse_result(read_message_t) { }

local_target_t::local_target_t() :
    target_t(), m_stream() { }

bool local_target_t::handle(message_hub_t *local_hub) {
    read_message_t msg(m_stream.read());
    if (!msg.buffer.has()) { return false; }
    local_hub->spawn(std::move(msg));
    return true;
}

stream_t *local_target_t::stream() {
    return &m_stream;
}

bool local_target_t::is_local() const {
    return true;
}

remote_target_t::remote_target_t() :
    target_t(), m_stream(-1) { }

stream_t *remote_target_t::stream() {
    return &m_stream;
}

bool remote_target_t::is_local() const {
    return false;
}

} // namespace indecorous
