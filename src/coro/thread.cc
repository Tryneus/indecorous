#include "coro/thread.hpp"

#include <signal.h>
#include <sys/time.h>

#include <limits>

#include "coro/barrier.hpp"
#include "coro/coro.hpp"
#include "coro/sched.hpp"
#include "coro/shutdown.hpp"
#include "errors.hpp"
#include "sync/interruptor.hpp"

namespace indecorous {

thread_local thread_t* thread_t::s_instance = nullptr;

thread_t::thread_t(scheduler_t *parent,
                   target_t *io_target,
                   std::function<void()> inner_main,
                   std::function<void()> coro_pull) :
        m_parent(parent),
        m_direct_stream(),
        m_direct_target(&m_direct_stream),
        m_hub(&m_direct_target, io_target),
        m_events(),
        m_dispatcher(nullptr),
        m_shutdown_event(),
        m_stop_immediately(false),
        m_inner_main(std::move(inner_main)),
        m_coro_pull(std::move(coro_pull)),
        m_thread(&thread_t::main, this) { }

shared_registry_t *thread_t::get_shared_registry() {
    return &m_parent->m_shared_registry;
}

void thread_t::note_local_rpc() {
    m_parent->m_shutdown->update(1);
}

void thread_t::begin_shutdown() {
    m_shutdown_event.set();
}

void thread_t::finish_shutdown() {
    assert(m_shutdown_event.triggered());
    m_stop_immediately = true;
}

void thread_t::join() {
    m_thread.join();
}

// This should be instantiated at the beginning of a system coroutine
// (aside from the initial coroutine, which is implicitly handled) to
// counteract coroutine delta tracking so that it doesn't count against
// shutdown.
class ignore_coro_for_shutdown_t {
public:
    ignore_coro_for_shutdown_t() {
        thread_t::self()->dispatcher()->m_coro_delta -= 1;
    }
    ~ignore_coro_for_shutdown_t() {
        thread_t::self()->dispatcher()->m_coro_delta += 1;
    }
};

void thread_t::main() {
    logDebug("Starting");
    s_instance = this;

    m_parent->m_barrier.wait(); // Barrier for the scheduler_t constructor, thread ready
    m_parent->m_barrier.wait(); // Wait for run or ~scheduler_t

    m_stop_immediately = false;
    event_t close_event;
    m_dispatcher = std::make_unique<dispatcher_t>(
        m_parent->m_shutdown.get(),
        [&] {
            try {
                interruptor_t shutdown(&close_event);
                auto result = coro_t::spawn([&] {
                    ignore_coro_for_shutdown_t ignore;
                    try {
                        m_coro_pull();
                    } catch (wait_interrupted_exc_t &) {
                        // Do nothing
                    }
                });

                while (true) {
                    read_message_t msg = m_direct_stream.read();
                    if (msg.buffer.has()) {
                        m_hub.spawn_task(std::move(msg));
                    } else {
                        m_direct_stream.wait();
                    }
                }
            } catch (wait_interrupted_exc_t &) {
                // Do nothing
            }
        });

    while (!m_parent->m_destroying.load()) {
        logDebug("Running");
        while (!m_stop_immediately) {
            m_inner_main();
        }

        logDebug("Pausing");
        m_parent->m_barrier.wait(); // Wait for other threads to finish
        m_parent->m_barrier.wait(); // Wait for run or ~scheduler_t
    }


    logDebug("Exiting");
    close_event.set();
    m_dispatcher->run();
    m_dispatcher.reset();

    m_parent->m_barrier.wait(); // Barrier for ~scheduler_t, safe to destruct
    s_instance = nullptr;
}

coro_thread_t::coro_thread_t(scheduler_t *parent, target_t *io_target) :
    m_thread(parent, io_target,
             std::bind(&coro_thread_t::inner_main, this),
             [] { }) { }

void coro_thread_t::inner_main() {
    m_thread.events()->check(m_thread.dispatcher()->m_run_queue.empty());
    m_thread.dispatcher()->run();
}

io_thread_t::io_thread_t(scheduler_t *parent,
                         target_t *io_target,
                         io_stream_t *io_stream) :
    m_io_stream(io_stream),
    m_ready_for_next(),
    m_thread(parent, io_target,
             std::bind(&io_thread_t::inner_main, this),
             std::bind(&io_thread_t::coro_pull, this)) { }

void io_thread_t::coro_pull() {
    while (true) {
        m_ready_for_next.wait();
        m_ready_for_next.reset();
        read_message_t msg = m_io_stream->read();
        m_thread.hub()->spawn_task(std::move(msg));
    }
}

void io_thread_t::inner_main() {
    m_ready_for_next.set();
    m_thread.events()->check(m_thread.dispatcher()->m_run_queue.empty());
    m_thread.dispatcher()->run();
    while (m_thread.dispatcher()->m_coro_cache.extant() > 2) {
        m_thread.events()->check(m_thread.dispatcher()->m_run_queue.empty());
        m_thread.dispatcher()->run();
    }
}

} // namespace indecorous

