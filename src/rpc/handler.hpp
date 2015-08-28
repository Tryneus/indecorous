#ifndef RPC_HANDLER_HPP_
#define RPC_HANDLER_HPP_

#include <tuple>
#include <utility>
#include <type_traits>
#include <unordered_map>

#include "rpc/message.hpp"
#include "rpc/serialize.hpp"

namespace indecorous {

class message_hub_t;

struct rpc_callback_t {
    virtual ~rpc_callback_t() { }
    virtual write_message_t handle(read_message_t msg) = 0;
    virtual void handle_noreply(read_message_t msg) = 0;
    virtual rpc_id_t id() const = 0;
};

// TODO: put this someplace sensical
std::unordered_map<rpc_id_t, rpc_callback_t *> &register_callback(rpc_callback_t *cb);

template <typename T>
struct static_rpc_t : public rpc_callback_t {
    static_rpc_t() { }
    write_message_t handle(read_message_t msg) final {
        return T::static_handle(std::move(msg));
    };
    void handle_noreply(read_message_t msg) final {
        T::static_handle_noreply(std::move(msg));
    }
    rpc_id_t id() const final {
        return T::s_rpc_id;
    }
};


template <typename T>
struct static_rpc_registration_t {
    static_rpc_registration_t() {
        static static_rpc_t<T> rpc = static_rpc_t<T>();
        register_callback(&rpc);
    }
};

template <typename... Args>
struct write_generator_t {
    static write_message_t make(target_id_t src,
                                rpc_id_t rpc,
                                request_id_t req,
                                Args &&...args) {
        return write_message_t::create(src, rpc, req, std::forward<Args>(args)...);
    }
};

template <typename Res, typename... Args>
struct rpc_bridge_t {
    typedef Res result_t;
    typedef write_generator_t<Args...> write_t;
};

// Dummy functions to get the return types of member functions
template <typename Class, typename Res, typename... Args>
rpc_bridge_t<Res, Args...> rpc_bridge(Res(Class::*fn)(Args...));
template <typename Res, typename... Args>
rpc_bridge_t<Res, Args...> rpc_bridge(Res(*fn)(Args...));

#define INDECOROUS_STRINGIFY_INTERNAL(x) #x
#define INDECOROUS_STRINGIFY(x) INDECOROUS_STRINGIFY_INTERNAL(x)

// Note: this doesn't allow for templatized rpc types
// TODO: '__FILE__' is probably not safe for cross-compiler or build environment compatibility
#define INDECOROUS_UNIQUE_RPC(RPC) \
    const indecorous::rpc_id_t RPC::s_rpc_id = \
        rpc_id_t(std::hash<std::string>()(__FILE__ ":" INDECOROUS_STRINGIFY(__LINE__) ":" #RPC));

#define DECLARE_MEMBER_RPC(Class, RPC, ...) \
    struct RPC : public indecorous::rpc_callback_t { \
        RPC(Class *parent); \
        ~RPC(); \
        indecorous::write_message_t handle(indecorous::read_message_t msg) final; \
        void handle_noreply(indecorous::read_message_t msg) final; \
        indecorous::rpc_id_t id() const final; \
        static auto fn_ptr() { return &Class::RPC ## _indecorous_callback; } \
        static const indecorous::rpc_id_t s_rpc_id; \
        Class * const m_parent; \
    } RPC ## _indecorous_rpc = RPC(this); \
    auto RPC ## _indecorous_callback(__VA_ARGS__)

#define IMPL_MEMBER_RPC(Class, RPC, ...) \
    INDECOROUS_UNIQUE_RPC(Class::RPC); \
    Class::RPC::RPC(Class *parent) : m_parent(parent) { \
        indecorous::thread_t::self()->hub()->add_member_rpc(s_rpc_id, this); \
    } \
    Class::RPC::~RPC() { \
        indecorous::thread_t::self()->hub()->remove_member_rpc(s_rpc_id); \
    } \
    indecorous::write_message_t Class::RPC::handle(indecorous::read_message_t msg) final { \
        return indecorous::do_member_rpc(&Class::RPC ## _indecorous_callback, m_parent, std::move(msg)); \
    } \
    void Class::RPC::handle_noreply(indecorous::read_message_t msg) final { \
        indecorous::do_member_rpc_noreply(&Class::RPC ## _indecorous_callback, m_parent, std::move(msg)); \
    } \
    indecorous::rpc_id_t Class::RPC::id() const final { \
        return s_rpc_id; \
    } \
    auto Class::RPC ## _indecorous_callback(__VA_ARGS__)

#define DECLARE_STATIC_RPC(RPC, ...) \
    struct RPC : public indecorous::static_rpc_t<RPC> { \
        RPC() = delete; \
        static indecorous::write_message_t static_handle(indecorous::read_message_t msg); \
        static void static_handle_noreply(indecorous::read_message_t msg); \
        static auto fn_ptr() { return &RPC ## _indecorous_callback; } \
        static const indecorous::rpc_id_t s_rpc_id; \
        static const static_rpc_registration_t<RPC> s_registration; \
    }; \
    static auto RPC ## _indecorous_callback(__VA_ARGS__)

#define IMPL_STATIC_RPC(RPC, ...) \
    INDECOROUS_UNIQUE_RPC(RPC); \
    indecorous::write_message_t RPC::static_handle(indecorous::read_message_t msg) { \
        return indecorous::do_static_rpc(&RPC ## _indecorous_callback, \
                                         std::move(msg)); \
    } \
    void RPC::static_handle_noreply(indecorous::read_message_t msg) { \
        indecorous::do_static_rpc_noreply(&RPC ## _indecorous_callback, \
                                          std::move(msg)); \
    } \
    const indecorous::static_rpc_registration_t<RPC> RPC::s_registration = \
        indecorous::static_rpc_registration_t<RPC>(); \
    auto RPC ## _indecorous_callback(__VA_ARGS__)

// Combine this arg-forwarding code with coro.hpp for smaller binaries?
template <typename Res, size_t... N, typename... Args>
Res call_static_with_args(Res (*fn)(Args...), std::tuple<Args...> &&args,
                   std::integer_sequence<size_t, N...>) {
    return fn(std::get<N>(std::move(args))...);
}

template <typename Res, size_t... N, typename Class, typename... Args>
Res call_member_with_args(Class *c, Res (Class::*fn)(Args...), std::tuple<Args...> &&args,
                          std::integer_sequence<size_t, N...>) {
    return c->*fn(std::get<N>(std::move(args))...);
}

template <typename Res, typename... Args>
typename std::enable_if<!std::is_same<Res, void>::value, write_message_t>::type
do_static_rpc(Res (*fn)(Args...), read_message_t msg) {
    std::tuple<Args...> args { serializer_t<Args>::read(&msg)... };
    Res res = call_static_with_args(fn, std::move(args),
        std::make_index_sequence<std::tuple_size<decltype(args)>::value>{});
    return write_message_t::create(msg.source_id,
                                   rpc_id_t::reply(),
                                   msg.request_id,
                                   std::move(res));
}

template <typename Class, typename Res, typename... Args>
typename std::enable_if<!std::is_same<Res, void>::value, write_message_t>::type
do_member_rpc(Class *instance, Res (Class::*fn)(Args...), read_message_t msg) {
    std::tuple<Args...> args { serializer_t<Args>::read(&msg)... };
    Res res = call_member_with_args(instance, fn, std::move(args),
        std::make_index_sequence<std::tuple_size<decltype(args)>::value>{});
    return write_message_t::create(msg.source_id,
                                   rpc_id_t::reply(),
                                   msg.request_id,
                                   std::move(res));
}

template <typename Res, typename... Args>
typename std::enable_if<std::is_same<Res, void>::value, write_message_t>::type
do_static_rpc(Res (*fn)(Args...), read_message_t msg) {
    std::tuple<Args...> args { serializer_t<Args>::read(&msg)... };
    call_static_with_args(fn, std::move(args),
        std::make_index_sequence<std::tuple_size<decltype(args)>::value>{});
    return write_message_t::create(msg.source_id,
                                   rpc_id_t::reply(),
                                   msg.request_id);
}

template <typename Class, typename Res, typename... Args>
typename std::enable_if<std::is_same<Res, void>::value, write_message_t>::type
do_member_rpc(Class *instance, Res (Class::*fn)(Args...), read_message_t msg) {
    std::tuple<Args...> args { serializer_t<Args>::read(&msg)... };
    call_member_with_args(instance, fn, std::move(args),
        std::make_index_sequence<std::tuple_size<decltype(args)>::value>{});
    return write_message_t::create(msg.source_id,
                                   rpc_id_t::reply(),
                                   msg.request_id);
}

template <typename Res, typename... Args>
void do_static_rpc_noreply(Res (*fn)(Args...), read_message_t msg) {
    std::tuple<Args...> args { serializer_t<Args>::read(&msg)... };
    call_static_with_args(fn, std::move(args),
        std::make_index_sequence<std::tuple_size<decltype(args)>::value>{});
}
template <typename Class, typename Res, typename... Args>
void do_member_rpc_noreply(Class *instance, Res (Class::*fn)(Args...), read_message_t msg) {
    std::tuple<Args...> args { serializer_t<Args>::read(&msg)... };
    call_member_with_args(instance, fn, std::move(args),
        std::make_index_sequence<std::tuple_size<decltype(args)>::value>{});
}

} // namespace indecorous

#endif // HANDLER_HPP_
