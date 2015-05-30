#ifndef COMMON_HPP_
#define COMMON_HPP_

// TODO: make sure these aren't exposed to users
#ifdef NDEBUG
    #define DEBUG_ONLY(...)
#else
    #define DEBUG_ONLY(...) __VA_ARGS__
#endif

namespace indecorous {

#define debugf(format, ...) printf("Thread %ld - " format, thread_t::self()->id().value(), ##__VA_ARGS__)

} // namespace indecorous

#endif // COMMON_HPP_
