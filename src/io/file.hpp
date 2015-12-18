#ifndef IO_FILE_HPP_
#define IO_FILE_HPP_

#include <aio.h>

#include <exception>
#include <string>
#include <unordered_set>

#include "common.hpp"
#include "containers/intrusive.hpp"
#include "containers/file.hpp"
#include "sync/drainer.hpp"
#include "sync/promise.hpp"

namespace indecorous {

// Thrown when opening a file fails
class file_open_exc_t final : public std::exception {
public:
    file_open_exc_t(std::string filename, int err);

    const char *what() const noexcept override final;
    int error_code() const;
    std::string filename() const;
private:
    int m_err;
    std::string m_filename;
    std::string m_what;
};

class file_t {
public:
    file_t(std::string filename, int flags, int permissions = 0600);
    file_t(file_t &&other); // Cannot be called while there are outstanding requests

    future_t<int> write(off_t offset, const void *buffer, size_t size);
    future_t<int> read(off_t offset, void *buffer, size_t size);
    future_t<int> truncate(off_t size);

private:
    std::string m_filename;
    int m_flags;
    scoped_fd_t m_file;

    DISABLE_COPYING(file_t);
};

} // namespace indecorous

#endif // IO_FILE_HPP_
