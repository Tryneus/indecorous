#include "sync/swap.hpp"

assert_no_swap_t::assert_no_swap_t() {
    // TODO: save previous swap state
    thread_t::self()->dispatcher()->forbid_swap();
}

assert_no_swap_t::~assert_no_swap_t() {
    // TODO: restore previous swap state (so we can nest these objects)
    thread_t::self()->dispatcher()->allow_swap();
}
