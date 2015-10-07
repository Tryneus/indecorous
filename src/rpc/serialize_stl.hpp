#ifndef RPC_SERIALIZE_STL_HPP_
#define RPC_SERIALIZE_STL_HPP_

#include <array>
#include <cassert>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "rpc/serialize.hpp"

namespace indecorous {

template <typename Container>
int serialize_container(write_message_t *msg, const Container &c) {
    typedef typename Container::value_type Value;
    for (auto const &item : c) {
        serializer_t<Value>::write(msg, item);
    }
    return 0;
}

// Generic hack to obtain the internal container of an STL type which doesn't have
// a interface conducive to serialization/deserialization.
template <class T>
const typename T::container_type &get_container(const T &item) {
    struct hack_t : private T {
        static const typename T::container_type &do_hack(const T &x) {
            return x.*&hack_t::c;
        }
    };
    return hack_t::do_hack(item);
}

// std::string
template <> struct serializer_t<std::string> {
    static size_t size(const std::string &item);
    static int write(write_message_t *msg, const std::string &item);
    static std::string read(read_message_t *msg);
};

// std::vector
template <typename T> struct serializer_t<std::vector<T> > {
    static size_t size(const std::vector<T> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<T>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::vector<T> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::vector<T> read(read_message_t *msg) {
        std::vector<T> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_back(serializer_t<T>::read(msg));
        }
        return res;
    }
};

// std::pair
template <typename A, typename B> struct serializer_t<std::pair<A, B> > {
    static size_t size(const std::pair<A, B> &item) {
        return serializer_t<A>::size(item.first) + serializer_t<B>::size(item.second);
    }
    static int write(write_message_t *msg, const std::pair<A, B> &item) {
        return full_serialize(msg, item.first, item.second);
    }
    static std::pair<A, B> read(read_message_t *msg) {
        return full_deserialize<std::pair<A, B>, A, B>(msg);
    }
};


// std::map
template <typename K, typename V, typename C>
struct serializer_t<std::map<K, V, C> > {
    typedef typename std::map<K, V, C>::value_type Value;
    static size_t size(const std::map<K, V, C> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<Value>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::map<K, V, C> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::map<K, V, C> read(read_message_t *msg) {
        std::map<K, V, C> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_hint(res.end(), serializer_t<std::pair<K, V> >::read(msg));
        }
        return res;
    }
};

// std::multimap
template <typename K, typename V, typename C>
struct serializer_t<std::multimap<K, V, C> > {
    typedef typename std::multimap<K, V, C>::value_type Value;
    static size_t size(const std::multimap<K, V, C> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<Value>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::multimap<K, V, C> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::multimap<K, V, C> read(read_message_t *msg) {
        std::multimap<K, V, C> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_hint(res.end(), serializer_t<std::pair<K, V> >::read(msg));
        }
        return res;
    }
};

// std::unordered_map
template <typename K, typename V, typename H, typename E>
struct serializer_t<std::unordered_map<K, V, H, E> > {
    typedef typename std::unordered_map<K, V, H, E>::value_type Value;
    static size_t size(const std::unordered_map<K, V, H, E> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<Value>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::unordered_map<K, V, H, E> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::unordered_map<K, V, H, E> read(read_message_t *msg) {
        std::unordered_map<K, V, H, E> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace(serializer_t<std::pair<K, V> >::read(msg));
        }
        return res;
    }
};

// std::unordered_multimap
template <typename K, typename V, typename H, typename E>
struct serializer_t<std::unordered_multimap<K, V, H, E> > {
    typedef typename std::unordered_multimap<K, V, H, E>::value_type Value;
    static size_t size(const std::unordered_multimap<K, V, H, E> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<Value>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::unordered_multimap<K, V, H, E> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::unordered_multimap<K, V, H, E> read(read_message_t *msg) {
        std::unordered_multimap<K, V, H, E> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace(serializer_t<std::pair<K, V> >::read(msg));
        }
        return res;
    }
};

// std::set
template <typename T, typename C> struct serializer_t<std::set<T, C> > {
    static size_t size(const std::set<T, C> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<T>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::set<T, C> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::set<T, C> read(read_message_t *msg) {
        std::set<T, C> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_hint(res.end(), serializer_t<T>::read(msg));
        }
        return res;
    }
};

// std::multiset
template <typename T, typename C> struct serializer_t<std::multiset<T, C> > {
    static size_t size(const std::multiset<T, C> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<T>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::multiset<T, C> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::multiset<T, C> read(read_message_t *msg) {
        std::multiset<T, C> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_hint(res.end(), serializer_t<T>::read(msg));
        }
        return res;
    }
};

// std::unordered_set
template <typename T, typename H, typename E>
struct serializer_t<std::unordered_set<T, H, E> > {
    static size_t size(const std::unordered_set<T, H, E> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<T>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::unordered_set<T, H, E> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::unordered_set<T, H, E> read(read_message_t *msg) {
        std::unordered_set<T, H, E> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace(serializer_t<T>::read(msg));
        }
        return res;
    }
};

// std::unordered_multiset
template <typename T, typename H, typename E>
struct serializer_t<std::unordered_multiset<T, H, E> > {
    static size_t size(const std::unordered_multiset<T, H, E> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<T>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::unordered_multiset<T, H, E> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::unordered_multiset<T, H, E> read(read_message_t *msg) {
        std::unordered_multiset<T, H, E> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace(serializer_t<T>::read(msg));
        }
        return res;
    }
};

// std::list
template <typename T> struct serializer_t<std::list<T> > {
    static size_t size(const std::list<T> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<T>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::list<T> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::list<T> read(read_message_t *msg) {
        std::list<T> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_back(serializer_t<T>::read(msg));
        }
        return res;
    }
};

// std::stack
template <typename T> struct serializer_t<std::stack<T> > {
    typedef typename std::stack<T>::container_type Container;
    static size_t size(const std::stack<T> &item) {
        const Container &container = get_container(item);
        return serializer_t<Container>::size(container);
    }
    static int write(write_message_t *msg, const std::stack<T> &item) {
        const Container &container = get_container(item);
        return serializer_t<Container>::write(msg, container);
    }
    static std::stack<T> read(read_message_t *msg) {
        return std::stack<T>(serializer_t<Container>::read(msg));
    }
};

// std::queue
template <typename T> struct serializer_t<std::queue<T> > {
    typedef typename std::queue<T>::container_type Container;
    static size_t size(const std::queue<T> &item) {
        const Container &container = get_container(item);
        return serializer_t<Container>::size(container);
    }
    static int write(write_message_t *msg, const std::queue<T> &item) {
        const Container &container = get_container(item);
        return serializer_t<Container>::write(msg, container);
    }
    static std::queue<T> read(read_message_t *msg) {
        return std::queue<T>(serializer_t<Container>::read(msg));
    }
};

// std::deque
template <typename T> struct serializer_t<std::deque<T> > {
    static size_t size(const std::deque<T> &item) {
        size_t res = serializer_t<uint64_t>::size(item.size());
        for (auto const &i : item) {
            res += serializer_t<T>::size(i);
        }
        return res;
    }
    static int write(write_message_t *msg, const std::deque<T> &item) {
        serializer_t<uint64_t>::write(msg, item.size());
        return serialize_container(msg, item);
    }
    static std::deque<T> read(read_message_t *msg) {
        std::deque<T> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_back(serializer_t<T>::read(msg));
        }
        return res;
    }
};

