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

class waitable_t;

class target_t {
public:
    explicit target_t();
    virtual ~target_t();

    target_id_t id() const;

    virtual bool is_local() const = 0;

    template <typename RPC, typename... Args>
    void call_noreply(Args &&...args) {
        note_send();
        // TODO: this is the wrong source id
        send_request<RPC>(id(), request_id_t::noreply(), std::forward<Args>(args)...);
    }

    template <typename RPC, typename... Args,
              typename Res = typename decltype(rpc_bridge(RPC::fn_ptr()))::result_t>
    Res call_sync(Args &&...args) {
        note_send();
        auto params = new_request();
        send_request<RPC>(params.source_id, params.request_id, std::forward<Args>(args)...);
        return serializer_t<Res>::read(params.future.release());
    }

    template <typename RPC, typename... Args,
              typename Res = typename decltype(rpc_bridge(RPC::fn_ptr()))::result_t>
    future_t<Res> call_async(Args &&...args) {
        note_send();
        auto params = new_request();
        send_request<RPC>(params.source_id, params.request_id, std::forward<Args>(args)...);
        return params.future.then_release(&target_t::parse_result<Res>);
    }

    void send_reply(write_message_t &&msg);

    void wait(waitable_t *interruptor);

protected:
    virtual stream_t *stream() = 0;

    target_id_t target_id;

private:
    friend class message_hub_t;
    struct request_params_t {
        target_id_t source_id;
        request_id_t request_id;
        future_t<read_message_t> future;
    };
    request_params_t new_request() const;

    void note_send() const;

    template <typename RPC, typename... Args>
    void send_request(target_id_t source_id, request_id_t request_id, Args &&...args) {
        typedef typename decltype(rpc_bridge(RPC::fn_ptr()))::write_t rpc_write_t;
        stream()->write(rpc_write_t::make(source_id,
                                          RPC::s_rpc_id,
                                          request_id,
                                          std::forward<Args>(args)...));
    }

    template <typename Res>
    static Res parse_result(read_message_t msg) {
        return serializer_t<Res>::read(&msg);
    }
};

template <> void target_t::parse_result(read_message_t data);

class local_target_t : public target_t {
public:
    local_target_t();

    // Returns true if a message was processed, false otherwise
    bool handle(message_hub_t *local_hub);

    bool is_local() const;
private:
    friend class scheduler_t; // To get the initial stream size
    friend class thread_t; // To check empty at the end of a run
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
