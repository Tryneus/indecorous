#include "sync/coro_pool.hpp"

namespace indecorous {

coro_pool_t::coro_pool_t(size_t pool_size) :
    m_semaphore(pool_size), m_drainer() { }

coro_pool_t::~coro_pool_t() { }

void coro_pool_t::resize(size_t new_pool_size) {
    m_semaphore.resize(new_pool_size);
}

} // namespace indecorous
