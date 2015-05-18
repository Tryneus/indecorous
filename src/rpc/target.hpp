#ifndef RPC_TARGET_HPP_
#define RPC_TARGET_HPP_

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
    target_t(message_hub_t *hub);
    virtual ~target_t();

    target_id_t id() const;

    template <class Callback, typename... Args>
    void noreply_call(Args &&...args) {
        write_message_t msg =
            handler_t<Callback>::handler_impl_t::make_write(request_id_t::noreply(),
                                                            std::forward<Args>(args)...);
        stream()->write(std::move(msg));
    }

    /*
    template <class Callback, typename result_t = typename handler_t<Callback>::result_t, typename... Args>
    result_t call(Args &&...args) {
        request_id_t request_id = new_request_id();
        write_message_t msg =
            handler_t<Callback>::handler_impl_t::make_write(request_id,
                                                            std::forward<Args>(args)...);
        stream()->write(std::move(msg));
        promise_t<read_message_t> promise = get_response(request_id);
        read_message_t response(promise.release());
        return serializer_t<result_t>::read(&response);
    }
    */

    /*
    template <class Callback, typename result_t = typename handler_t<Callback>::result_t, typename... Args>
    promise_t<result_t> async_call(Args &&...args) {
        request_id_t request_id = new_request_id();
        write_message_t msg =
            handler_t<Callback>::handler_impl_t::make_write(request_id,
                                                            std::forward<Args>(args)...);
        stream()->write(std::move(msg));
        // TODO: chain promises
        promise_t<read_message_t> promise = get_response(request_id);
        read_message_t response(promise.release());
        return serializer_t<result_t>::read(&response);
    }
    */


protected:
    virtual stream_t *stream() = 0;

private:
    /*
    class sync_request_t {
    public:
        sync_request_t(target_t *_parent, request_id_t request_id);
        ~sync_request_t();
        message_t run(message_t &&msg);
    private:
        target_t *parent;
        request_id_t request_id;
        event_t done_event;
    };
    */
    target_id_t target_id;
    message_hub_t::membership_t<target_t> membership;
};

class local_target_t : public target_t {
public:
    local_target_t(message_hub_t *hub,
                   thread_t *owner);
private:
    stream_t *stream();
    local_stream_t stream_;
};

/*
class remote_target_t : public target_t {
public:
    remote_target_t(message_hub_t *hub);
private:
    stream_t *stream();
    tcp_stream_t stream_;
};
*/

} // namespace indecorous

#endif // TARGET_HPP_
