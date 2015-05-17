#include "coro/sched.hpp"

#include <sys/time.h>
#include <limits.h>

#include "coro/coro.hpp"
#include "coro/errors.hpp"

CoroScheduler* CoroScheduler::s_instance = nullptr;
__thread CoroScheduler::Thread* CoroScheduler::Thread::s_instance = nullptr;
__thread size_t CoroScheduler::Thread::s_threadId = -1;

CoroScheduler::CoroScheduler(size_t numThreads) :
  m_running(false),
  m_active_contexts(0),
  m_numThreads(numThreads)
{
  if (s_instance != nullptr)
    throw coro_exc_t("CoroScheduler instance already exists");

  pthread_barrier_init(&m_barrier, nullptr, m_numThreads + 1);

  // Create Thread objects for each thread
  m_threads = new Thread*[m_numThreads];
  for (size_t i = 0; i < m_numThreads; ++i) {
    m_threads[i] = new Thread(this, i, &m_barrier);
  }

  // Wait for all threads to start up
  pthread_barrier_wait(&m_barrier);

  s_instance = this;
}

CoroScheduler::~CoroScheduler() {
  assert(s_instance == this);
  assert(!m_running);

  if (m_active_contexts != 0)
    printf("Warning: %d coroutines have not completed\n", m_active_contexts.load());

  // Tell all threads to shutdown once we hit the following barrier
  for (size_t i = 0; i < m_numThreads; ++i) {
    m_threads[i]->shutdown();
  }

  // Tell threads to exit
  pthread_barrier_wait(&m_barrier);

  // Wait for threads to exit
  pthread_barrier_wait(&m_barrier);
  pthread_barrier_destroy(&m_barrier);

  for (size_t i = 0; i < m_numThreads; ++i) {
    delete m_threads[i];
  }
  delete [] m_threads;

  s_instance = nullptr;
}

size_t CoroScheduler::getThreadId() {
  return Thread::s_threadId;
}

CoroDispatcher& CoroScheduler::getDispatcher(size_t threadId) {
  assert(s_instance != nullptr);
  if (threadId >= s_instance->m_numThreads)
    throw coro_exc_t("threadId out of bounds");

  return *s_instance->m_threads[threadId]->m_dispatcher;
}

// TODO: reimplement this using templates/parameter packs
/*
void CoroScheduler::spawn(size_t threadId, void (fn)(void*), void* p) {
  if (m_running)
    throw coro_exc_t("cannot externally spawn a coroutine while already running");

  if (threadId >= m_numThreads)
    throw coro_exc_t("threadId out of bounds");

  // Coroutines are not running at the moment, so we can do what we please
  //  Note: this is not thread-safe
  CoroDispatcher* dispatch = m_threads[threadId]->m_dispatcher;
  coro_t* context = dispatch->m_contextArena.get(fn, p, dispatch, nullptr);
  dispatch->pushForeignContext(context);
  ++m_active_contexts;
}
*/

void CoroScheduler::run() {
  if (m_running.exchange(true)) {
    throw coro_exc_t("CoroScheduler already running");
  }

  // Wait for all threads to get to their loop
  pthread_barrier_wait(&m_barrier);

  // Wait for all threads to finish all their coroutines
  pthread_barrier_wait(&m_barrier);

  m_running.store(false);
}

struct thread_args_t {
  CoroScheduler::Thread* instance;
  size_t threadId;
};

CoroScheduler::Thread::Thread(CoroScheduler* parent,
                              size_t threadId,
                              pthread_barrier_t* barrier) :
  m_parent(parent),
  m_shutdown(false),
  m_barrier(barrier)
{
  // Launch pthread for the new thread, then return
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  thread_args_t* args = new thread_args_t;

  args->instance = this;
  args->threadId = threadId;

  pthread_create(&m_pthread, &attr, &threadHook, args);
  pthread_attr_destroy(&attr);
}

