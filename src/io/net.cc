#include "io/net.hpp"

#include "udns.h"

#include "common.hpp"
#include "rpc/message.hpp"
#include "sync/drainer.hpp"
#include "sync/file_wait.hpp"
#include "sync/multiple_wait.hpp"
#include "sync/mutex.hpp"
#include "sync/timer.hpp"

namespace indecorous {

size_t serializer_t<in6_addr>::size(const in6_addr &item) {
    return sizeof(item.s6_addr);
}

int serializer_t<in6_addr>::write(write_message_t *msg, const in6_addr &item) {
    for (size_t i = 0; i < sizeof(item.s6_addr); ++i) {
        msg->push_back(item.s6_addr[i]);
    }
    return 0;
}

in6_addr serializer_t<in6_addr>::read(read_message_t *msg) {
    in6_addr res;
    for (size_t i = 0; i < sizeof(res.s6_addr); ++i) {
        res.s6_addr[i] = msg->pop();
    }
    return res;
}

IMPL_SERIALIZABLE(ip_address_t, m_addr, m_scope_id);
IMPL_SERIALIZABLE(ip_and_port_t, m_addr, m_port);

auto udns_init_result = [&] {
        int res = dns_init(nullptr, false);
        GUARANTEE(res == 0);
        return res;
    }();

class udns_ctx_t {
private:
    enum class result_t { Pending, Error, Success };
    class resolve_data_t {
    public:
        resolve_data_t() : addrs(), result(result_t::Pending) { }

        std::vector<ip_address_t> addrs;
        result_t result;
    private:
        DISABLE_COPYING(resolve_data_t);
    };
public:
    udns_ctx_t() :
            m_ctx(dns_new(nullptr)),
            m_mutex(),
            m_drainer() {
        dns_open(m_ctx);
        assert(dns_sock(m_ctx) != -1);
    }

    ~udns_ctx_t() {
        dns_close(m_ctx);
        dns_free(m_ctx);
    }

    std::vector<ip_address_t> resolve(const std::string &host) {
        drainer_lock_t drain = m_drainer.lock();
        resolve_data_t data;
        GUARANTEE(dns_submit_a6(m_ctx, host.c_str(), 0,
                                &udns_ctx_t::resolve_callback_ipv6, &data) != nullptr);

        // Only one coroutine should be calling into these functions at a time
        mutex_lock_t lock = m_mutex.lock();
        single_timer_t timer;
        file_wait_t in = file_wait_t::in(dns_sock(m_ctx));
        while (data.result == result_t::Pending) {
            time_t current_time = time(nullptr);
            int timeout = dns_timeouts(m_ctx, 10, current_time);
            assert(timeout != -1);
            timer.start(timeout * 1000);
            dns_ioevent(m_ctx, current_time);

            if (data.result != result_t::Pending) break; // TODO: simplify loop

            wait_any(timer, in);
        }

        if (data.result == result_t::Error) {
            throw std::runtime_error("DNS lookup failed..."); // TODO: more errors
        }

        return std::move(data.addrs);
    }

private:
    static void resolve_callback_ipv4(dns_ctx *ctx, dns_rr_a4 *result, void *param) {
        resolve_data_t *out = reinterpret_cast<resolve_data_t *>(param);
        if (result == nullptr) {
            switch (dns_status(ctx)) {
            case DNS_E_TEMPFAIL: debugf("DNS_E_TEMPFAIL"); break;
            case DNS_E_PROTOCOL: debugf("DNS_E_PROTOCOL"); break;
            case DNS_E_NXDOMAIN: debugf("DNS_E_NXDOMAIN"); break;
            case DNS_E_NODATA: debugf("DNS_E_NODATA"); break;
            case DNS_E_NOMEM: debugf("DNS_E_NOMEM"); break;
            case DNS_E_BADQUERY: debugf("DNS_E_BADQUERY"); break;
            default: debugf("DNS_E_UNKNOWN"); break;
            }
            out->result = result_t::Error;
        } else {
            assert(dns_status(ctx) == 0);
            debugf("Got %d IPv4 addresses", result->dnsa4_nrr);
            out->result = result_t::Success;
        }
    }

