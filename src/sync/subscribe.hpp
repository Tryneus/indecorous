#ifndef SYNC_SUBSCRIBE_HPP_
#define SYNC_SUBSCRIBE_HPP_

#include <memory>

#include "containers/intrusive.hpp"
#include "drainer.hpp"
#include "sync/swap.hpp"

namespace indecorous {

template <typename T>
class subscribable_t;

class subscription_t {
public:
    subscription_t(subscription_t &&other);
    ~subscription_t();

private:
    class base_t {
    public:
        virtual ~base_t() { }
    };

    template <typename T>
    class typed_t : public base_t, public intrusive_node_t<typed_t<T> > {
    public:
        typed_t(subscribable_t<T> *parent);
        virtual ~typed_t();

        virtual void on_event(const T &value) const = 0;
    private:
        subscribable_t<T> *m_parent;
    };

    template <typename T, typename Callable>
    class final_typed_t final : public typed_t<T> {
    public:
        final_typed_t(subscribable_t<T> *parent, Callable &&cb) :
           typed_t<T>(parent), m_cb(std::forward<Callable>(cb)) { }
        void on_event(const T &value) const override final {
            m_cb(value);
        }
    private:
        const Callable m_cb;
    };

    template <typename T>
    friend class subscribable_t;
    template <typename T, typename Callable>
    subscription_t(subscribable_t<T> *parent, Callable &&cb);

    std::unique_ptr<base_t> m_base;

    DISABLE_COPYING(subscription_t);
};

template <typename T>
class subscribable_t {
public:
    subscribable_t() : m_subs(), m_drainer() { }
    
    template <typename Callable>
    subscription_t add(Callable &&cb);

    void notify(const T &value) const;
private:
    intrusive_list_t<subscription_t::typed_t<T> > m_subs;
    drainer_t m_drainer;

    DISABLE_COPYING(subscribable_t);
};

template <typename T, typename Callable>
subscription_t::subscription_t(subscribable_t<T> *parent, Callable &&cb) :
        m_base(std::make_unique<final_typed_t<T, Callable> >(parent, std::forward<Callable>(cb))) { }

subscription_t::subscription_t(subscription_t &&other) :
        m_base(std::move(other.m_base)) { }

template <typename T>
subscription_t::typed_t<T>::typed_t(subscribable_t<T> *parent) :
        m_parent(parent) {
    m_parent->m_subs.push_back(this);
}

template <typename T>
subscription_t::typed_t<T>::~typed_t() {
    m_parent->m_subs.remove(this);
}

template <typename T>
template <typename Callable>
subscription_t subscribable_t<T>::add(Callable &&cb) {
    return subscription_t(this, std::forward<Callable>(cb));
}

template <typename T>
void subscribable_t<T>::notify(const T &value) const {
    assert_no_swap_t no_swap;
    m_subs.each([&] (auto const &s) {
        s->on_event(value);
    });
}

} // namespace indecorous

#endif // SYNC_SUBSCRIBE_HPP_
