#include "coro/coro.hpp"

#include <stdexcept>
#include <sys/eventfd.h>
#include <valgrind/valgrind.h>
#include <iostream>
#include <unistd.h>

#include "coro/coro_event.hpp"
#include "coro/sched.hpp"
#include "coro/errors.hpp"

uint32_t CoroDispatcher::s_max_swaps_per_loop = 100;

CoroDispatcher::foreign_queue_event_t::foreign_queue_event_t() :
  m_event(eventfd(0, EFD_CLOEXEC))
{
  if (m_event == -1) 
    throw coro_exc_t("failed to create foreignRunQueue eventfd");

  CoroScheduler::Thread::addFileWait(m_event, POLLIN, this);
}

CoroDispatcher::foreign_queue_event_t::~foreign_queue_event_t() {
  CoroScheduler::Thread::removeFileWait(m_event, POLLIN, this);
  close(m_event);
}

void CoroDispatcher::foreign_queue_event_t::notify() {
  // TODO: add an atomic value here to reduce system calls, if it can be done without racing
  uint64_t data = 1;
  write(m_event, &data, sizeof(data));
}

void CoroDispatcher::foreign_queue_event_t::wait_callback(wait_result_t result) {
  assert(result == wait_result_t::Success);
  uint64_t data;
  read(m_event, &data, sizeof(data));
  CoroDispatcher::s_instance->pullForeignRunQueue();
}

__thread CoroDispatcher* CoroDispatcher::s_instance = nullptr;

CoroDispatcher::CoroDispatcher(std::atomic<int32_t>* active_contexts) :
  m_self(nullptr),
  m_active_contexts(active_contexts),
  m_moveContext(nullptr),
  m_moveTarget(nullptr),
  m_releaseContext(nullptr),
  m_contextArena(256)
{
  assert(s_instance == nullptr);
  s_instance = this;
}

CoroDispatcher::~CoroDispatcher() {
  assert(m_self == nullptr);
  assert(s_instance == this);
  s_instance = nullptr;
}

CoroDispatcher& CoroDispatcher::getInstance() {
  if (s_instance == nullptr)
    throw coro_exc_t("CoroDispatcher instance not found");
  return *s_instance;
}

uint32_t CoroDispatcher::run() {
  assert(s_instance == this);
  assert(m_self == nullptr);
  m_swap_count = 0;

  if (!m_runQueue.empty()) {
    // Save the currently running context
    assert(getcontext(&m_parentContext) == 0);

    // Start the queue of coroutines - they will swap in the next until no more are left
    m_self = m_runQueue.pop();
    assert(m_self != nullptr);
    swapcontext(&m_parentContext, &m_self->m_context);

    // If the last context had requested a move, move it
    if (m_moveContext != nullptr)
      moveContext();

    if (m_releaseContext != nullptr)
      releaseContext();
  }

  assert(m_self == nullptr);
  assert(s_instance == this);
  return m_active_contexts->load();
}

void coro_t::swap() {
  // Make sure we are currently inside a coroutine on the right thread
  assert(m_dispatcher == &CoroDispatcher::getInstance());
  assert(m_dispatcher->m_self == this);
  ++m_dispatcher->m_swap_count;

  if (m_dispatcher->m_runQueue.empty() ||
      m_dispatcher->m_swap_count >= CoroDispatcher::s_max_swaps_per_loop) {
    m_dispatcher->m_self = nullptr;
    swapcontext(&m_context, &m_dispatcher->m_parentContext);
  } else {
    m_dispatcher->m_self = m_dispatcher->m_runQueue.pop();
    if (m_dispatcher->m_self != this)
      swapcontext(&m_context, &m_dispatcher->m_self->m_context);
  }
  assert(m_dispatcher->m_self == this);

  // If the last context had requested a move, move it
  if (m_dispatcher->m_moveContext != nullptr)
    m_dispatcher->moveContext();

  if (m_dispatcher->m_releaseContext != nullptr)
    m_dispatcher->releaseContext();

  // If we did a wait that failed, throw an appropriate exception,
  //  so we can require the user to handle it
  wait_result_t waitResult = m_waitResult;
  m_waitResult = wait_result_t::Success;
  switch (waitResult) {
  case wait_result_t::Success:
    break;
  case wait_result_t::Interrupted:
    throw wait_interrupted_exc_t();
  case wait_result_t::ObjectLost:
    throw wait_object_lost_exc_t();
  case wait_result_t::Error:
  default:
    throw wait_error_exc_t("unrecognized error while waiting");
  }
}

