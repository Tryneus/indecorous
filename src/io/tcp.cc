#include "io/tcp.hpp"

#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>

namespace indecorous {

tcp_conn_t::tcp_conn_t(fd_t sock) :
        m_socket(sock),
        m_in(file_wait_t::in(m_socket.get())),
        m_out(file_wait_t::out(m_socket.get())),
        m_read_buffer_offset(0) {
    m_read_buffer.reserve(s_read_buffer_size);
}

tcp_conn_t::tcp_conn_t(ip_and_port_t &ip_port, wait_object_t *interruptor) :
        m_socket(init_socket(ip_port, interruptor)),
        m_in(file_wait_t::in(m_socket.get())),
        m_out(file_wait_t::out(m_socket.get())),
        m_read_buffer_offset(0) {
    m_read_buffer.reserve(s_read_buffer_size);
}

void tcp_conn_t::read(char *buf, size_t count, wait_object_t *interruptor) {
    mutex_lock_t lock(&m_read_mutex);

    while (count > 0) {
        size_t bytes_to_copy =
            std::min<size_t>(count, m_read_buffer.size() - m_read_buffer_offset);
        while (bytes_to_copy > 0) {
            --count;
            *buf++ = m_read_buffer[m_read_buffer_offset++];
        }

        if (count > s_read_buffer_size / 2) {
            ssize_t res = eintr_wrap([&] { return ::read(m_socket.get(), buf, count); },
                                     &m_in, interruptor);
            count -= res;
            buf += res;
        } else {
            read_into_buffer(interruptor);
        }
    }
}

size_t tcp_conn_t::read_until(char delim, char *buf, size_t count, wait_object_t *interruptor) {
    mutex_lock_t lock(&m_read_mutex);
    size_t original_count = count;

    while (count > 0) {
        size_t bytes_to_copy =
            std::min<size_t>(count, m_read_buffer.size() - m_read_buffer_offset);
        while (bytes_to_copy > 0) {
            --count;
            *buf++ = m_read_buffer[m_read_buffer_offset++];
            if (*(buf - 1) == delim) { return original_count - count; }
        }
        read_into_buffer(interruptor);
    }
    return original_count;
}

void tcp_conn_t::write(char *buf, size_t count, wait_object_t *interruptor) {
    mutex_lock_t lock(&m_write_mutex);

    while (count > 0) {
        ssize_t res = eintr_wrap([&] { return ::write(m_socket.get(), buf, count); },
                                 &m_out, interruptor);
        count -= res;
        buf += res;
    }
}

scoped_fd_t tcp_conn_t::init_socket(const ip_and_port_t &, wait_object_t *) {
    return scoped_fd_t(-1);
}

void tcp_conn_t::read_into_buffer(wait_object_t *interruptor) {
    assert(m_read_buffer_offset == m_read_buffer.size());
    ssize_t res = 0;
    m_read_buffer_offset = 0;
    m_read_buffer.resize(s_read_buffer_size);
                         
    while (res == 0) {
        res = eintr_wrap([&] { return ::read(m_socket.get(),
                                             &m_read_buffer[0], s_read_buffer_size); },
                         &m_in, interruptor);
    }
    m_read_buffer.resize(res);
}

uint16_t tcp_listener_t::local_port() {
    if (m_local_port == 0) {
        sockaddr_in6 sa;
        socklen_t sa_len = sizeof(sa);
        int res = getsockname(m_socket.get(), (sockaddr *)&sa, &sa_len);
        assert(res == 0);
        assert(sa.sin6_family == AF_INET6);
        assert(sa.sin6_port != 0);
        m_local_port = sa.sin6_port;
    }
    return m_local_port;
}

scoped_fd_t tcp_listener_t::init_socket() {
    scoped_fd_t sock(::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
    assert(sock.valid());

    sockaddr_in6 sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_port = m_local_port;
    sa.sin6_addr = in6addr_any;
    int res = ::bind(sock.get(), (sockaddr *)&sa, sizeof(sa));
    assert(res == 0);

    res = ::listen(sock.get(), 10);
    assert(res == 0);

    return sock;
}

void tcp_listener_t::accept_loop(drainer_lock_t drain) {
    try {
        file_wait_t in = file_wait_t::in(m_socket.get());
        while (true) {
            sockaddr_in6 sa;
            socklen_t sa_len = sizeof(sa);
            in.wait();       
            m_on_connect(tcp_conn_t(::accept(m_socket.get(), (sockaddr *)&sa, &sa_len)), drain);
        }
    } catch (const wait_interrupted_exc_t &ex) {
        assert(drain.draining());
    }
}

} // namespace indecorous
