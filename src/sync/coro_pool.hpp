#ifndef SYNC_CORO_POOL_HPP_
#define SYNC_CORO_POOL_HPP_

#include "coro/coro.hpp"
#include "sync/drainer.hpp"
#include "sync/semaphore.hpp"

namespace indecorous {

class coro_pool_t {
public:
    coro_pool_t(size_t pool_size) : m_semaphore(pool_size) { }

    // The producer should return a value that can be passed into the consumer.  The
    // producer may throw a wait_interrupted_exc_t to indicate the end of the loop.  The
    // pool provides a waitable_t to each function that will block destruction until
    // completion, as well as allow the functions to check if they should be interrupted.
    // The consumer should not throw a wait_interrupted_exc_t.
    //
    // producer signature: T(waitable_t *)
    // consumer signature: void(T, waitable_t *)
    template <typename callable_producer_t,
              typename callable_consumer_t>
    void run(callable_producer_t &&producer,
             callable_consumer_t &&consumer) {
        drainer_lock_t keepalive = m_drainer.lock();
        drainer_t local_drainer;

        try {
            while (true) {
                auto val = producer(&m_drainer);
                semaphore_acq_t acq = m_semaphore.start_acq(1);
                wait_any_interruptible(&keepalive, acq);
                assert(acq.owned() == 1);
                coro_t::spawn(
                    [&] (decltype(val) inner_val,
                         semaphore_acq_t, // acq
                         drainer_lock_t, // local_keepalive
                         drainer_lock_t inner_keepalive) {
                        consumer(std::move(inner_val), &inner_keepalive);
                    }, std::move(val), std::move(acq), local_drainer.lock(), keepalive);
            }
        } catch (const wait_interrupted_exc_t &) {
            // Do nothing, either the drainer is draining or the producer is finished
        }
    }

    ~coro_pool_t() { }

private:
    semaphore_t m_semaphore;
    drainer_t m_drainer;
};

} // namespace indecorous

#endif // SYNC_CORO_POOL_HPP_
