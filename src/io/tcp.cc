#include "io/tcp.hpp"

#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>

namespace indecorous {

const size_t tcp_conn_t::s_read_buffer_size = 65536;

// TODO: this probably isn't safe if someone has a read or write lock (or is acquiring one)
tcp_conn_t::tcp_conn_t(tcp_conn_t &&other) :
    m_socket(std::move(other.m_socket)),
    m_in(std::move(other.m_in)),
    m_out(std::move(other.m_out)),
    m_read_mutex(std::move(other.m_read_mutex)),
    m_write_mutex(std::move(other.m_write_mutex)),
    m_read_buffer(std::move(other.m_read_buffer)),
    m_read_buffer_offset(other.m_read_buffer_offset) { }

tcp_conn_t::tcp_conn_t(scoped_fd_t sock) :
        m_socket(std::move(sock)),
        m_in(file_wait_t::in(m_socket.get())),
        m_out(file_wait_t::out(m_socket.get())),
        m_read_buffer_offset(0) {
    m_read_buffer.reserve(s_read_buffer_size);
}

tcp_conn_t::tcp_conn_t(const ip_and_port_t &ip_port, waitable_t *interruptor) :
        m_socket(init_socket(ip_port)),
        m_in(file_wait_t::in(m_socket.get())),
        m_out(file_wait_t::out(m_socket.get())),
        m_read_buffer_offset(0) {
    m_read_buffer.reserve(s_read_buffer_size);

    // Wait for the socket to become writable and check the result
    wait_all_interruptible(interruptor, m_out);

    int err;
    socklen_t err_size = sizeof(err);
    GUARANTEE_ERR(::getsockopt(m_socket.get(), SOL_SOCKET, SO_ERROR, &err, &err_size) == 0);
    assert(err == 0); // TODO: exception
}

void tcp_conn_t::read(void *buffer, size_t count, waitable_t *interruptor) {
    char *buf = reinterpret_cast<char *>(buffer);
    auto lock = m_read_mutex.lock();

    while (count > 0) {
        size_t bytes_to_copy =
            std::min<size_t>(count, m_read_buffer.size() - m_read_buffer_offset);
        count -= bytes_to_copy;
        while (bytes_to_copy > 0) {
            --bytes_to_copy;
            *buf++ = m_read_buffer[m_read_buffer_offset++];
        }

        if (count > 0) {
            if (count > s_read_buffer_size / 2) {
                ssize_t res = eintr_wrap([&] {
                        return ::read(m_socket.get(), buf, count);
                    }, &m_in, interruptor);
                count -= res;
                buf += res;
            } else {
                read_into_buffer(interruptor);
            }
        }
    }
}

size_t tcp_conn_t::read_until(char delim, void *buffer, size_t count, waitable_t *interruptor) {
    char *buf = reinterpret_cast<char *>(buffer);
    auto lock = m_read_mutex.lock();
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

void tcp_conn_t::write(void *buffer, size_t count, waitable_t *interruptor) {
    char *buf = reinterpret_cast<char *>(buffer);
    auto lock = m_write_mutex.lock();

    while (count > 0) {
        ssize_t res = eintr_wrap([&] {
                return ::write(m_socket.get(), buf, count);
            }, &m_out, interruptor);
        count -= res;
        buf += res;
    }
}

scoped_fd_t tcp_conn_t::init_socket(const ip_and_port_t &ip_port) {
    scoped_fd_t sock(::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
    assert(sock.valid());

    sockaddr_in6 sa;
    memset(&sa, 0, sizeof(sa));
    ip_port.to_sockaddr(&sa);
    GUARANTEE_ERR(::connect(sock.get(), (sockaddr *)&sa, sizeof(sa)) == 0 || errno == EINPROGRESS);
    return sock;
}

void tcp_conn_t::read_into_buffer(waitable_t *interruptor) {
    assert(m_read_buffer_offset == m_read_buffer.size());
    ssize_t res = 0;
    m_read_buffer_offset = 0;
    m_read_buffer.resize(s_read_buffer_size);
                         
    while (res == 0) {
        res = eintr_wrap([&] {
                return ::read(m_socket.get(), &m_read_buffer[0], s_read_buffer_size);
            }, &m_in, interruptor);
    }
    m_read_buffer.resize(res);
}

static void accept_loop(std::function<void (tcp_conn_t, drainer_lock_t)> on_connect,
                        fd_t sock, drainer_lock_t drain) {
    try {
        file_wait_t in = file_wait_t::in(sock);
        while (true) {
            sockaddr_in6 sa;
            socklen_t sa_len = sizeof(sa);
            in.wait();       
            scoped_fd_t new_sock = ::accept(sock, (sockaddr *)&sa, &sa_len);
            GUARANTEE_ERR(new_sock.valid());
            on_connect(tcp_conn_t(std::move(new_sock)), drain);
        }
    } catch (const wait_interrupted_exc_t &ex) {
        assert(drain.draining());
    }
}

tcp_listener_t::tcp_listener_t(tcp_listener_t &&other) :
        m_local_port(other.m_local_port),
        m_socket(std::move(other.m_socket)),
        m_drainer(std::move(other.m_drainer)) { }

tcp_listener_t::tcp_listener_t(uint16_t _local_port,
                               std::function<void (tcp_conn_t, drainer_lock_t)> on_connect) :
        m_local_port(_local_port),
        m_socket(init_socket()) {
    coro_t::spawn(&accept_loop,
                  std::move(on_connect), m_socket.get(), m_drainer.lock());
}

uint16_t tcp_listener_t::local_port() {
    if (m_local_port == 0) {
        sockaddr_in6 sa;
        memset(&sa, 0, sizeof(sa));
        socklen_t sa_len = sizeof(sa);
        GUARANTEE_ERR(::getsockname(m_socket.get(), (sockaddr *)&sa, &sa_len) == 0);
        assert(sa.sin6_family == AF_INET6);
        assert(sa.sin6_port != 0);
        m_local_port = sa.sin6_port;
    }
    return m_local_port;
}

scoped_fd_t tcp_listener_t::init_socket() {
    scoped_fd_t sock = ::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    GUARANTEE_ERR(sock.valid());

    sockaddr_in6 sa;
    memset(&sa, 0, sizeof(sa));
    ip_and_port_t::any(m_local_port).to_sockaddr(&sa);
    GUARANTEE_ERR(::bind(sock.get(), (sockaddr *)&sa, sizeof(sa)) == 0);
    GUARANTEE_ERR(::listen(sock.get(), 10) == 0);

    return sock;
}

} // namespace indecorous