// std::forward_list
template <typename T> struct serializer_t<std::forward_list<T> > {
    static size_t size(const std::forward_list<T> &item) {
        size_t res = 0;
        size_t size = 0;
        for (auto const &i : item) {
            res += serializer_t<T>::size(i);
            size += 1;
        }
        res += serializer_t<uint64_t>::size(size);
        return res;
    }
    static int write(write_message_t *msg, const std::forward_list<T> &item) {
        size_t size = 0;
        // TODO: size isn't known - have to traverse twice?
        // Other option - reserve space in the stream and rewrite it later?
        for (UNUSED auto const &i : item) { size += 1; }
        serializer_t<uint64_t>::write(msg, std::move(size));
        return serialize_container(msg, item);
    }
    static std::forward_list<T> read(read_message_t *msg) {
        std::forward_list<T> res;
        size_t size = serializer_t<uint64_t>::read(msg);
        // TODO: see if this can be simplified
        if (size > 0) {
            res.emplace_front(serializer_t<T>::read(msg));
        }
        typename std::forward_list<T>::iterator it = res.begin();
        for (size_t i = 1; i < size; ++i) {
            it = res.emplace_after(it, serializer_t<T>::read(msg));
        }
        return res;
    }
};

// std::priority_queue
template <typename T, typename U, typename C>
struct serializer_t<std::priority_queue<T, U, C> > {
    static size_t size(const std::priority_queue<T, U, C> &item) {
        const U &container = get_container(item);
        return serializer_t<U>::size(container);
    }
    static int write(write_message_t *msg, const std::priority_queue<T, U, C> &item) {
        const U &container = get_container(item);
        return serializer_t<U>::write(msg, container);
    }
    static std::priority_queue<T, U, C> read(read_message_t *msg) {
        return std::priority_queue<T, U, C>(C(), serializer_t<U>::read(msg));
    }
};

// std::array
template <typename T, size_t N> struct serializer_t<std::array<T, N> > {
    template <size_t... X>
    static size_t size_internal(std::integer_sequence<size_t, X...>,
                                const std::array<T, N> &item) {
        struct accumulator_t {
            accumulator_t() : total(0) { }
            int add(size_t value) { total += value; return 0; }
            size_t total;
        } acc;
        UNUSED int dummy[] = { acc.add(serializer_t<T>::size(std::get<X>(item)))... };
        return acc.total;
    }
    static size_t size(const std::array<T, N> &item) {
        return size_internal(std::make_index_sequence<N>(), item);
    }
    template <size_t... X>
    static int write_internal(std::integer_sequence<size_t, X...>,
                              write_message_t *msg, const std::array<T, N> &item) {
        UNUSED int dummy[] = { serializer_t<T>::write(msg, std::get<X>(item))... };
        return 0;
    }
    static int write(write_message_t *msg, const std::array<T, N> &item) {
        return write_internal(std::make_index_sequence<N>(), msg, item);
    }
    template <size_t... X>
    static std::array<T, N> read_internal(std::integer_sequence<size_t, X...>,
                                         read_message_t *msg) {
        // The comma operator here is an ugly hack to get the parameter pack running
        return std::array<T, N>({ ((void)X, serializer_t<T>::read(msg))... });
    }
    static std::array<T, N> read(read_message_t *msg) {
        return read_internal(std::make_index_sequence<N>(), msg);
    }
};

} // namespace indecorous

#endif // SERIALIZE_STL_HPP_
