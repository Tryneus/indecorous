#include "sync/swap.hpp"

#include "coro/thread.hpp"

namespace indecorous {

assert_no_swap_t::assert_no_swap_t() :
        m_dispatch(thread_t::self()->dispatcher()),
        m_old_swap_permitted(m_dispatch->m_swap_permitted) {
    m_dispatch->m_swap_permitted = false;
}

assert_no_swap_t::~assert_no_swap_t() {
    assert(!m_dispatch->m_swap_permitted);
    m_dispatch->m_swap_permitted = m_old_swap_permitted;
}

} // namespace indecorous
