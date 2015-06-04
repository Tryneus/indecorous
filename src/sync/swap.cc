#include "sync/swap.hpp"

assert_no_swap_t::assert_no_swap_t() {
    thread_t::self()->dispatcher()->forbid_swap();
}

assert_no_swap_t::~assert_no_swap_t() {
    thread_t::self()->dispatcher()->allow_swap();
}
