#ifndef UTILS_HPP_
#define UTILS_HPP_

#include "sync/multiple_wait.hpp"

namespace indecorous {

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
auto eintr_wrap(Callable &&c, waitable_t *w) {
    while (true) {
        auto res = c();
        if (res != -1) { return res; }
        GUARANTEE_ERR(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK);
        w->wait();
    }
}

} // namespace indecorous

#endif // UTILS_HPP_
