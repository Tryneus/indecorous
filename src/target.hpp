#ifndef TARGET_HPP_
#define TARGET_HPP_

#include "hub.hpp"
#include "id.hpp"
#include "message.hpp"
#include "stream.hpp"

class target_t {
public:
    target_t(message_hub_t *hub);
    virtual ~target_t();

    target_id_t id() const;

private:
    target_id_t target_id;
    message_hub_t::membership_t<target_t> membership;
};

class local_target_t : public target_t {
public:
    local_target_t(message_hub_t *hub);

    template <class Handler, typename... Args>
    void noreply_call(Args &&...args) {
        Handler::check_args(std::forward<Args>(args)...);
        write_message_t msg = write_message_t::create(Handler::handler_id(),
                                                      request_id_t::noreply(),
                                                      std::forward<Args>(args)...);
        stream.write(std::move(msg));
    }

    dummy_stream_t stream; // TODO: shortcut
};

class remote_target_t : public target_t {
public:
    remote_target_t(message_hub_t *hub);

    /*
    template <class Handler, typename result_t = Handler::result_t, typename... Args>
    result_t call(Args &&...args) {
        request_id_t request_id = new_request_id();
        message_t msg = message_t::create(Handler::id,
                                          request_id,
                                          std::forward<Args>(args)...);
        sync_request_t request(this, request_id);
        return request.run(std::move(msg)).deserialize<result_t>();
    }
    */

    template <class Handler, typename... Args>
    void noreply_call(Args &&...args) {
        write_message_t msg = write_message_t::create(Handler::handler_id(),
                                                      request_id_t::noreply(),
                                                      std::forward<Args>(args)...);
        stream.write(std::move(msg));
    }

    /*
    class sync_request_t {
    public:
        sync_request_t(target_t *_parent, request_id_t request_id);
        ~sync_request_t();
        message_t run(message_t &&msg);
    private:
        target_t *parent;
        request_id_t request_id;
        cond_t done_cond;
    };
    // TODO: better data structure
    std::map<request_id_t, signal_t *> waiters;
    */

    tcp_stream_t stream;
};

#endif // TARGET_HPP_
