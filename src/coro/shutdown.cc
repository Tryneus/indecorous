#include "coro/shutdown.hpp"

#include <cassert>

namespace indecorous {

shutdown_t::shutdown_t(size_t num_threads) :
        m_done_flags(make_done_flags(num_threads)),
        m_stage_one_flags(0),
        m_stage_two_flags(0),
        m_state(state_t::Running),
        m_next_context(0) {
    assert(num_threads <= 64);
}

void shutdown_t::shutdown() {
    state_t old_val = m_state.exchange(state_t::ShuttingDown);
    assert(old_val == state_t::Running);
}

bool shutdown_t::shutting_down() const {
    return (m_state.load() != state_t::Running);
}

int64_t shutdown_t::make_done_flags(size_t count) {
    int64_t res = 0;
    for (size_t i = 0; i < count; ++i) {
        res |= (1 << i);
    }
    return res;
}

void shutdown_t::reset_activity() {
    assert(m_state.load() != state_t::Finished);
    m_stage_one_flags.store(0);
    m_stage_two_flags.store(0);
}

bool shutdown_t::no_activity(size_t index) {
    assert(m_state.load() == state_t::ShuttingDown);
    assert(index < 64);
    uint64_t mask = 1 << index;
    uint64_t old_flags = m_stage_one_flags.fetch_or(mask);
    if ((old_flags | mask) == m_done_flags) {
        old_flags = m_stage_two_flags.fetch_or(mask);
        if ((old_flags | mask) == m_done_flags) {
            m_state.store(state_t::Finished);
            return true;
        }
    }
    return false;
}

shutdown_t::context_t::context_t() : m_parent(nullptr), m_index(0) { }

void shutdown_t::context_t::reset(shutdown_t *parent) {
    m_parent = parent;
    if (m_parent != nullptr) {
        m_index = m_parent->m_next_context++;
        assert(m_index < m_parent->m_done_flags);
    }
}

bool shutdown_t::context_t::shutting_down() const {
    assert(m_parent != nullptr);
    return m_parent->shutting_down();
}

bool shutdown_t::context_t::update(bool had_activity) {
    assert(m_parent != nullptr);
    if (had_activity) {
        m_parent->reset_activity();
        return false;
    }
    return m_parent->no_activity(m_index);
}

} // namespace indecorous
