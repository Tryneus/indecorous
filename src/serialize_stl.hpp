#ifndef SERIALIZE_STL_HPP_
#define SERIALIZE_STL_HPP_

#include "serialize.hpp"

#include <array>
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

template <typename T>
int serialize_move_iterator(write_message_t *msg,
                            std::move_iterator<T> &&begin,
                            std::move_iterator<T> &&end) {

}

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
        item.clear();
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

// std::pair
template <typename A, typename B> struct sizer_t<std::pair<A,B> > {
    static size_t run(const std::pair<A,B> &item) {
        return sizer_t<A>::run(item.first) + sizer_t<B>::run(item.second);
    }
};
template <typename A, typename B> struct serializer_t<std::pair<A,B> > {
    static int run(write_message_t *msg, std::pair<A,B> &&item) {
        return full_serialize(msg, std::move(item.first), std::move(item.second));
    }
};
template <typename A, typename B> struct deserializer_t<std::pair<A,B> > {
    static std::pair<A,B> run(read_message_t *msg) {
        return full_deserialize<std::pair<A,B>,A,B>(msg);
    }
};


// std::map
template <typename K, typename V, typename C>
struct sizer_t<std::map<K,V,C> > {
    static size_t run(const std::map<K,V,C> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<std::pair<K,V> >::run(i);
        }
        return res;
    }
};
template <typename K, typename V, typename C>
struct serializer_t<std::map<K,V,C> > {
    static int run(write_message_t *msg, std::map<K,V,C> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            // TODO: do this without copying
            serializer_t<std::pair<K,V> >::run(msg,
                                               std::pair<K,V>(std::move(i.first),
                                                              std::move(i.second)));
        }
        item.clear();
        return 0;
    }
};
template <typename K, typename V, typename C>
struct deserializer_t<std::map<K,V,C> > {
    static std::map<K,V,C> run(read_message_t *msg) {
        std::map<K,V,C> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_hint(res.end(), deserializer_t<std::pair<K,V> >::run(msg));
        }
        return res;
    }
};

// std::multimap
template <typename K, typename V, typename C>
struct sizer_t<std::multimap<K,V,C> > {
    static size_t run(const std::multimap<K,V,C> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<std::pair<K,V> >::run(i);
        }
        return res;
    }
};
template <typename K, typename V, typename C>
struct serializer_t<std::multimap<K,V,C> > {
    static int run(write_message_t *msg, std::multimap<K,V,C> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            // TODO: do this without copying
            serializer_t<std::pair<K,V> >::run(msg,
                                               std::pair<K,V>(std::move(i.first),
                                                              std::move(i.second)));
        }
        item.clear();
        return 0;
    }
};
template <typename K, typename V, typename C>
struct deserializer_t<std::multimap<K,V,C> > {
    static std::multimap<K,V,C> run(read_message_t *msg) {
        std::multimap<K,V,C> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_hint(res.end(), deserializer_t<std::pair<K,V> >::run(msg));
        }
        return res;
    }
};

// std::unordered_map
template <typename K, typename V, typename H, typename E>
struct sizer_t<std::unordered_map<K,V,H,E> > {
    static size_t run(const std::unordered_map<K,V,H,E> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<std::pair<K,V> >::run(i);
        }
        return res;
    }
};
template <typename K, typename V, typename H, typename E>
struct serializer_t<std::unordered_map<K,V,H,E> > {
    static int run(write_message_t *msg, std::unordered_map<K,V,H,E> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            // TODO: do this without copying
            serializer_t<std::pair<K,V> >::run(msg,
                                               std::pair<K,V>(std::move(i.first),
                                                              std::move(i.second)));
        }
        item.clear();
        return 0;
    }
};
template <typename K, typename V, typename H, typename E>
struct deserializer_t<std::unordered_map<K,V,H,E> > {
    static std::unordered_map<K,V,H,E> run(read_message_t *msg) {
        std::unordered_map<K,V,H,E> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace(deserializer_t<std::pair<K,V> >::run(msg));
        }
        return res;
    }
};

