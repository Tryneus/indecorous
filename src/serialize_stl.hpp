#ifndef SERIALIZE_STL_HPP_
#define SERIALIZE_STL_HPP_

#include "serialize.hpp"

#include <cstddef>
#include <string>
#include <vector>

// std::string
template <> struct sizer_t<std::string> {
    static size_t run(const std::string &item);
};
template <> struct serializer_t<std::string> {
    static int run(write_message_t *msg, std::string &&item);
};
template <> struct deserializer_t<std::string> {
    static std::string run(read_message_t *msg);
};

// std::vector
template <typename T> struct sizer_t<std::vector<T> > {
    static size_t run(const std::vector<T> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<T>::run(i);
        }
        return res;
    }
};
template <typename T> struct serializer_t<std::vector<T> > {
    static int run(write_message_t *msg, std::vector<T> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            serializer_t<T>::run(msg, std::move(i));
        }
        return 0;
    }
};
template <typename T> struct deserializer_t<std::vector<T> > {   
    static std::vector<T> run(read_message_t *msg) {
        std::vector<T> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_back(deserializer_t<T>::run(msg));
        }
        return res;
    }
};

// std::map
//template <typename K, typename V, typename C>
//    std::map<K,V,C> deserialize(read_message_t *msg);

// std::set
//template <typename T, typename C>
//    std::set<T,C> deserialize(read_message_t *msg);

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
    std::forward_list<T> deserialize(read_message_t *msg);
template <> template <typename T>
    std::priority_queue<T> deserialize(read_message_t *msg);
template <> template <typename T, size_t N>
    std::array<T,N> deserialize(read_message_t *msg);
template <> template <typename T, typename U>
    std::pair<T,U> deserialize(read_message_t *msg);
template <> template <typename T, typename C>
    std::multiset<T,C> deserialize(read_message_t *msg);
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
