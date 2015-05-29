#ifndef CORO_SHUTDOWN_HPP_
#define CORO_SHUTDOWN_HPP_

#include <atomic>
#include <cstddef>

namespace indecorous {

class shutdown_t {
public:
    shutdown_t(size_t num_threads);

    void shutdown();
    bool shutting_down() const;

    // context_ts do not have thread-safe construction - construct them from
    // the main thread only.
    class context_t {
    public:
        context_t();
        void reset(shutdown_t *parent);

        // Returns true if the thread should stop immediately
        // This may do a bit of extra work, but it is fine if we're shutting down
        bool update(bool had_activity);
        bool shutting_down() const;
    private:
        shutdown_t *m_parent;
        size_t m_index;
    };

private:
    static int64_t make_done_flags(size_t count);

    void reset_activity();
    bool no_activity(size_t index);

    enum class state_t {
        Running,
        ShuttingDown,
        Finished,
    };

    const uint64_t m_done_flags;
    std::atomic<uint64_t> m_stage_one_flags;
    std::atomic<uint64_t> m_stage_two_flags;
    std::atomic<state_t> m_state;
    size_t m_next_context;

};

} // namespace indecorous

#endif // CORO_SHUTDOWN_HPP_
