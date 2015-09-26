#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"
#include "rpc/handler.hpp"
#include "rpc/target.hpp"
#include "sync/event.hpp"
#include "io/tcp.hpp"
#include "rpc/serialize_stl.hpp"

const size_t num_threads = 2;

using namespace indecorous;

struct tcp_test_t {
    DECLARE_STATIC_RPC(client)(uint16_t server_port) -> void;
    DECLARE_STATIC_RPC(server_loop)() -> void;
    DECLARE_STATIC_RPC(resolve)(std::string host) -> std::vector<ip_address_t>;
};

IMPL_STATIC_RPC(tcp_test_t::client)(uint16_t server_port) -> void {
    tcp_conn_t conn(ip_and_port_t::loopback(server_port));
    uint64_t val = 142;
    conn.write(&val, sizeof(val));
}

IMPL_STATIC_RPC(tcp_test_t::server_loop)() -> void {
    {
        event_t done_event;
        tcp_listener_t listener(0,
            [&] (tcp_conn_t conn, drainer_lock_t) {
                interruptor_t done(&done_event);
                uint64_t val;
                conn.read(&val, sizeof(val));
                done_event.set();
            });

        thread_t::self()->hub()->broadcast_local_sync<tcp_test_t::client>(listener.local_port());
        done_event.wait();
    }
}

TEST_CASE("tcp/basic", "[tcp]") {
    scheduler_t sched(4, shutdown_policy_t::Eager);
    sched.threads().begin()->hub()->self_target()->call_noreply<tcp_test_t::server_loop>();
    sched.run();
}

IMPL_STATIC_RPC(tcp_test_t::resolve)(std::string host) -> std::vector<ip_address_t> {
    return resolve_hostname(host);
}

TEST_CASE("tcp/resolve", "[tcp][dns][hide]") {
    scheduler_t sched(2, shutdown_policy_t::Eager);
    sched.threads().begin()->hub()->self_target()->call_noreply<tcp_test_t::resolve>("www.google.com");
    sched.run();
}
