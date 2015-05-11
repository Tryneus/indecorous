#ifndef SERIALIZE_STL_HPP_
#define SERIALIZE_STL_HPP_

#include "include/serialize.hpp"

#include <cstddef>
#include <string>

template <> size_t serialized_size(const std::string &item);
template <> int serialize(write_message_t *msg, std::string &&item);
template <> std::string deserialize(read_message_t *msg);

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

#endif // SERIALIZE_STL_HPP_
