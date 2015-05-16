#include "serialize_stl.hpp"

#include "debug.hpp"
#include "message.hpp"

size_t serializer_t<std::string>::size(const std::string &item) {
    return serializer_t<uint64_t>::size(item.size()) + item.size();
}
int serializer_t<std::string>::write(write_message_t *msg, const std::string &item) {
    serializer_t<uint64_t>::write(msg, item.size());
    for (size_t i = 0; i < item.size(); ++i) {
        msg->push_back(item[i]);
    }
    return 0;
}
std::string serializer_t<std::string>::read(read_message_t *msg) {
    std::string res;
    size_t length = serializer_t<uint64_t>::read(msg);
    for (size_t i = 0; i < length; ++i) {
        res.push_back(msg->pop());
    }
    return res;
}

