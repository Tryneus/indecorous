#ifndef IO_FILE_HPP_
#define IO_FILE_HPP_

#include <aio.h>

#include <string>
#include <unordered_set>

#include "common.hpp"
#include "containers/intrusive.hpp"
#include "containers/file.hpp"
#include "sync/drainer.hpp"
#include "sync/promise.hpp"

namespace indecorous {

class file_t {
public:
    file_t(std::string filename, int flags, int permissions = 0600);
    file_t(file_t &&other); // Cannot be called while there are outstanding requests
    ~file_t();

    future_t<int> write(uint64_t offset, const void *buffer, size_t size);
    future_t<int> read(uint64_t offset, void *buffer, size_t size);

private:
    static void done_callback(sigval_t val);
    void pull_results();

    struct request_t : public intrusive_node_t<request_t> {
        request_t(file_t *parent, uint64_t offset, void *buffer, size_t size);

        aiocb m_acb;
        file_t *m_parent;
        promise_t<int> m_promise;
        drainer_lock_t m_drainer_lock;

        DISABLE_COPYING(request_t);
    };

    const std::string m_filename;
    const int m_flags;

    const scoped_fd_t m_file;
    const scoped_fd_t m_eventfd;

    std::unordered_set<request_t *> m_extant_requests;
    mpsc_queue_t<request_t> m_completed_requests;

    // This drainer keeps us from being destroyed until all extant requests have
    // been completed or completely canceled.
    drainer_t m_drainer;

    DISABLE_COPYING(file_t);
};

} // namespace indecorous

#endif // IO_FILE_HPP_