// std::unordered_multimap
template <typename K, typename V, typename H, typename E>
struct sizer_t<std::unordered_multimap<K,V,H,E> > {
    static size_t run(const std::unordered_multimap<K,V,H,E> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<std::pair<K,V> >::run(i);
        }
        return res;
    }
};
template <typename K, typename V, typename H, typename E>
struct serializer_t<std::unordered_multimap<K,V,H,E> > {
    static int run(write_message_t *msg, std::unordered_multimap<K,V,H,E> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            // TODO: do this without copying
            serializer_t<std::pair<K,V> >::run(msg,
                                               std::pair<K,V>(std::move(i.first),
                                                              std::move(i.second)));
        }
        item.clear();
        return 0;
    }
};
template <typename K, typename V, typename H, typename E>
struct deserializer_t<std::unordered_multimap<K,V,H,E> > {
    static std::unordered_multimap<K,V,H,E> run(read_message_t *msg) {
        std::unordered_multimap<K,V,H,E> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace(deserializer_t<std::pair<K,V> >::run(msg));
        }
        return res;
    }
};

// std::set
template <typename T, typename C> struct sizer_t<std::set<T,C> > {
    static size_t run(const std::set<T,C> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<T>::run(i);
        }
        return res;
    }
};
template <typename T, typename C> struct serializer_t<std::set<T,C> > {
    static int run(write_message_t *msg, std::set<T,C> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            // TODO: do this without copying
            serializer_t<T>::run(msg, T(std::move(i)));
        }
        item.clear();
        return 0;
    }
};
template <typename T, typename C> struct deserializer_t<std::set<T,C> > {
    static std::set<T,C> run(read_message_t *msg) {
        std::set<T,C> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_hint(res.end(), deserializer_t<T>::run(msg));
        }
        return res;
    }
};

// std::multiset
template <typename T, typename C> struct sizer_t<std::multiset<T,C> > {
    static size_t run(const std::multiset<T,C> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<T>::run(i);
        }
        return res;
    }
};
template <typename T, typename C> struct serializer_t<std::multiset<T,C> > {
    static int run(write_message_t *msg, std::multiset<T,C> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            // TODO: do this without copying
            serializer_t<T>::run(msg, T(std::move(i)));
        }
        item.clear();
        return 0;
    }
};
template <typename T, typename C> struct deserializer_t<std::multiset<T,C> > {
    static std::multiset<T,C> run(read_message_t *msg) {
        std::multiset<T,C> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_hint(res.end(), deserializer_t<T>::run(msg));
        }
        return res;
    }
};

// std::unordered_set
template <typename T, typename H, typename E>
struct sizer_t<std::unordered_set<T,H,E> > {
    static size_t run(const std::unordered_set<T,H,E> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<T>::run(i);
        }
        return res;
    }
};
template <typename T, typename H, typename E>
struct serializer_t<std::unordered_set<T,H,E> > {
    static int run(write_message_t *msg, std::unordered_set<T,H,E> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            // TODO: do this without copying
            serializer_t<T>::run(msg, T(std::move(i)));
        }
        item.clear();
        return 0;
    }
};
template <typename T, typename H, typename E>
struct deserializer_t<std::unordered_set<T,H,E> > {
    static std::unordered_set<T,H,E> run(read_message_t *msg) {
        std::unordered_set<T,H,E> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace(deserializer_t<T>::run(msg));
        }
        return res;
    }
};

// std::unordered_multiset
template <typename T, typename H, typename E>
struct sizer_t<std::unordered_multiset<T,H,E> > {
    static size_t run(const std::unordered_multiset<T,H,E> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<T>::run(i);
        }
        return res;
    }
};
template <typename T, typename H, typename E>
struct serializer_t<std::unordered_multiset<T,H,E> > {
    static int run(write_message_t *msg, std::unordered_multiset<T,H,E> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        for (auto &&i : item) {
            // TODO: do this without copying
            serializer_t<T>::run(msg, T(std::move(i)));
        }
        item.clear();
        return 0;
    }
};
template <typename T, typename H, typename E>
struct deserializer_t<std::unordered_multiset<T,H,E> > {
    static std::unordered_multiset<T,H,E> run(read_message_t *msg) {
        std::unordered_multiset<T,H,E> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            res.emplace(deserializer_t<T>::run(msg));
        }
        return res;
    }
};

