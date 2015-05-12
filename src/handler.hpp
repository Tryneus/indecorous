#ifndef HANDLER_HPP_
#define HANDLER_HPP_

#include <tuple>
#include <utility>

#include "message.hpp"
#include "serialize.hpp"

class read_message_t;
class message_hub_t;

class message_callback_t {
public:
    message_callback_t(message_hub_t *hub);
    virtual ~message_callback_t();
    virtual write_message_t handle(read_message_t *msg) = 0;
    virtual void handle_noreply(read_message_t *msg) = 0;

    virtual handler_id_t id() const = 0;
private:
    message_hub_t *parent_hub;
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

template <typename Impl, typename Res, typename... Args>
class handler_t : public message_callback_t, public unique_handler_t<Impl> {
public:
    handler_t(message_hub_t *hub) : message_callback_t(hub) { }

    write_message_t handle(read_message_t *msg) {
        Res res = handle(std::index_sequence_for<Args...>{},
                         std::tuple<Args...>{deserialize<Args>(msg)...});
        return write_message_t(handler_id_t::reply(),
                               msg->request_id,
                               std::move(res));
    }

    void handle_noreply(read_message_t *msg) {
        handle(std::index_sequence_for<Args...>{},
               std::tuple<Args...>{deserialize<Args>(msg)...});
    }

    handler_id_t id() const {
        return this->handler_id();
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

#endif // HANDLER_HPP_
