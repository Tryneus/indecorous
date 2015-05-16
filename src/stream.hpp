#ifndef STREAM_HPP_
#define STREAM_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

class write_message_t;

class stream_t {
public:
    virtual ~stream_t();
    virtual void read(char *buffer, size_t length) = 0;
    virtual void write(char *buffer, size_t length) = 0;
};

class dummy_stream_t : public stream_t {
public:
    dummy_stream_t();
    void read(char *buffer, size_t length);
    void write(char *buffer, size_t length);
private:
    std::vector<char> data;
};

class tcp_stream_t : public stream_t {
public:
    tcp_stream_t(int _fd);
    void read(char *buffer, size_t length);
    void write(char *buffer, size_t length);
private:
    int fd;
};

#endif // STREAM_HPP_
