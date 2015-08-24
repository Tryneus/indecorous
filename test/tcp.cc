#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"
#include "rpc/handler.hpp"
#include "rpc/target.hpp"
#include "sync/event.hpp"
#include "io/tcp.hpp"

const size_t num_threads = 2;

using namespace indecorous;

struct tcp_test_t {
    DECLARE_STATIC_RPC(tcp_test_t, client, uint16_t server_port) -> void;
    DECLARE_STATIC_RPC(tcp_test_t, server_loop) -> void;
    DECLARE_STATIC_RPC(tcp_test_t, resolve, std::string host) -> std::vector<ip_address_t>;
};

IMPL_STATIC_RPC(tcp_test_t::client, uint16_t server_port) -> void {
    tcp_conn_t conn(ip_and_port_t::loopback(server_port), nullptr);
    uint64_t val = 142;
    conn.write(&val, sizeof(val), nullptr);
}

IMPL_STATIC_RPC(tcp_test_t::server_loop) -> void {
    event_t done_event;
    tcp_listener_t listener(0,
        [&] (tcp_conn_t conn, drainer_lock_t) {
            uint64_t val;
            conn.read(&val, sizeof(val), &done_event);
        });
    
    thread_t::self()->hub()->broadcast_local_sync<client_t>(listener.local_port());
    done_event.wait();
}

IMPL_STATIC_RPC(tcp_test_t::resolve, std::string host) -> std::vector<ip_address_t> {
    return resolve_hostname(host);
}

INDECOROUS_UNIQUE_RPC(tcp_test_t::client);
INDECOROUS_UNIQUE_RPC(tcp_test_t::server_loop);
INDECOROUS_UNIQUE_RPC(tcp_test_t::serve);
INDECOROUS_UNIQUE_RPC(tcp_test_t::serve);

TEST_CASE("tcp/basic", "[tcp]") {
    scheduler_t sched(2, shutdown_policy_t::Eager);
    sched.threads().begin()->hub()->self_target()->call_noreply<server_t>();
    sched.run();
}

TEST_CASE("tcp/resolve", "[tcp][dns]") {
    scheduler_t sched(2, shutdown_policy_t::Eager);
    sched.threads().begin()->hub()->self_target()->call_noreply<resolve_t>();
    sched.run();
}