void CoroDispatcher::moveContext() {
  assert(m_moveContext != nullptr);
  assert(m_moveTarget != nullptr);

  m_moveTarget->pushForeignContext(m_moveContext);
  m_moveContext = nullptr;
  m_moveTarget = nullptr;
}

void CoroDispatcher::pushForeignContext(coro_t* context) {
  // The coroutine will have no assigned dispatcher until it is picked up by the other side
  // This should prevent race conditions from other operations in the meantime
  context->m_dispatcher = nullptr;
  m_foreignRunQueue.push(context);
  m_foreign_queue_event.notify();
}

void CoroDispatcher::releaseContext() {
  assert(m_releaseContext != nullptr);
  m_contextArena.release(m_releaseContext);
  m_releaseContext = nullptr;
  m_active_contexts->fetch_sub(1);
}

void CoroDispatcher::pullForeignRunQueue() {
  size_t i = 0;
  for (coro_t* coro = m_foreignRunQueue.pop();
       coro != nullptr; coro = m_foreignRunQueue.pop()) {
    assert(coro != nullptr);
    assert(coro->m_dispatcher == nullptr);
    coro->m_dispatcher = this;
    m_runQueue.push(coro);
    ++i;
  }
}

coro_t::coro_t(void(*fn)(void*),
               void* param, 
               CoroDispatcher* dispatcher,
               coro_event_t* done_event) :
  m_param(param),
  m_fn(fn),
  m_dispatcher(dispatcher),
  m_done_event(done_event),
  m_waitResult(wait_result_t::Success)
{
  assert(getcontext(&m_context) == 0);
  m_context.uc_stack.ss_sp = m_stack;
  m_context.uc_stack.ss_size = s_stackSize;
  m_context.uc_link = nullptr;
  makecontext(&m_context, (void(*)())hook, 1, this);
  m_valgrindStackId = VALGRIND_STACK_REGISTER(m_stack, m_stack + sizeof(m_stack));
}

coro_t::~coro_t() {
  VALGRIND_STACK_DEREGISTER(m_valgrindStackId);
}

coro_t* coro_t::self() {
  return CoroDispatcher::getInstance().m_self;
}

void coro_t::wait() {
  CoroDispatcher& dispatch = CoroDispatcher::getInstance();
  coro_t *coro = self();
  assert(coro != nullptr);
  assert(coro->m_dispatcher == &dispatch);
  coro->swap();
}

void coro_t::yield() {
  CoroDispatcher& dispatch = CoroDispatcher::getInstance();
  coro_t *coro = self();
  assert(coro != nullptr);
  assert(coro->m_dispatcher == &dispatch);

  dispatch.m_runQueue.push(coro);
  coro->swap();
}

void coro_t::spawn(void(*fn)(void*), void* p, coro_event_t* done_event) {
  CoroDispatcher& dispatch = CoroDispatcher::getInstance();
  coro_t* context = dispatch.m_contextArena.get(fn, p, &dispatch, done_event);
  dispatch.m_runQueue.push(context);
  dispatch.m_active_contexts->fetch_add(1);
}

// TODO: spawn this function on a separate dispatcher for blocking calls
void coro_t::spawn_blocking(void(*fn)(void*), void* p, coro_event_t* done_event) {
  CoroDispatcher& dispatch = CoroDispatcher::getInstance();
  coro_t* context = dispatch.m_contextArena.get(fn, p, &dispatch, done_event);
  dispatch.m_runQueue.push(context);
  dispatch.m_active_contexts->fetch_add(1);
}