// std::list
template <typename T> struct sizer_t<std::list<T> > {
    static size_t run(const std::list<T> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<T>::run(i);
        }
        return res;
    }
};
template <typename T> struct serializer_t<std::list<T> > {
    static int run(write_message_t *msg, std::list<T> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        while (!item.empty()) {
            serializer_t<T>::run(msg, std::move(item.front()));
            item.pop_front();
        }
        return 0;
    }
};
template <typename T> struct deserializer_t<std::list<T> > {
    static std::list<T> run(read_message_t *msg) {
        std::list<T> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_back(deserializer_t<T>::run(msg));
        }
        return res;
    }
};

// Generic hack to obtain the internal container of an STL type which doesn't have
// a interface conducive to serialization/deserialization.
template <class T>
typename T::container_type &get_container(T &item) {
    struct hack_t : private T {
        static typename T::container_type &do_hack(T &x) {
            return x.*&hack_t::c;
        }
    };
    return hack_t::do_hack(item);
}

template <class T>
const typename T::container_type &get_container(const T &item) {
    struct hack_t : private T {
        static const typename T::container_type &do_hack(T &x) {
            return x.*&hack_t::c;
        }
    };
    return hack_t::do_hack(item);
}

// std::stack
template <typename T> struct sizer_t<std::stack<T> > {
    static size_t run(const std::stack<T> &item) {
        auto container = get_container(item);
        return sizer_t<decltype(container)>::run(container);
    }
};
template <typename T> struct serializer_t<std::stack<T> > {
    static int run(write_message_t *msg, std::stack<T> &&item) {
        auto container = get_container(item);
        return serializer_t<decltype(container)>::run(msg, container);
    }
};
template <typename T> struct deserializer_t<std::stack<T> > {
    typedef typename std::stack<T>::container_type internal_t;
    static std::stack<T> run(read_message_t *msg) {
        return std::stack<T>(deserializer_t<internal_t>::run(msg));
    }
};

// std::queue
template <typename T> struct sizer_t<std::queue<T> > {
    static size_t run(const std::queue<T> &item) {
        auto container = get_container(item);
        return sizer_t<decltype(container)>::run(container);
    }
};
template <typename T> struct serializer_t<std::queue<T> > {
    static int run(write_message_t *msg, std::queue<T> &&item) {
        auto container = get_container(item);
        return serializer_t<decltype(container)>::run(msg, container);
    }
};
template <typename T> struct deserializer_t<std::queue<T> > {
    typedef typename std::queue<T>::container_type internal_t;
    static std::queue<T> run(read_message_t *msg) {
        return std::queue<T>(deserializer_t<internal_t>::run(msg));
    }
};

// std::deque
template <typename T> struct sizer_t<std::deque<T> > {
    static size_t run(const std::deque<T> &item) {
        size_t res = sizer_t<uint64_t>::run(item.size());
        for (auto const &i : item) {
            res += sizer_t<T>::run(i);
        }
        return res;
    }
};
template <typename T> struct serializer_t<std::deque<T> > {
    static int run(write_message_t *msg, std::deque<T> &&item) {
        serializer_t<uint64_t>::run(msg, item.size());
        while (!item.empty()) {
            serializer_t<T>::run(msg, std::move(item.front()));
            item.pop_front();
        }
        return 0;
    }
};
template <typename T> struct deserializer_t<std::deque<T> > {
    static std::deque<T> run(read_message_t *msg) {
        std::deque<T> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        for (size_t i = 0; i < size; ++i) {
            res.emplace_back(deserializer_t<T>::run(msg));
        }
        return res;
    }
};

