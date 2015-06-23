#ifndef RPC_HANDLER_HPP_
#define RPC_HANDLER_HPP_

#include <tuple>
#include <utility>
#include <type_traits>

#include "rpc/message.hpp"
#include "rpc/serialize.hpp"

namespace indecorous {

class message_hub_t;

class handler_callback_t {
public:
    virtual ~handler_callback_t();
    virtual void handle(message_hub_t *hub, read_message_t msg) = 0;
    virtual void handle_noreply(read_message_t msg) = 0;
    virtual handler_id_t id() const = 0;
};

void send_reply(message_hub_t *hub, target_id_t source_id, write_message_t msg);

template <typename Callback>
class handler_wrapper_t {
public:
    template <typename Res, typename... Args>
    class internal_handler_t : public handler_callback_t {
    public:
        typedef Res result_t;
        void handle(message_hub_t *hub, read_message_t msg) {
            assert(msg.buffer.has());
            Res res = handle_internal(std::index_sequence_for<Args...>{},
                                      std::tuple<Args...>{serializer_t<Args>::read(&msg)...});
            send_reply(hub, msg.source_id,
                       write_message_t::create(msg.source_id,
                           handler_id_t::reply(),
                           msg.request_id,
                           std::move(res)));
        }

        void handle_noreply(read_message_t msg) {
            assert(msg.buffer.has());
            handle_internal(std::index_sequence_for<Args...>{},
                            std::tuple<Args...>{serializer_t<Args>::read(&msg)...});
        }

        handler_id_t id() const {
            return Callback::handler_id();
        }

        static write_message_t make_write(target_id_t target, request_id_t request_id, Args &&...args) {
            return write_message_t::create(target,
                                           Callback::handler_id(),
                                           request_id,
                                           std::forward<Args>(args)...);
        }

    private:
        template <size_t... N>
        static Res handle_internal(std::integer_sequence<size_t, N...>,
                                   std::tuple<Args...> &&args) {
            return Callback::call(std::move(std::get<N>(args))...);
        }
    };

    // Predeclare the void specialization or this breaks on clang
    template <typename... Args> class internal_handler_t<void, Args...>;

    // Used to convert a function to a parameter pack of result and arg types
    template <typename Res, typename... Args>
    static internal_handler_t<Res, Args...> dummy_translator(Res(*fn)(Args...));

    typedef decltype(dummy_translator(Callback::call)) handler_impl_t;
    handler_impl_t internal_handler;
    typedef typename handler_impl_t::result_t result_t;
};

// Specialization for handlers with a void return type
template <typename Callback>
template <typename... Args>
class handler_wrapper_t<Callback>::internal_handler_t<void, Args...> : public handler_callback_t {
public:
    typedef void result_t;
    void handle(message_hub_t *hub, read_message_t msg) {
        assert(msg.buffer.has());
        handle_internal(std::index_sequence_for<Args...>{},
                        std::tuple<Args...>{serializer_t<Args>::read(&msg)...});
        send_reply(hub, msg.source_id,
                   write_message_t::create(msg.source_id,
                       handler_id_t::reply(),
                       msg.request_id));
    }

    void handle_noreply(read_message_t msg) {
        assert(msg.buffer.has());
        handle_internal(std::index_sequence_for<Args...>{},
                        std::tuple<Args...>{serializer_t<Args>::read(&msg)...});
    }

    handler_id_t id() const {
        return Callback::handler_id();
    }

    static write_message_t make_write(target_id_t target, request_id_t request_id, Args &&...args) {
        return write_message_t::create(target,
                                       Callback::handler_id(),
                                       request_id,
                                       std::forward<Args>(args)...);
    }

private:
    template <size_t... N>
    static void handle_internal(std::integer_sequence<size_t, N...>,
                                std::tuple<Args...> &&args) {
        Callback::call(std::move(std::get<N>(args))...);
    }
};

} // namespace indecorous

#endif // HANDLER_HPP_
