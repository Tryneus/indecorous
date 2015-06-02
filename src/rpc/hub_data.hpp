#ifndef RPC_HUB_DATA_HPP_
#define RPC_HUB_DATA_HPP_

#include <string>
#include <unordered_map>

#include "rpc/handler.hpp"
#include "rpc/id.hpp"

namespace indecorous {

struct hub_data_t {
    static std::unordered_map<handler_id_t, handler_callback_t *> s_handlers;

    static void auto_register(handler_callback_t *cb) {
        s_handlers.emplace(cb->id(), cb);
    }
};

template <typename T>
struct hub_registration_t {
    hub_registration_t() {
        static handler_wrapper_t<T> handler = handler_wrapper_t<T>();
        hub_data_t::auto_register(&handler.internal_handler);
    }
};

template <class T>
class handler_t {
public:
    static handler_id_t handler_id() {
        return s_unique_id;
    }
    static handler_id_t id_from_name(std::string name) {
        return handler_id_t(std::hash<std::string>()(name));
    }
private:
    static const hub_registration_t<T> s_hub_registration;
    static const handler_id_t s_unique_id;
};

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)

// Note: this doesn't allow for templatized handler types
#define IMPL_UNIQUE_HANDLER(Type) \
    template <> const indecorous::handler_id_t handler_t<Type>::s_unique_id = handler_t<Type>::id_from_name(__FILE__ ":" STRINGIFY(__LINE__) ":" #Type); \
    template <> const indecorous::hub_registration_t<Type> handler_t<Type>::s_hub_registration = indecorous::hub_registration_t<Type>()

} // namespace indecorous

#endif // RPC_HUB_DATA_HPP_
