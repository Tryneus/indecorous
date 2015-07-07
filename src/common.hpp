#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <cstdio>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <cassert>
#include <cstdint>
#include <inttypes.h>

#include "sync/multiple_wait.hpp"

// TODO: make sure these aren't exposed to users
#ifdef NDEBUG
    #define DEBUG_ONLY(...)
    #define DEBUG_VAR __attribute__((unused))
    // #define ASSERT(x) do { (void)sizeof(x); } while (0)
#else
    #define DEBUG_ONLY(...) __VA_ARGS__
    #define DEBUG_VAR
    // #define ASSERT(x) do { assert(x); } while (0)
#endif

#define GUARANTEE(x) do { \
    if (!(x)) { \
        debugf("Guarantee failed [" #x "] " __FILE__ ":%d", __LINE__); \
        ::abort(); \
    } } while (0)
#define GUARANTEE_ERR(x) do { \
    if (!(x)) { \
        char errno_buffer[100]; \
        char *err_str = strerror_r(errno, errno_buffer, sizeof(errno_buffer)); \
        debugf("Guarantee failed [" #x "] errno = %d (%s) " __FILE__ ":%d", \
               errno, err_str, __LINE__); \
        ::abort(); \
    } } while (0)

#define debugf(format, ...) printf("Thread %" PRIi32 " - " format "\n", indecorous::thread_self_id(), ##__VA_ARGS__)

namespace indecorous {

int32_t thread_self_id();

template <typename Callable>
auto eintr_wrap(Callable &&c) {
    while (true) {
        auto res = c();
        if (res != -1) { return res; }
        GUARANTEE_ERR(errno == EINTR);
    }
}

// Same as above, except it handles EAGAIN by waiting on the specified wait object
template <typename Callable>
auto eintr_wrap(Callable &&c, waitable_t *w, waitable_t *interruptor) {
    while (true) {
        auto res = c();
        if (res != -1) { return res; }
        GUARANTEE_ERR(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK);
        wait_all_interruptible(interruptor, w);
    }
}

} // namespace indecorous

#endif // COMMON_HPP_
