#include "coro/coro_event.hpp"

#include "common.hpp"
#include "coro/errors.hpp"

coro_event_t::coro_event_t(bool autoReset, bool wakeAll) :
  m_autoReset(autoReset),
  m_wakeAll(wakeAll),
  m_triggered(false)
{
  if (!autoReset && !wakeAll) {
    throw parameter_exc_t("behavior is undefined when both autoReset and wakeAll are false");
  }
}

coro_event_t::~coro_event_t() {
  // Fail any remaining waits
  while (!m_waiters.empty()) {
    m_waiters.pop()->wait_callback(wait_result_t::ObjectLost);
  }
}

bool coro_event_t::triggered() const {
  return m_triggered;
}

bool coro_event_t::set() {
  if (m_triggered)
    return false;

  if (m_waiters.empty()) {
    m_triggered = true;
  } else {
    if (!m_autoReset)
      m_triggered = true;

    if (m_wakeAll) {
      while (!m_waiters.empty())
        m_waiters.pop()->wait_callback(wait_result_t::Success);
    } else if (!m_waiters.empty())
      m_waiters.pop()->wait_callback(wait_result_t::Success);
  }

  return true;
}

bool coro_event_t::reset() {
  if (!m_triggered)
    return false;
  m_triggered = false;
  return true;
}

void coro_event_t::wait() {
  if (m_triggered) {
    assert(m_waiters.empty());
    if (m_autoReset)
      m_triggered = false;
  } else {
    DEBUG_ONLY(coro_t* self = coro_t::self());
    m_waiters.push(coro_t::self());
    coro_t::wait();
    assert(coro_t::self() == self);
  }
}

void coro_event_t::addWait(wait_callback_t* cb) {
  if (m_triggered) {
    assert(m_waiters.empty());
    if (m_autoReset)
      m_triggered = false;
    cb->wait_callback(wait_result_t::Success);
  } else {
    m_waiters.push(cb);
  }
}

void coro_event_t::removeWait(wait_callback_t* cb) {
  m_waiters.remove(cb);
}

