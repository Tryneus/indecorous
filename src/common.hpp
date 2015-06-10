#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <cstdint>
#include <inttypes.h>

// TODO: make sure these aren't exposed to users
#ifdef NDEBUG
    #define DEBUG_ONLY(...)
#else
    #define DEBUG_ONLY(...) __VA_ARGS__
#endif

#define debugf(format, ...) printf("Thread %" PRIi32 " - " format, indecorous::thread_self_id(), ##__VA_ARGS__)

namespace indecorous {

int32_t thread_self_id();

} // namespace indecorous

#endif // COMMON_HPP_
