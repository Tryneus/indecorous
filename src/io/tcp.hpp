#ifndef IO_TCP_HPP_
#define IO_TCP_HPP_

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

};

class ip_and_port_t {

};

class tcp_conn_t : public read_stream_t, public write_stream_t {
public:
    tcp_conn_t(fd_t sock);
    tcp_conn_t(ip_and_port_t &host_port, wait_object_t *interruptor);
    tcp_conn_t(tcp_conn_t &&other);

    void read(char *buf, size_t count, wait_object_t *interruptor);
    size_t read_until(char delim, char *buf, size_t count, wait_object_t *interruptor);
    void write(char *buf, size_t count, wait_object_t *interruptor);

private:
    scoped_fd_t init_socket(const ip_and_port_t &ip_port, wait_object_t *interruptor);
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
    template <typename Callable>
    tcp_listener_t(uint16_t _local_port, Callable on_connect) :
            m_local_port(_local_port),
            m_socket(init_socket()),
            m_on_connect(on_connect) {
        coro_t::spawn(&tcp_listener_t::accept_loop, this, m_drainer.lock());
    }

    uint16_t local_port();

private:
    scoped_fd_t init_socket();
    void accept_loop(drainer_lock_t drain);

    uint16_t m_local_port;
    scoped_fd_t m_socket;
    std::function<void (tcp_conn_t, drainer_lock_t)> m_on_connect;
    drainer_t m_drainer;
};

} // namespace indecorous

#endif // IO_TCP_HPP_
