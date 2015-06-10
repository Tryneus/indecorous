#ifndef RPC_TARGET_HPP_
#define RPC_TARGET_HPP_

#include <unordered_map>

#include "coro/coro.hpp"
#include "rpc/handler.hpp"
#include "rpc/id.hpp"
#include "rpc/message.hpp"
#include "rpc/stream.hpp"
#include "sync/promise.hpp"

namespace indecorous {

class wait_object_t;

class target_t {
public:
    explicit target_t();
    virtual ~target_t();

    target_id_t id() const;

    virtual bool is_local() const = 0;

    template <typename Callback, typename... Args>
    void call_noreply(Args &&...args) {
        if (is_local()) { note_send(); }
        send_request<Callback>(request_id_t::noreply(), std::forward<Args>(args)...);
    }

    template <typename Callback, typename result_t = typename handler_wrapper_t<Callback>::result_t, typename... Args>
    result_t call_sync(Args &&...args) {
        if (is_local()) { note_send(); }
        request_id_t request_id = send_request(request_gen.next(), std::forward<Args>(args)...);
        return parse_result<result_t>(get_response(request_id));
    }

    template <typename Callback, typename result_t = typename handler_wrapper_t<Callback>::result_t, typename... Args>
    future_t<result_t> call_async(Args &&...args) {
        if (is_local()) { note_send(); }
        request_id_t request_id = send_request(request_gen.next(), std::forward<Args>(args)...);
        return coro_t::spawn(&target_t::parse_result<result_t>, get_response(request_id));
    }

    void send_reply(write_message_t &&msg);

    void wait(wait_object_t *interruptor);

protected:
    virtual stream_t *stream() = 0;

    target_id_t target_id;

    std::unordered_map<request_id_t, promise_t<read_message_t> > request_map;

private:
    void note_send() const;

    template <typename Callback, typename... Args>
    request_id_t send_request(request_id_t request_id, Args &&...args) {
        write_message_t msg =
            handler_wrapper_t<Callback>::handler_impl_t::make_write(id(),
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
    local_target_t();

    // Returns true if a message was processed, false otherwise
    bool handle(message_hub_t *local_hub);

    bool is_local() const;
private:
    friend class scheduler_t; // To get the initial stream size
    stream_t *stream();
    local_stream_t m_stream;
};

// TODO: need to be able to address multiple targets in a remote process
class remote_target_t : public target_t {
public:
    explicit remote_target_t();
    bool is_local() const;
private:
    stream_t *stream();
    tcp_stream_t m_stream;
};

} // namespace indecorous

#endif // TARGET_HPP_
