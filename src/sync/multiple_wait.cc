#include "sync/multiple_wait.hpp"

#include "common.hpp"
#include "coro/coro.hpp"

namespace indecorous {

multiple_wait_t::multiple_wait_t() :
  m_waitAll(false),
  m_waiter(nullptr),
  m_cb_arena(5)
{ }

multiple_wait_t::~multiple_wait_t()
{
  assert(m_waiter == nullptr);
  // Deallocate any remaining wait callbacks
  while (!m_waits.empty()) {
    m_cb_arena.release(*m_waits.begin());
    m_waits.erase(m_waits.begin());
  }
}

bool multiple_wait_t::add(wait_object_t* obj) {
  assert(m_waiter == nullptr);
  multiple_wait_callback_t *new_cb = m_cb_arena.get(this, obj);
  return m_waits.insert(new_cb).second;
}

bool multiple_wait_t::remove(wait_object_t* obj) {
  assert(m_waiter == nullptr);
  m_waitSuccesses.erase(obj);
  multiple_wait_callback_t temp(this, obj);
  auto it = m_waits.find(&temp);
  if (it != m_waits.end()) {
    m_cb_arena.release(*it);
    m_waits.erase(it);
    return true;
  }
  return false;
}

void multiple_wait_t::clear() {
  assert(m_waiter == nullptr);
  m_waitSuccesses.clear();
  m_waits.clear();
}

wait_object_t* multiple_wait_t::wait_one() {
  assert(m_waiter == nullptr);
  assert(m_waits.size() > 0);
  wait_object_t* success;

  if (!m_waitSuccesses.empty()) {
    success = *m_waitSuccesses.begin();
    m_waitSuccesses.erase(m_waitSuccesses.begin());
  } else {
    m_waitAll = false;
    wait_internal();
    assert(!m_waitSuccesses.empty());
    success = *m_waitSuccesses.begin();
    m_waitSuccesses.erase(m_waitSuccesses.begin());
  }
  return success;
}

void multiple_wait_t::wait_all() {
  assert(m_waiter == nullptr);
  assert(m_waits.size() > 0);
  m_waitAll = true;
  wait_internal();
  assert(m_waitSuccesses.size() == m_waits.size());
  m_waitSuccesses.clear();
}

void multiple_wait_t::wait_internal() {
  m_waiter = coro_t::self();

  // Add ourselves to each registered wait object
  for (auto i = m_waits.begin(); i != m_waits.end(); ++i) {
    multiple_wait_callback_t* wait_cb = *i;
    wait_cb->m_target->addWait(wait_cb);
  }

  // It is possible that wait successes occurred when we added ourselves to those wait objects
  if ((m_waitAll && m_waitSuccesses.size() != m_waits.size()) ||
      m_waitSuccesses.size() == 0)
    coro_t::wait();

  // Remove ourselves from any registered wait object that wasn't triggered
  if (!m_waitAll && m_waitSuccesses.size() != m_waits.size()) {
    auto j = m_waits.begin();
    while (j != m_waits.end()) {
      multiple_wait_callback_t* wait_cb = *j;
      // TODO: do this in n time rather than n^2
      if (m_waitSuccesses.find(wait_cb->m_target) == m_waitSuccesses.end()) {
        wait_cb->m_target->removeWait(wait_cb);
      }
      ++j;
    }
  }
  m_waiter = nullptr;

#ifndef NDEBUG
  // Make sure that all waits were removed from their lists
  for (auto i = m_waits.begin(); i != m_waits.end(); ++i) {
    assert((*i)->m_intrusive_node_parent == nullptr);
  }
#endif
}

void multiple_wait_t::multiple_wait_callback(wait_object_t* obj,
                                             wait_result_t result) {
  // Just some debug checks
  {
    DEBUG_ONLY(multiple_wait_callback_t temp(this, obj));
    assert(m_waits.find(&temp) != m_waits.end());
    assert(m_waiter != nullptr);
    assert(m_waitSuccesses.find(obj) == m_waitSuccesses.end());
  }

  if (result != wait_result_t::Success) {
    m_waiter->wait_callback(result);
  } else {
    m_waitSuccesses.insert(obj);
    if (!m_waitAll || m_waitSuccesses.size() == m_waits.size()) {
      m_waiter->wait_callback(wait_result_t::Success);
    }
  }
}

} // namespace indecorous