void coro_t::spawn_now(void(*fn)(void*), void* p, coro_event_t* done_event) {
  CoroDispatcher& dispatch = CoroDispatcher::getInstance();
  coro_t* context = dispatch.m_contextArena.get(fn, p, &dispatch, done_event);
  dispatch.m_runQueue.push_front(context);
  dispatch.m_active_contexts->fetch_add(1);
}

void coro_t::spawn_on(size_t threadId, void(*fn)(void*), void* p, coro_event_t* done_event) {
  CoroDispatcher& dispatch = CoroDispatcher::getInstance();
  CoroDispatcher& other_dispatch = CoroScheduler::getDispatcher(threadId);

  // Allocate from this thread, then push to the other thread
  coro_t* context = dispatch.m_contextArena.get(fn, p, &other_dispatch, done_event);
  other_dispatch.pushForeignContext(context);
  dispatch.m_active_contexts->fetch_add(1);
}

void coro_t::notify(coro_t* coro) {
  if (coro == nullptr) {
    throw coro_exc_t("coro_t::notify called with a nullptr pointer");
  }

  CoroDispatcher& dispatch = CoroDispatcher::getInstance();

  if (&dispatch != coro->m_dispatcher) {
    throw coro_exc_t("cannot notify a coroutine that is assigned to another thread");
  } else if (self() == coro) {
    throw coro_exc_t("cannot notify the running coroutine");
  }

  dispatch.m_runQueue.push(coro);
}

// TODO: problem with allowing any coroutine to move any other coroutine on the same thread:
//  the other coroutine may be waiting on events, and it will not be safe for those events to
//  notify the coroutine once it has moved.  It is intended that by requiring the coroutine to
//  be queued, that it is not waiting on any events.  This requires the synchronization
//  primitives to play along, so this case never happens.  The TODO is to go through and
//  make sure this situation can't arise, or at least have some strong asserts.
void coro_t::move(size_t threadId) {
  CoroDispatcher& dispatch = CoroDispatcher::getInstance();
  if (m_dispatcher != &dispatch) {
    throw coro_exc_t("cannot move a coroutine that is assigned to another thread");
  }

  CoroDispatcher& other_dispatch = CoroScheduler::getDispatcher(threadId);

  if (this == self()) {
    // Moving should be done in the context of the next coroutine to prevent a race condition
    dispatch.m_moveContext = this;
    dispatch.m_moveTarget = &other_dispatch;
    swap();
  } else {
    if (QueueNode<coro_t>::m_queueNodeParent == nullptr) {
      throw coro_exc_t("cannot move a coroutine that is not queued to run");
    }

    assert(QueueNode<coro_t>::m_queueNodeParent == &dispatch.m_runQueue);
    dispatch.m_runQueue.remove(this);

    dispatch.m_moveContext = this;
    dispatch.m_moveTarget = &other_dispatch;
    dispatch.moveContext();
  }
}

void coro_t::hook(void* instance) {
  assert(instance != nullptr);
  reinterpret_cast<coro_t*>(instance)->begin();
  assert(false);
}

void coro_t::begin() {
  assert(m_dispatcher == &CoroDispatcher::getInstance());
  assert(this == self());
  assert(m_fn != nullptr);

  if (m_dispatcher->m_moveContext != nullptr)
    m_dispatcher->moveContext();

  if (m_dispatcher->m_releaseContext != nullptr)
    m_dispatcher->releaseContext();

  m_fn(m_param);
  m_fn = nullptr; // Probably not necessary

  // Check again after running to make sure nothing moved
  assert(m_dispatcher == &CoroDispatcher::getInstance());
  assert(this == self());

  // Notify any coroutines waiting on this context
  if (m_done_event != nullptr) {
    m_done_event->set();
  }

  m_dispatcher->m_releaseContext = this;
  swap();
}

void coro_t::wait_callback(wait_result_t result) {
  assert(m_waitResult == wait_result_t::Success);
  m_waitResult = result;
  notify(this);
}

