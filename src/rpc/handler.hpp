#ifndef RPC_HANDLER_HPP_
#define RPC_HANDLER_HPP_

#include <tuple>
#include <utility>

#include "rpc/hub.hpp"
#include "rpc/message.hpp"
#include "rpc/serialize.hpp"

namespace indecorous {

class read_message_t;
class message_hub_t;

class handler_callback_t {
public:
    virtual ~handler_callback_t();
    virtual write_message_t handle(read_message_t *msg) = 0;
    virtual void handle_noreply(read_message_t *msg) = 0;
    virtual handler_id_t id() const = 0;
};

template <class T>
class unique_handler_t {
public:
    static handler_id_t handler_id() {
        return unique_id;
    }
private:
    static const handler_id_t unique_id;
};

template <typename Impl>
class handler_t : public unique_handler_t<Impl> {
public:
    handler_t(message_hub_t *hub) : membership(hub, &internal_handler) { }

    template <typename Res, typename... Args>
    class internal_handler_t : public handler_callback_t {
    public:
        write_message_t handle(read_message_t *msg) {
            Res res = handle(std::index_sequence_for<Args...>{},
                             std::tuple<Args...>{serializer_t<Args>::read(msg)...});
            return write_message_t(handler_id_t::reply(),
                                   msg->request_id,
                                   std::move(res));
        }

        void handle_noreply(read_message_t *msg) {
            handle(std::index_sequence_for<Args...>{},
                   std::tuple<Args...>{serializer_t<Args>::read(msg)...});
        }

        handler_id_t id() const {
            return handler_t<Impl>::handler_id();
        }

        // The client calls this to error at compilation if the args don't line up
        static void check_args(Args &&...) { }

    private:
        template <size_t... N>
        static Res handle(std::integer_sequence<size_t, N...>, std::tuple<Args...> &&args) {
            return Impl::call(std::move(std::get<N>(args))...);
        }
    };

    // Used to convert a function to a parameter pack of result and arg types
    template <typename Res, typename... Args>
    static internal_handler_t<Res, Args...> dummy_translator(Res(*fn)(Args...));

    typedef decltype(dummy_translator(Impl::call)) handler_impl_t;
    handler_impl_t internal_handler;

    message_hub_t::membership_t<handler_callback_t> membership;
};

} // namespace indecorous

#endif // HANDLER_HPP_
