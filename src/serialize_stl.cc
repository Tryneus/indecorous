#include "serialize_stl.hpp"

#include "debug.hpp"
#include "message.hpp"

size_t sizer_t<std::string>::run(const std::string &item) {
    return sizer_t<uint64_t>::run(item.size()) + item.size();
}
int serializer_t<std::string>::run(write_message_t *msg, std::string item) {
    serializer_t<uint64_t>::run(msg, item.size());
    for (size_t i = 0; i < item.size(); ++i) {
        msg->buffer.push_back(item[i]);
    }
    return 0;
}
std::string deserializer_t<std::string>::run(read_message_t *msg) {
    std::string res;
    size_t length = deserializer_t<uint64_t>::run(msg);
    for (size_t i = 0; i < length; ++i) {
        res.push_back(msg->pop());
    }
    return res;
}

