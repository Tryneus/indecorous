#include "sync/timer.hpp"

#include "common.hpp"
#include "coro/sched.hpp"

namespace indecorous {

timer_t::timer_t() :
  m_wakeAll(true),
  m_autoReset(true),
  m_running(false),
  m_timeout(-1)
{ }

timer_t::~timer_t()
{
  // Fail any remaining waits
  while (!m_waiters.empty()) {
    m_waiters.pop()->wait_callback(wait_result_t::ObjectLost);
  }

  stop();
}

void timer_t::set(uint32_t timeoutMs, bool autoReset, bool wakeAll) {
  assert(timeoutMs != (uint32_t)-1);
  m_autoReset = autoReset;
  m_timeout = timeoutMs;
  m_wakeAll = wakeAll;
  m_running = true;
  CoroScheduler::Thread::addTimer(this, m_timeout);
}

void timer_t::reset() {
  assert(m_timeout != (uint32_t)-1);
  m_running = true;
  CoroScheduler::Thread::updateTimer(this, m_timeout);
}

bool timer_t::stop() {
  if (!m_running)
    return false;

  CoroScheduler::Thread::removeTimer(this);
  m_running = false;

  // Fail any current waits - this may not be the 'right' thing to do...
  while (!m_waiters.empty()) {
    m_waiters.pop()->wait_callback(wait_result_t::Interrupted);
  }
  return true;
}

void timer_t::wait() {
  DEBUG_ONLY(coro_t* self = coro_t::self());
  m_waiters.push(coro_t::self());
  coro_t::wait();
  assert(coro_t::self() == self);
}

void timer_t::addWait(wait_callback_t* cb) {
  m_waiters.push(cb);
}

void timer_t::removeWait(wait_callback_t* cb) {
  m_waiters.remove(cb);
}

void timer_t::wait_callback(wait_result_t result) {
  assert(m_running);

  if (m_wakeAll)
    while (!m_waiters.empty()) {
      m_waiters.pop()->wait_callback(result);
    }
  else if (!m_waiters.empty())
    m_waiters.pop()->wait_callback(result);

  if (m_autoReset)
    reset();
  else
    m_running = false;
}

} // namespace indecorous