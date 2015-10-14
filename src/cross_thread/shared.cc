#include "cross_thread/shared.hpp"

namespace indecorous {

shared_registry_t::shared_registry_t() :
        m_id(uuid_t::generate(&true_random_t::s_instance)),
        m_next_entry_index(0),
        m_entries() { }

const uuid_t &shared_registry_t::id() const {
    return m_id;
}

void shared_registry_t::remove(uint64_t index) {
    size_t res = m_entries.erase(index);
    assert(res == 1);
}

size_t shared_registry_t::next_type_id() {
    static size_t next = 0;
    return next++;
}

} // namespace indecorous
