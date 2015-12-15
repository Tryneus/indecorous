#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <cstdio>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <cassert>
#include <cstdint>
#include <inttypes.h>

// TODO: make sure macros aren't exposed to users
#define DISABLE_COPYING(T) \
    T(const T &) = delete; \
    T &operator = (const T &) = delete

#define UNUSED __attribute__((unused))

#ifdef NDEBUG
    #define DEBUG_ONLY(...)
    #define DEBUG_VAR UNUSED
    // #define ASSERT(x) do { (void)sizeof(x); } while (0)
#else
    #define DEBUG_ONLY(...) __VA_ARGS__
    #define DEBUG_VAR
    // #define ASSERT(x) do { assert(x); } while (0)
#endif

#define UNREACHABLE() do { \
        debugf("Unreachable code " __FILE__ ":%d", __LINE__); \
        ::abort(); \
    } while (0)

#define GUARANTEE(x) do { \
        if (!(x)) { \
            debugf("Guarantee failed [" #x "] " __FILE__ ":%d", __LINE__); \
            ::abort(); \
        } \
    } while (0)
#define GUARANTEE_ERR(x) do { \
        if (!(x)) { \
            char errno_buffer[100]; \
            char *err_str = strerror_r(errno, errno_buffer, sizeof(errno_buffer)); \
            debugf("Guarantee failed [" #x "] errno = %d (%s) " __FILE__ ":%d", \
                   errno, err_str, __LINE__); \
            ::abort(); \
        } \
    } while (0)

// Disable this so we can use -Wpedantic in clang
//#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define debugf(format, ...) printf("Thread %" PRIi64 " - " format "\n", indecorous::thread_self_id(), ##__VA_ARGS__)

namespace indecorous {
    typedef int fd_t;
    int64_t thread_self_id();
}

#endif // COMMON_HPP_
