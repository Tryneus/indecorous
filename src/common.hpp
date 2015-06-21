#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <errno.h>

#include <cassert>
#include <cstdint>
#include <inttypes.h>

#include "sync/multiple_wait.hpp"

// TODO: make sure these aren't exposed to users
#ifdef NDEBUG
    #define DEBUG_ONLY(...)
    #define DEBUG_VAR __attribute__((unused))
    #define GUARANTEE(x) do { if (!(x)) abort(); } while (0)
#else
    #define DEBUG_ONLY(...) __VA_ARGS__
    #define DEBUG_VAR
    #define GUARANTEE(x) do { assert(x == y); } while (0)
#endif

#define debugf(format, ...) printf("Thread %" PRIi32 " - " format "\n", indecorous::thread_self_id(), ##__VA_ARGS__)

namespace indecorous {

int32_t thread_self_id();

template <typename Callable>
auto eintr_wrap(Callable &&c) {
    while (true) {
        auto res = c();
        if (res != -1) { return res; }
        assert(errno == EINTR);
    }
}

// Same as above, except it handles EAGAIN by waiting on the specified wait object
template <typename Callable>
auto eintr_wrap(Callable &&c, wait_object_t *w, wait_object_t *interruptor) {
    while (true) {
        auto res = c();
        if (res != -1) { return res; }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            wait_all_interruptible(interruptor, w);
        }
        assert(errno == EINTR);
    }
}

} // namespace indecorous

#endif // COMMON_HPP_