    static void resolve_callback_ipv6(dns_ctx *ctx, dns_rr_a6 *result, void *param) {
        resolve_data_t *out = reinterpret_cast<resolve_data_t *>(param);
        if (result == nullptr) {
            switch (dns_status(ctx)) {
            case DNS_E_TEMPFAIL: debugf("DNS_E_TEMPFAIL"); break;
            case DNS_E_PROTOCOL: debugf("DNS_E_PROTOCOL"); break;
            case DNS_E_NXDOMAIN: debugf("DNS_E_NXDOMAIN"); break;
            case DNS_E_NODATA: debugf("DNS_E_NODATA"); break;
            case DNS_E_NOMEM: debugf("DNS_E_NOMEM"); break;
            case DNS_E_BADQUERY: debugf("DNS_E_BADQUERY"); break;
            default: debugf("DNS_E_UNKNOWN"); break;
            }
            out->result = result_t::Error;
        } else {
            assert(dns_status(ctx) == 0);
            debugf("Got %d IPv6 addresses", result->dnsa6_nrr);
            for (int i = 0; i < result->dnsa6_nrr; ++i) {
                out->addrs.push_back(ip_address_t(result->dnsa6_addr[i], 0));
            }
            out->result = result_t::Success;
        }
    }

    dns_ctx *m_ctx;
    mutex_t m_mutex;
    drainer_t m_drainer;

    DISABLE_COPYING(udns_ctx_t);
};

std::vector<ip_address_t> resolve_hostname(const std::string &host) {
    // TODO: try to resolve the host as an ip address first
    udns_ctx_t ctx;
    return ctx.resolve(host);
}

//ip_address_t::ip_address_t(const in6_addr &addr, uint32_t scope_id) :
//    m_addr(addr), m_scope_id(scope_id) { }

ip_address_t ip_address_t::loopback() {
    return ip_address_t(in6addr_loopback, 0);
}

ip_address_t ip_address_t::any() {
    return ip_address_t(in6addr_any, 0);
}

bool ip_address_t::is_loopback() const {
    return IN6_IS_ADDR_LOOPBACK(&m_addr);
}

bool ip_address_t::is_any() const {
    return IN6_IS_ADDR_UNSPECIFIED(&m_addr);
}

ip_address_t ip_address_t::from_sockaddr(sockaddr_in6 *sa) {
    return ip_address_t(sa->sin6_addr, sa->sin6_scope_id);
}

void ip_address_t::to_sockaddr(sockaddr_in6 *sa) const {
    sa->sin6_family = AF_INET6;
    sa->sin6_flowinfo = 0;
    sa->sin6_addr = m_addr;
    sa->sin6_scope_id = m_scope_id;
}

ip_and_port_t::ip_and_port_t(const in6_addr &_addr, uint32_t scope_id, uint16_t _port) :
    m_addr(_addr, scope_id), m_port(_port) { }

ip_and_port_t ip_and_port_t::loopback(uint16_t port) {
    return ip_and_port_t(in6addr_loopback, 0, port);
}

ip_and_port_t ip_and_port_t::any(uint16_t port) {
    return ip_and_port_t(in6addr_any, 0, port);
}

ip_and_port_t ip_and_port_t::from_sockaddr(sockaddr_in6 *sa) {
    return ip_and_port_t(sa->sin6_addr, sa->sin6_scope_id, sa->sin6_port);
}

void ip_and_port_t::to_sockaddr(sockaddr_in6 *sa) const {
    m_addr.to_sockaddr(sa);
    sa->sin6_port = m_port;
}

const ip_address_t &ip_and_port_t::addr() const {
    return m_addr;
}

uint16_t ip_and_port_t::port() const {
    return m_port;
}

} // namespace indecorous
