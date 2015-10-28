#ifndef CROSS_THREAD_SHARED_HPP_
#define CROSS_THREAD_SHARED_HPP_

#include <memory>
#include <utility>
#include <unordered_map>

#include "common.hpp"
#include "coro/thread.hpp"
#include "random.hpp"
#include "rpc/serialize.hpp"

namespace indecorous {

// The shared_registry_t allows creating objects that can be shared between threads.
// These must be created before running the scheduler
class shared_registry_t {
public:
    shared_registry_t();

    const uuid_t &id() const;

    template <typename T>
    T *get(uint64_t index) {
        auto it = m_entries.find(index);
        if (it == m_entries.end()) {
            return nullptr;
        }
        assert(it->second->id() == type_id<T>());
        assert(thread_t::self() != nullptr);
        return reinterpret_cast<T *>(it->second->get());
    }

    template <typename T, typename ...Args>
    uint64_t emplace(Args &&...args) {
        auto res = m_entries.emplace(m_next_entry_index++,
            std::unique_ptr<entry_t>(new entry_impl_t<T>(std::forward<Args>(args)...)));
        assert(res.second);
        return res.first->first;
    }

    void remove(uint64_t index);

private:
    class entry_t {
    public:
        virtual ~entry_t() { }
        virtual void *get() = 0;
        virtual size_t id() const = 0;
    };

    template <typename T>
    class entry_impl_t final : public entry_t {
    public:
        template <typename ...Args>
        entry_impl_t(Args &&...args) :
            m_object(new T(std::forward<Args>(args)...)) { }

        virtual ~entry_impl_t() { }

        size_t id() const override final {
            return shared_registry_t::type_id<T>();
        }
        void *get() override final {
            return m_object.get();
        }
    private:
        T m_object;
    };

    template <typename T>
    static size_t type_id() {
        static size_t local_type_id = next_type_id();
        return local_type_id;
    }

    static size_t next_type_id();

    uuid_t m_id;
    uint64_t m_next_entry_index;
    std::unordered_map<uint64_t, std::unique_ptr<entry_t> > m_entries;

    DISABLE_COPYING(shared_registry_t);
};

template <typename T>
class shared_var_t {
public:
    shared_var_t(shared_var_t<T> &&other) :
            m_registry(other.m_registry),
            m_index(other.m_index),
            m_owner(other.m_owner) {
        other.m_registry = nullptr;
        other.m_index = 0;
        other.m_owner = false;
    }

    ~shared_var_t() {
        if (m_owner) {
            m_registry->remove(m_index);
        }
    }

    const T *get() const {
        assert(m_registry != nullptr);
        return m_registry->get<T>(m_index);
    }

    T *get() {
        assert(m_registry != nullptr);
        return m_registry->get<T>(m_index);
    }

private:
    friend class scheduler_t;
    template <typename ...Args>
    shared_var_t(shared_registry_t *registry, Args &&...args) :
        m_registry(registry),
        m_index(m_registry->emplace(std::forward<Args>(args)...)),
        m_owner(true) { }

    friend struct serializer_t<T>;
    shared_var_t(shared_registry_t *registry, uint64_t index) :
        m_registry(registry),
        m_index(index),
        m_owner(false) { }

    shared_registry_t *m_registry;
    uint64_t m_index;
    bool m_owner;

    DISABLE_COPYING(shared_var_t);
};

// Special serialization/deserialization rules
template <typename T> struct serializer_t<shared_var_t<T> > {
    static size_t size(const shared_var_t<T> &value) {
        return serializer_t<uuid_t>::size(value.m_registry->id()) +
               serializer_t<uint64_t>::size(value.m_index);
    }
    static int write(write_message_t *message, const shared_var_t<T> &value) {
        serializer_t<uuid_t>::write(message, value.m_registry->id());
        serializer_t<uint64_t>::write(message, value.m_index);
        return 0;
    }
    static shared_var_t<T> read(read_message_t *message) {
        DEBUG_VAR uuid_t id = serializer_t<uuid_t>::read(message);
        shared_registry_t *registry = thread_t::self()->get_shared_registry();
        assert(registry != nullptr);
        assert(registry->id() == id);
        return shared_var_t<T>(registry, serializer_t<uint64_t>::read(message));
    }
};

} // namespace indecorous

#endif // CROSS_THREAD_SHARED_HPP_
