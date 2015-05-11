#ifndef STREAM_HPP_
#define STREAM_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

class write_message_t;

class fake_stream_t {
public:
    void read(char *buffer, size_t length);
    void write(write_message_t &&msg);
private:
    std::vector<char> data;
};

#endif // STREAM_HPP_
