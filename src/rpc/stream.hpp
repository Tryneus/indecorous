#ifndef RPC_STREAM_HPP_
#define RPC_STREAM_HPP_

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>

namespace indecorous {

class thread_t;
class write_message_t;
class read_message_t;

class stream_t {
public:
    virtual ~stream_t();
    virtual void write(write_message_t &&) = 0;
    virtual read_message_t read() = 0;
};

class local_stream_t : public stream_t {
public:
    local_stream_t(thread_t *_thread);
    void write(write_message_t &&msg);
    read_message_t read();
private:
    thread_t *thread;
    std::queue<write_message_t> message_queue;
    std::mutex mutex;
};

class tcp_stream_t : public stream_t {
public:
    tcp_stream_t(int _fd);
    void write(write_message_t &&msg);
    read_message_t read();
private:
    int fd;
};

} // namespace indecorous

#endif // STREAM_HPP_
