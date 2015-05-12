#include "include/serialize_stl.hpp"

#include "include/debug.hpp"
#include "include/message.hpp"

template <> size_t serialized_size(const std::string &item) {
    return serialized_size(size_t()) + item.size();
}
template <> int serialize(write_message_t *msg, std::string &&item) {
    // TODO: efficient moving of buffers (linked list of variants?)
    serialize(msg, size_t(item.size()));
    for (size_t i = 0; i < item.size(); ++i) {
        msg->buffer.push_back(item[i]);
    }
    return 0;
}
template <> std::string deserialize(read_message_t *msg) {
    std::string res;
    printf("Deserializing string, offset: %zu\n", msg->offset);
    size_t length = deserialize<size_t>(msg);
    printf("Got length %zu, offset: %zu\n", length, msg->offset);
    for (size_t i = 0; i < length; ++i) {
        res.push_back(msg->pop());
        printf("Got character %c, offset: %zu\n", res[res.size() - 1], msg->offset);
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