void* CoroScheduler::Thread::threadHook(void* param) {
  thread_args_t *args = reinterpret_cast<thread_args_t*>(param);
  s_threadId = args->threadId;
  s_instance = args->instance;
  delete args;

  s_instance->m_dispatcher = new CoroDispatcher();

  s_instance->threadMain();

  delete s_instance->m_dispatcher;
  s_instance->m_dispatcher = nullptr;

  // Last barrier is for the CoroScheduler destructor,
  //  indicates that the Thread objects are safe for destruction
  pthread_barrier_wait(s_instance->m_barrier);

  return nullptr;
}

void CoroScheduler::Thread::threadMain() {
  // First barrier is for the CoroScheduler constructor,
  //  indicates that the dispatchers are ready
  pthread_barrier_wait(m_barrier);

  while (true) {
    // Wait for CoroScheduler::run or ~CoroScheduler
    pthread_barrier_wait(m_barrier);

    if (m_shutdown)
      break;

    while (m_dispatcher->run() > 0) {
      doWait();
    }

    // Wait for other threads to finish
    pthread_barrier_wait(m_barrier);
  }
}

void CoroScheduler::Thread::shutdown() {
  m_shutdown = true;
}

CoroScheduler::Thread& CoroScheduler::Thread::getInstance() {
  assert(s_instance != nullptr);
  return *s_instance;
}

void CoroScheduler::Thread::buildPollSet(struct pollfd* pollArray) {
  // TODO don't fully generate this every time
  // Build up poll array for subscribed file events
  size_t index = 0;

  pollArray[index].fd = 0;
  pollArray[index].events = 0;
  for (auto i = m_fileWaiters.begin(); i != m_fileWaiters.end(); ++i) {
    if (pollArray[index].fd == 0) {
      pollArray[index].fd = i->first.fd;
      pollArray[index].events = i->first.event;
    } else if (i->first.fd == pollArray[index].fd) {
      pollArray[index].events |= i->first.event;
    } else {
      ++index;
      pollArray[index].fd = 0;
      pollArray[index].events = 0;
    }
  }
}

// TODO: pass error events using WaitError or whatever
void CoroScheduler::Thread::notifyWaiters(size_t size, struct pollfd* pollArray) {
  // Call any triggered file waiters
  for (size_t index = 0; index < size; ++index) {
    // Check each flag and call associated file waiters
    notifyWaitersForEvent(&pollArray[index], POLLIN);
    notifyWaitersForEvent(&pollArray[index], POLLOUT);
    notifyWaitersForEvent(&pollArray[index], POLLERR);
    notifyWaitersForEvent(&pollArray[index], POLLHUP);
    notifyWaitersForEvent(&pollArray[index], POLLPRI);
    notifyWaitersForEvent(&pollArray[index], POLLRDHUP);
  }
}

void CoroScheduler::Thread::notifyWaitersForEvent(struct pollfd* pollEvent, int eventMask) {
  if (pollEvent->revents & eventMask) {
    file_wait_info_t info = { pollEvent->fd, eventMask };

    for (auto i = m_fileWaiters.lower_bound(info);
         i != m_fileWaiters.end() && i->first == info; ++i)
       i->second->wait_callback(wait_result_t::Success);
  }
}

void CoroScheduler::Thread::doWait() {
  // Wait on timers and file descriptors
  size_t pollSize = m_fileWaiters.size(); // Check if this works for multimaps
  struct pollfd pollArray[1]; // TODO: RSI: fix variable length array

  buildPollSet(pollArray);
  int timeout = getWaitTimeout(); // This will handle any timer callbacks

  // Do the wait
  int res;
  do {
    res = poll(pollArray, pollSize, timeout);
  } while (res == -1 && errno == EINTR);

  if (res == -1)
    throw coro_exc_t("poll failed");
  else if (res != 0)
    notifyWaiters(pollSize, pollArray);
  else
    getWaitTimeout(); // Time out, a timer has probably expired, so notify the waiter
}

