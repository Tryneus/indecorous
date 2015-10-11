#ifndef SYNC_CORO_POOL_HPP_
#define SYNC_CORO_POOL_HPP_

#include <cstddef>
#include <type_traits>

#include "common.hpp"
#include "coro/coro.hpp"
#include "sync/drainer.hpp"
#include "sync/interruptor.hpp"
#include "sync/semaphore.hpp"

namespace indecorous {

class coro_pool_t {
public:
    explicit coro_pool_t(size_t pool_size);
    ~coro_pool_t();

    void resize(size_t new_pool_size);

    // The producer should return a value that can be passed into the consumer.  The
    // producer may throw a wait_interrupted_exc_t to indicate the end of the loop.  The
    // pool provides a waitable_t to each function that will block destruction until
    // completion, as well as allow the functions to check if they should be interrupted.
    // The consumer should not throw a wait_interrupted_exc_t.
    //
    // producer signature: T()
    // consumer signature: void(T)
    template <typename callable_producer_t,
              typename callable_consumer_t,
              typename val_t = typename std::result_of<callable_producer_t()>::type >
    typename std::enable_if<!std::is_void<val_t>::value, void>::type
    run(callable_producer_t &&producer, callable_consumer_t &&consumer) {
        drainer_lock_t keepalive = m_drainer.lock();
        interruptor_t pool_interruptor(&keepalive);

        drainer_t local_drainer;

        try {
            while (true) {
                val_t val = producer();
                semaphore_acq_t acq = m_semaphore.start_acq(1);
                acq.wait();
                coro_t::spawn(
                    [&] (val_t inner_val, semaphore_acq_t, drainer_lock_t) {
                        consumer(std::move(inner_val));
                    }, std::move(val), std::move(acq), local_drainer.lock());
            }
        } catch (const wait_interrupted_exc_t &) {
            // Do nothing, either the drainer is draining or the producer is finished
        }
    }

    // Alternate version for a producer that returns void
    // producer signature: void()
    // consumer signature: void()
    template <typename callable_producer_t,
              typename callable_consumer_t,
              typename val_t = typename std::result_of<callable_producer_t()>::type >
    typename std::enable_if<std::is_void<val_t>::value, void>::type
    run(callable_producer_t &&producer, callable_consumer_t &&consumer) {
        drainer_lock_t keepalive = m_drainer.lock();
        interruptor_t pool_interruptor(&keepalive);

        drainer_t local_drainer;

        try {
            while (true) {
                producer();
                semaphore_acq_t acq = m_semaphore.start_acq(1);
                acq.wait();
                coro_t::spawn(
                    [&] (semaphore_acq_t, drainer_lock_t) {
                        consumer();
                    }, std::move(acq), local_drainer.lock());
            }
        } catch (const wait_interrupted_exc_t &) {
            // Do nothing, either the drainer is draining or the producer is finished
        }
    }

private:
    semaphore_t m_semaphore;
    drainer_t m_drainer;

    DISABLE_COPYING(coro_pool_t);
};

} // namespace indecorous

#endif // SYNC_CORO_POOL_HPP_
