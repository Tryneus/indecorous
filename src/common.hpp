#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <cstddef>

// TODO: make sure these aren't exposed to users
#ifdef NDEBUG
    #define DEBUG_ONLY(...)
#else
    #define DEBUG_ONLY(...) __VA_ARGS__
#endif

namespace indecorous {

size_t thread_self_id();

#define debugf(format, ...) printf("Thread %zu - " format, thread_self_id(), ##__VA_ARGS__)

} // namespace indecorous

#endif // COMMON_HPP_
