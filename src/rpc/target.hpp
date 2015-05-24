#ifndef RPC_TARGET_HPP_
#define RPC_TARGET_HPP_

#include <unordered_map>

#include "rpc/handler.hpp"
#include "rpc/hub.hpp"
#include "rpc/id.hpp"
#include "rpc/message.hpp"
#include "rpc/stream.hpp"
#include "sync/event.hpp"

namespace indecorous {

class thread_t;

class target_t {
public:
    explicit target_t(message_hub_t *hub);
    virtual ~target_t();

    target_id_t id() const;

    template <typename Callback, typename... Args>
    void noreply_call(Args &&...args) {
        send_request<Callback>(std::forward<Args>(args)...);
    }

    template <typename Callback, typename result_t = typename handler_t<Callback>::result_t, typename... Args>
    result_t call(Args &&...args) {
        request_id_t request_id = send_request(std::forward<Args>(args)...);
        return parse_result<result_t>(get_response(request_id));
    }

    template <typename Callback, typename result_t = typename handler_t<Callback>::result_t, typename... Args>
    future_t<result_t> async_call(Args &&...args) {
        request_id_t request_id = send_request(std::forward<Args>(args)...);
        return coro_t::spawn(&target_t::parse_result<result_t>, get_response(request_id));
    }


protected:
    virtual stream_t *stream() = 0;

    target_id_t target_id;
    message_hub_t::membership_t<target_t> membership;

    std::unordered_map<request_id_t, promise_t<read_message_t> > request_map;

private:
    template <typename Callback, typename... Args>
    request_id_t send_request(Args &&...args) {
        request_id_t request_id = request_gen.next();
        write_message_t msg =
            handler_t<Callback>::handler_impl_t::make_write(id(),
                                                            request_id,
                                                            std::forward<Args>(args)...);
        stream()->write(std::move(msg));
        return request_id;
    }

    template <typename result_t>
    result_t parse_result(future_t<read_message_t> data) {
        return serializer_t<result_t>::read(data.release());
    }

    future_t<read_message_t> get_response(request_id_t request_id); // TODO: implement
    id_generator_t<request_id_t> request_gen;
};

class local_target_t : public target_t {
public:
    local_target_t(message_hub_t *hub, thread_t *owner);

    // Convert RPCs into local coroutines on the owning thread
    void pull_calls();
private:
    stream_t *stream();
    local_stream_t m_stream;
};

// TODO: need to be able to address multiple targets in a remote process
class remote_target_t : public target_t {
public:
    explicit remote_target_t(message_hub_t *hub);

private:
    stream_t *stream();
    tcp_stream_t m_stream;
};

} // namespace indecorous

#endif // TARGET_HPP_