int CoroScheduler::Thread::getWaitTimeout() {
  if (m_dispatcher->m_runQueue.size() > 0)
    return 0;

  if (m_timerWaiters.empty())
    return -1;

  uint64_t now = getEndTime(0);
  uint64_t delta = INT_MAX;

  // Notify any elapsed timers and find the delta until the following timer
  for (auto i = m_timerWaiters.begin(); !m_timerWaiters.empty(); i = m_timerWaiters.begin()) {
    if (i->first <= now) {
      i->second->wait_callback(wait_result_t::Success);
      m_timerWaiters.erase(i);
    } else {
      delta = i->first - now;
      break;
    }
  }

  return (delta > INT_MAX) ? INT_MAX : (int)delta;
}

uint64_t CoroScheduler::Thread::getEndTime(uint32_t timeout) {
  struct timeval time;
  if (gettimeofday(&time, nullptr) != 0)
    throw coro_exc_t("gettimeofday failed");

  uint64_t now = (time.tv_usec / 1000) + (time.tv_sec * 1000);
  return (now + timeout);
}

void CoroScheduler::Thread::addTimer(wait_callback_t* cb, uint32_t timeout) {
  // TODO: do a better than O(n) search - store expiration time in timer?
  Thread& thread = getInstance();

  auto i = thread.m_timerWaiters.begin();
  for (; i != thread.m_timerWaiters.end() && i->second != cb; ++i);
  assert(i == thread.m_timerWaiters.end());

  uint64_t endTime = getEndTime(timeout);
  thread.m_timerWaiters.insert(std::make_pair(endTime, cb));
}

void CoroScheduler::Thread::updateTimer(wait_callback_t* cb, uint32_t timeout) {
  // TODO: do a better than O(n) search - store expiration time in timer?
  Thread& thread = getInstance();

  auto i = thread.m_timerWaiters.begin();
  for (; i != thread.m_timerWaiters.end() && i->second != cb; ++i);
  assert(i != thread.m_timerWaiters.end());
  thread.m_timerWaiters.erase(i);

  uint64_t endTime = getEndTime(timeout);
  thread.m_timerWaiters.insert(std::make_pair(endTime, cb));
}

void CoroScheduler::Thread::removeTimer(wait_callback_t* cb) {
  // TODO: do a better than O(n) search - store expiration time in timer?
  Thread& thread = getInstance();
  auto i = thread.m_timerWaiters.begin();
  for (; i != thread.m_timerWaiters.end() && i->second != cb; ++i);
  assert(i != thread.m_timerWaiters.end());
  thread.m_timerWaiters.erase(i);
}

bool CoroScheduler::Thread::addFileWait(int fd, int eventMask, wait_callback_t* cb) {
  assert(eventMask == POLLRDHUP ||
         eventMask == POLLERR ||
         eventMask == POLLHUP ||
         eventMask == POLLOUT ||
         eventMask == POLLPRI ||
         eventMask == POLLIN);

  Thread& thread = getInstance();
  file_wait_info_t info = { fd, eventMask };

  for (auto i = thread.m_fileWaiters.lower_bound(info);
       i != thread.m_fileWaiters.end() && i->first == info; ++i) {
    if (i->second == cb)
      return false;
  }

  thread.m_fileWaiters.insert(std::make_pair(info, cb));
  return true;
}

bool CoroScheduler::Thread::removeFileWait(int fd, int eventMask, wait_callback_t* cb) {
  Thread& thread = getInstance();
  file_wait_info_t info = { fd, eventMask };

  // Find the specified item in the map
  for (auto i = thread.m_fileWaiters.lower_bound(info);
       i != thread.m_fileWaiters.end() && i->first == info; ++i) {
    if (i->second == cb) {
      thread.m_fileWaiters.erase(i);
      return true;
    }
  }

  return false;
}

