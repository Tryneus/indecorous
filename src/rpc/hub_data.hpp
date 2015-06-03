#ifndef RPC_HUB_DATA_HPP_
#define RPC_HUB_DATA_HPP_

#include <string>
#include <unordered_map>

#include "rpc/handler.hpp"
#include "rpc/id.hpp"

namespace indecorous {

std::unordered_map<handler_id_t, handler_callback_t *> &register_handler(handler_callback_t *cb);

template <typename T>
struct hub_registration_t {
    hub_registration_t() {
        static handler_wrapper_t<T> handler = handler_wrapper_t<T>();
        register_handler(&handler.internal_handler);
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

#define INDECOROUS_STRINGIFY_INTERNAL(x) #x
#define INDECOROUS_STRINGIFY(x) INDECOROUS_STRINGIFY_INTERNAL(x)

// Note: this doesn't allow for templatized handler types
#define IMPL_UNIQUE_HANDLER(Type) \
    template <> const indecorous::handler_id_t \
        handler_t<Type>::s_unique_id = \
            handler_t<Type>::id_from_name(__FILE__ ":" INDECOROUS_STRINGIFY(__LINE__) ":" #Type); \
    template <> const indecorous::hub_registration_t<Type> \
        handler_t<Type>::s_hub_registration = \
            indecorous::hub_registration_t<Type>()

} // namespace indecorous

#endif // RPC_HUB_DATA_HPP_
