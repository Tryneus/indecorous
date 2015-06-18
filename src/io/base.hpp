#ifndef IO_BASE_HPP_
#define IO_BASE_HPP_

#include <cstddef>

namespace indecorous {

class wait_object_t;

class read_stream_t {
public:
    virtual ~read_stream_t() { }
    virtual void read(char *buf, size_t count, wait_object_t *interruptor) = 0;
    virtual size_t read_until(char delim, char *buf, size_t count, wait_object_t *interruptor) = 0;
};

class write_stream_t {
public:
    virtual ~write_stream_t() { }
    virtual void write(char *buf, size_t count, wait_object_t *interruptor) = 0;
};

} // namespace indecorous

#endif // IO_BASE_HPP_
