#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <string>

#include "sync/multiple_wait.hpp"

namespace indecorous {

std::string strprintf(const char *format, ...) __attribute__((format(printf, 1, 2)));
std::string string_error(int err);

template <typename Callable>
auto eintr_wrap(Callable &&c) {
    auto res = c();
    if (res != -1) { return res; }
    while (errno == EINTR || errno == EAGAIN) {
        res = c();
        if (res != -1) { return res; }
    }
    return res;
}

// Same as above, except it handles EAGAIN by waiting on the specified wait object
template <typename Callable>
auto eintr_wrap(Callable &&c, waitable_t *w) {
    auto res = c();
    if (res != -1) { return res; }
    while (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        w->wait();
        res = c();
        if (res != -1) { return res; }
    }
    return res;
}

} // namespace indecorous

#endif // UTILS_HPP_
