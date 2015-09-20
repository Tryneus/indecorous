#ifndef SYNC_CORO_MAP_HPP_
#define SYNC_CORO_MAP_HPP_

#include <cstdint>
#include <vector>
#include <type_traits>

#include "coro/coro.hpp"
#include "sync/promise.hpp"

namespace indecorous {

template <typename callable_t>
void coro_map(size_t start, size_t end, callable_t &&cb) {
    assert(start <= end);
    std::vector<future_t<void> > futures;
    futures.reserve(end - start);
    for (size_t i = 0; i < end; ++i) {
        // TODO: high priority: coroutine spawning is passing 'i' down as a reference.
        // It needs to store a copy because the callable we're passing to spawn takes
        // a copy.
        futures.emplace_back(coro_t::spawn(
            [&] (size_t inner_i) {
                cb(inner_i);
            }, i));
    }
    wait_all_it(futures);
}

template <typename callable_t, typename iterable_t>
typename std::enable_if<!std::is_integral<iterable_t>::value, void>::type
coro_map(const iterable_t &iterable, callable_t &&cb) {
    std::vector<future_t<void> > futures;
    futures.reserve(iterable.size());
    for (auto const &item : iterable) {
        futures.emplace_back(coro_t::spawn(
            [&] (decltype(item) inner_item) {
                cb(inner_item);
            }, item));
    }
    wait_all_it(futures);
}

template <typename callable_t, typename count_t>
typename std::enable_if<std::is_integral<count_t>::value, void>::type
coro_map(count_t count, callable_t &&cb) {
    coro_map(0, static_cast<size_t>(count), std::forward<callable_t>(cb));
}

template <typename callable_t>
void throttled_coro_map(size_t start, size_t end, callable_t &&cb, size_t limit) {
    // TODO
}

template <typename callable_t, typename iterable_t>
typename std::enable_if<!std::is_integral<iterable_t>::value, void>::type
throttled_coro_map(const iterable_t &iterable, callable_t &&cb, size_t limit) {
    // TODO
}

template <typename callable_t, typename count_t>
typename std::enable_if<std::is_integral<count_t>::value, void>::type
throttled_coro_map(count_t count, callable_t &&cb, size_t limit) {
    throttled_coro_map(0, static_cast<size_t>(count),
                       std::forward<callable_t>(cb), limit);
}


} // namespace indecorous

#endif // SYNC_CORO_MAP_HPP_
