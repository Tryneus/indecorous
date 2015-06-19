#ifndef IO_TCP_HPP_
#define IO_TCP_HPP_

#include <netinet/in.h>

#include <cstddef>
#include <functional>
#include <vector>

#include "containers/file.hpp"
#include "coro/coro.hpp"
#include "io/base.hpp"
#include "sync/drainer.hpp"
#include "sync/file_wait.hpp"
#include "sync/mutex.hpp"

namespace indecorous {

class wait_object_t;

class ip_address_t {
public:
    static ip_address_t localhost();

private:
    in6_addr addr;
    uint32_t scope_id;
};

class ip_and_port_t {
public:
    static ip_and_port_t localhost(uint16_t port);

    void to_sockaddr(sockaddr_in6 *out) const;
private:
    ip_address_t addr;
    uint16_t port;
};

class tcp_conn_t : public read_stream_t, public write_stream_t {
public:
    tcp_conn_t(fd_t sock);
    tcp_conn_t(const ip_and_port_t &host_port, wait_object_t *interruptor);
    tcp_conn_t(tcp_conn_t &&other);

    void read(void *buf, size_t count, wait_object_t *interruptor);
    size_t read_until(char delim, void *buf, size_t count, wait_object_t *interruptor);
    void write(void *buf, size_t count, wait_object_t *interruptor);

private:
    scoped_fd_t init_socket(const ip_and_port_t &ip_port);
    void read_into_buffer(wait_object_t *interruptor);

    scoped_fd_t m_socket;
    file_wait_t m_in;
    file_wait_t m_out;

    mutex_t m_read_mutex;
    mutex_t m_write_mutex;

    std::vector<char> m_read_buffer;
    size_t m_read_buffer_offset;

    static const size_t s_read_buffer_size;
};

// TODO: specify 0 or 1 interface to listen on?  multiple?
class tcp_listener_t {
public:
    tcp_listener_t(uint16_t _local_port, std::function<void (tcp_conn_t, drainer_lock_t)> on_connect);
    tcp_listener_t(tcp_listener_t &&other);

    uint16_t local_port();

private:
    scoped_fd_t init_socket();

    uint16_t m_local_port;
    scoped_fd_t m_socket;
    drainer_t m_drainer;
};

} // namespace indecorous

#endif // IO_TCP_HPP_
