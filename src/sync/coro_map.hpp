#ifndef SYNC_CORO_MAP_HPP_
#define SYNC_CORO_MAP_HPP_

#include <cstdint>
#include <type_traits>

#include "coro/coro.hpp"
#include "sync/drainer.hpp"
#include "sync/semaphore.hpp"

namespace indecorous {

template <typename callable_t>
void coro_map(size_t start, size_t end, callable_t &&cb) {
    assert(start <= end);
    drainer_t drainer;
    for (size_t i = start; i < end; ++i) {
        coro_t::spawn([&] (size_t inner_i, drainer_lock_t) {
                cb(inner_i);
            }, i, drainer.lock());
    }
}

template <typename callable_t, typename iterable_t>
typename std::enable_if<!std::is_integral<iterable_t>::value, void>::type
coro_map(const iterable_t &iterable, callable_t &&cb) {
    drainer_t drainer;
    for (auto const &item : iterable) {
        coro_t::spawn([&] (decltype(item) inner_item, drainer_lock_t) {
                cb(inner_item);
            }, item, drainer.lock());
    }
}

template <typename callable_t, typename count_t>
typename std::enable_if<std::is_integral<count_t>::value, void>::type
coro_map(count_t count, callable_t &&cb) {
    coro_map(0, static_cast<size_t>(count), std::forward<callable_t>(cb));
}

template <typename callable_t>
void throttled_coro_map(size_t start, size_t end, callable_t &&cb, size_t limit) {
    assert(start <= end);
    semaphore_t sem(limit);
    drainer_t drainer;
    for (size_t i = start; i < end; ++i) {
        semaphore_acq_t acq = sem.start_acq(1);
        acq.wait();
        coro_t::spawn([&] (size_t inner_i, drainer_lock_t, semaphore_acq_t) {
                cb(inner_i);
            }, i, drainer.lock(), std::move(acq));
    }
}

template <typename callable_t, typename iterable_t>
typename std::enable_if<!std::is_integral<iterable_t>::value, void>::type
throttled_coro_map(const iterable_t &iterable, callable_t &&cb, size_t limit) {
    semaphore_t sem(limit);
    drainer_t drainer;
    for (auto const &item : iterable) {
        semaphore_acq_t acq = sem.start_acq(1);
        acq.wait();
        coro_t::spawn([&] (decltype(item) inner_item, drainer_lock_t, semaphore_acq_t) {
                cb(inner_item);
            }, item, drainer.lock(), std::move(acq));
    }
}

template <typename callable_t, typename count_t>
typename std::enable_if<std::is_integral<count_t>::value, void>::type
throttled_coro_map(count_t count, callable_t &&cb, size_t limit) {
    throttled_coro_map(0, static_cast<size_t>(count),
                       std::forward<callable_t>(cb), limit);
}


} // namespace indecorous

#endif // SYNC_CORO_MAP_HPP_
