#include "coro/file_wait.hpp"

#include "common.hpp"

// TODO: only allow one coroutine to wait on a file at once?
//  maybe get rid of wakeall, so we only allow one wakeup per scheduler loop
file_wait_base_t::file_wait_base_t(int fd, bool wakeAll) :
  m_fd(fd),
  m_triggered(false),
  m_wakeAll(wakeAll)
{ }

file_wait_base_t::~file_wait_base_t() {
  while (!m_waiters.empty()) {
    m_waiters.pop()->wait_callback(wait_result_t::ObjectLost);
  }
}

void file_wait_base_t::wait_callback(wait_result_t result) {
  if (m_wakeAll) {
    while (!m_waiters.empty())
      m_waiters.pop()->wait_callback(result);
  }
  else if (!m_waiters.empty())
    m_waiters.pop()->wait_callback(result);
}

void file_wait_base_t::addWait(wait_callback_t* cb) {
  m_waiters.push(cb);
}

void file_wait_base_t::removeWait(wait_callback_t* cb) {
  m_waiters.remove(cb);
}

