#include "utils.hpp"

#include <cstdarg>

namespace indecorous {

std::string strprintf(const char *format, ...) {
    std::vector<char> buffer;

    va_list ap;
    va_start(ap, format);
    va_list bp;
    va_copy(bp, ap);

    int res = vsnprintf(buffer.data(), 0, format, ap);
    va_end(ap);

    buffer.resize(res + 1);
    res = vsnprintf(buffer.data(), buffer.size(), format, bp);
    va_end(bp);

    assert(static_cast<size_t>(res) < buffer.size());
    return std::string(buffer.data(), buffer.size() - 1);
}

std::string string_error(int err) {
    std::vector<char> buffer;
    buffer.resize(100);
    return std::string(strerror_r(err, buffer.data(), buffer.size()));
}

} // namespace indecorous
