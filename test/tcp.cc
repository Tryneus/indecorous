#include "catch.hpp"

#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"
#include "rpc/handler.hpp"
#include "rpc/hub_data.hpp"
#include "rpc/target.hpp"
#include "sync/event.hpp"
#include "io/tcp.hpp"

const size_t num_threads = 2;

using namespace indecorous;

struct client_t : public handler_t<client_t> {
    static void call(uint16_t server_port) {
        debugf("client_t");
        tcp_conn_t conn(ip_and_port_t::loopback(server_port), nullptr);
        uint64_t val = 142;
        conn.write(&val, sizeof(val), nullptr);
        debugf("client wrote val: %" PRIu64, val);
    }
};
IMPL_UNIQUE_HANDLER(client_t);

struct server_t : public handler_t<server_t> {
    static void call() {
        debugf("server_t");
        event_t done_event;
        tcp_listener_t listener(0,
            [&] (tcp_conn_t conn, drainer_lock_t) {
                handle_conn(std::move(conn), &done_event);
            });
        
        thread_t::self()->hub()->broadcast_local_noreply<client_t>(listener.local_port());
        done_event.wait();
    }

    static void handle_conn(tcp_conn_t conn, event_t *done_event) {
        uint64_t val;
        conn.read(&val, sizeof(val), done_event);
        debugf("server got val: %" PRIu64, val);
    }
};
IMPL_UNIQUE_HANDLER(server_t);

TEST_CASE("tcp/basic", "[tcp]") {
    scheduler_t sched(2, shutdown_policy_t::Eager);
    sched.threads().begin()->hub()->self_target()->call_noreply<server_t>();
    sched.run();
}

struct resolve_t : public handler_t<resolve_t> {
    static void call() {
        std::vector<ip_address_t> addrs = resolve_hostname("localhost");
    }
};
IMPL_UNIQUE_HANDLER(resolve_t);

TEST_CASE("tcp/resolve", "[tcp][dns]") {
    scheduler_t sched(2, shutdown_policy_t::Eager);
    sched.threads().begin()->hub()->self_target()->call_noreply<resolve_t>();
    sched.run();
}
