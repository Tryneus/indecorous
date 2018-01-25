#ifndef IO_BASE_HPP_
#define IO_BASE_HPP_

#include <cstddef>

namespace indecorous {

class waitable_t;

class read_stream_t {
public:
    virtual ~read_stream_t() { }
    virtual void peek(void *buf, size_t count) = 0;
    virtual void read(void *buf, size_t count) = 0;
    virtual size_t read_until(char delim, void *buf, size_t count) = 0;
};

class write_stream_t {
public:
    virtual ~write_stream_t() { }
    virtual void write(void *buf, size_t count) = 0;
};

} // namespace indecorous

#endif // IO_BASE_HPP_
