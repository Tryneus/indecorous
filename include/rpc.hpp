#include <stdint.h>
#include <tuple>
#include <utility>

#include "include/id.hpp"
#include "include/message.hpp"
#include "include/serialize.hpp"
#include "include/serialize_stl.hpp"
#include "include/stream.hpp"

template <typename Impl, typename Res, typename... Args>
class base_handler_t {
public:
    static // TODO: make this not static
    Res handle(read_message_t *msg) {
        return handle(std::index_sequence_for<Args...>{}, parse<Args...>(msg));
    }

private:
    template <typename T>
    static std::tuple<T> parse(read_message_t *msg) {
        return std::tuple<T>(deserialize<T>(msg));
    }

    template <typename T, typename... Subargs>
    static std::tuple<T, Subargs...> parse(read_message_t *msg) {
        return std::tuple_cat(std::tuple<T>(deserialize<T>(msg)), parse<Subargs...>(msg));
    }

    template <size_t... N>
    static Res handle(std::integer_sequence<size_t, N...>, std::tuple<Args...> &&args) {
        return Impl::call(std::get<N>(args)...);
    }
};

class remote_target_t {
public:
    remote_target_t() : target_id(target_id_t::assign()) { }

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
        write_message_t msg = write_message_t::create(Handler::id,
                                                      request_id_t::noreply(),
                                                      std::forward<Args>(args)...);
        stream.write(std::move(msg));
    }

    /*
    class sync_request_t {
    public:
        sync_request_t(target_t *_parent, request_id_t request_id) :
                parent(_parent), id(request_id) {
            parent->waiters.insert(std::make_pair(id, &done_cond));
        }
        ~sync_request_t() {
            parent->waiters.erase(id);
        }
        message_t run(message_t &&msg) {
            parent->stream.write(std::move(msg));
            done_cond.wait();
        }
    private:
        target_t *parent;
        request_id_t request_id;
        cond_t done_cond;
    };
    // TODO: better data structure
    std::map<request_id_t, signal_t *> waiters;
    */

    fake_stream_t stream;
    target_id_t target_id;
};
