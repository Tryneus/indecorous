#include "serialize_stl.hpp"

#include "debug.hpp"
#include "message.hpp"

size_t sizer_t<std::string>::run(const std::string &item) {
    return sizer_t<uint64_t>::run(item.size()) + item.size();
}
int serializer_t<std::string>::run(write_message_t *msg, std::string &&item) {
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

/*
template <>
    std::string deserialize(read_message_t *msg);
template <> template <typename T>
    std::list<T> deserialize(read_message_t *msg);
template <> template <typename T>
    std::stack<T> deserialize(read_message_t *msg);
template <> template <typename T>
    std::queue<T> deserialize(read_message_t *msg);
template <> template <typename T>
    std::deque<T> deserialize(read_message_t *msg);
template <> template <typename T>
    std::vector<T> deserialize(read_message_t *msg);
template <> template <typename T>
    std::forward_list<T> deserialize(read_message_t *msg);
template <> template <typename T>
    std::priority_queue<T> deserialize(read_message_t *msg);
template <> template <typename T, size_t N>
    std::array<T,N> deserialize(read_message_t *msg);
template <> template <typename T, typename C>
    std::set<T,C> deserialize(read_message_t *msg);
template <> template <typename T, typename U>
    std::pair<T,U> deserialize(read_message_t *msg);
template <> template <typename T, typename C>
    std::multiset<T,C> deserialize(read_message_t *msg);
template <> template <typename K, typename V, typename C>
    std::map<K,V,C> deserialize(read_message_t *msg);
template <> template <typename K, typename V, typename C>
    std::multimap<K,V,C> deserialize(read_message_t *msg);
template <> template <typename T, typename H, typename E>
    std::unordered_set<T,H,E> deserialize(read_message_t *msg);
template <> template <typename T, typename H, typename E>
    std::unordered_multiset<T,H,E> deserialize(read_message_t *msg);
template <> template <typename K, typename V, typename H, typename E>
    std::unordered_map<K,V,H,E> deserialize(read_message_t *msg);
template <> template <typename K, typename V, typename H, typename E>
    std::unordered_multimap<K,V,H,E> deserialize(read_message_t *msg);
*/