// std::forward_list
template <typename T> struct sizer_t<std::forward_list<T> > {
    static size_t run(const std::forward_list<T> &item) {
        size_t res = 0;
        size_t size = 0;
        for (auto const &i : item) {
            res += sizer_t<T>::run(i);
            size += 1;
        }
        res += sizer_t<uint64_t>::run(size);
        return res;
    }
};
template <typename T> struct serializer_t<std::forward_list<T> > {
    static int run(write_message_t *msg, std::forward_list<T> &&item) {
        size_t size = 0;
        // TODO: size isn't known - have to traverse twice?
        // Other option - reserve space in the stream and rewrite it later?
        for (__attribute__((unused)) auto const &i : item) { size += 1; }
        serializer_t<uint64_t>::run(msg, std::move(size));
        while (!item.empty()) {
            // TODO: do this without copying
            serializer_t<T>::run(msg, T(item.front()));
            item.pop_front();
        }
        return 0;
    }
};
template <typename T> struct deserializer_t<std::forward_list<T> > {
    static std::forward_list<T> run(read_message_t *msg) {
        std::forward_list<T> res;
        size_t size = deserializer_t<uint64_t>::run(msg);
        // TODO: see if this can be simplified
        if (size > 0) {
            res.emplace_front(deserializer_t<T>::run(msg));
        }
        typename std::forward_list<T>::iterator it = res.begin();
        for (size_t i = 1; i < size; ++i) {
            it = res.emplace_after(it, deserializer_t<T>::run(msg));
        }
        return res;
    }
};

// std::priority_queue
template <typename T> struct sizer_t<std::priority_queue<T> > {
    static size_t run(const std::priority_queue<T> &item) {
        auto container = get_container(item);
        return sizer_t<decltype(container)>::run(container);
    }
};
template <typename T> struct serializer_t<std::priority_queue<T> > {
    static int run(write_message_t *msg, std::priority_queue<T> &&item) {
        auto container = get_container(item);
        return serializer_t<decltype(container)>::run(msg, container);
    }
};
template <typename T> struct deserializer_t<std::priority_queue<T> > {
    typedef typename std::priority_queue<T>::container_type internal_t;
    static std::priority_queue<T> run(read_message_t *msg) {
        return std::priority_queue<T>(deserializer_t<internal_t>::run(msg));
    }
};

// std::array - uses templates to push evaluation to compile-time
template <typename T, size_t N> struct sizer_t<std::array<T,N> > {
    template <size_t... X>
    static size_t run_internal(std::integer_sequence<size_t, X...>,
                               const std::array<T,N> &item) {
        struct accumulator_t {
            accumulator_t() : total(0) { }
            int add(size_t value) { total += value; return 0; }
            size_t total;
        } acc;
        __attribute__((unused)) int dummy[] =
            { acc.add(sizer_t<T>::run(std::get<X>(item)))... };
        return acc.total;
    }
    static size_t run(const std::array<T,N> &item) {
        return run_internal(std::make_index_sequence<N>(), item);
    }
};
template <typename T, size_t N> struct serializer_t<std::array<T,N> > {
    template <size_t... X>
    static int run_internal(std::integer_sequence<size_t, X...>,
                            write_message_t *msg, std::array<T,N> &&item) {
        __attribute__((unused)) int dummy[] =
            { serializer_t<T>::run(msg, std::get<X>(item))... };
        return 0;
    }
    static size_t run(write_message_t *msg, std::array<T,N> &&item) {
        return run_internal(std::make_index_sequence<N>(), msg, item);
    }
};
template <typename T, size_t N> struct deserializer_t<std::array<T,N> > {
    template <size_t... X>
    static std::array<T,N> run_internal(std::integer_sequence<size_t, X...>,
                                        read_message_t *msg) {
        // The comma operator here is an ugly hack to get the parameter pack running
        return std::array<T,N>({ (X, deserializer_t<T>::run(msg))... });
    }

    static std::array<T,N> run(read_message_t *msg) {
        return run_internal(std::make_index_sequence<N>(), msg);
    }
};

#endif // SERIALIZE_STL_HPP_
