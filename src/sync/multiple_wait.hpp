#ifndef SYNC_MULTIPLE_WAIT_HPP_
#define SYNC_MULTIPLE_WAIT_HPP_

#include "sync/wait_object.hpp"

namespace indecorous {

class coro_t;

class multiple_waiter_t : private wait_callback_t {
public:
    enum class wait_type_t { ANY, ALL };
    multiple_waiter_t(wait_type_t type, size_t total, wait_object_t *interruptor);
    ~multiple_waiter_t();

    void wait();
    void item_finished(wait_result_t result);

private:

    void ready(wait_result_t result);
    void wait_done(wait_result_t result);

    coro_t *m_owner_coro;
    wait_object_t *m_interruptor;

    bool m_ready;
    bool m_waiting;
    size_t m_needed;
    wait_result_t m_error_result;
};

class multiple_wait_callback_t : private wait_callback_t {
public:
    multiple_wait_callback_t(wait_object_t *obj,
                             multiple_waiter_t *waiter);
    multiple_wait_callback_t(wait_object_t &obj,
                             multiple_waiter_t *waiter);
    multiple_wait_callback_t(multiple_wait_callback_t &&other) = default;
    ~multiple_wait_callback_t();

private:
    void wait_done(wait_result_t result);

    wait_object_t *m_obj;
    multiple_waiter_t *m_waiter;
};

template <multiple_waiter_t::wait_type_t Type, typename... Args>
void wait_generic(Args &&...args, wait_object_t *interruptor) {
    multiple_waiter_t waiter(Type, sizeof...(Args), interruptor);
    multiple_wait_callback_t waits[] = { multiple_wait_callback_t(std::forward<Args>(args), &waiter)... };
    waiter.wait();
}

template <typename... Args>
void wait_any(Args &&...args) {
    wait_generic<multiple_waiter_t::wait_type_t::ANY>(std::forward<Args>(args)..., nullptr);
}

template <typename... Args>
void wait_any_interruptible(Args &&...args, wait_object_t *interruptor) {
    wait_generic<multiple_waiter_t::wait_type_t::ANY>(std::forward<Args>(args)..., interruptor);
}

template <typename... Args>
void wait_all(Args &&...args) {
    wait_generic<multiple_waiter_t::wait_type_t::ALL>(std::forward<Args>(args)..., nullptr);
}

template <typename... Args>
void wait_all_interruptible(Args &&...args, wait_object_t *interruptor) {
    wait_generic<multiple_waiter_t::wait_type_t::ALL>(std::forward<Args>(args)..., interruptor);
}

template <multiple_waiter_t::wait_type_t Type, typename It>
void wait_generic_it(const It &begin, const It &end, wait_object_t *interruptor) {
    multiple_waiter_t waiter(Type, end - begin, nullptr);
    std::vector<multiple_wait_callback_t> waits;
    waits.reserve(end - begin);
    for (const It it = begin; it != end; ++it) {
        waits.emplace_back(*it, &waiter);
    }
    waiter.wait();
}

template <typename It>
void wait_any(const It &begin, const It &end) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ANY>(begin, end, nullptr);
}

template <typename It>
void wait_all(const It &begin, const It &end) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ALL>(begin, end, nullptr);
}

} // namespace indecorous

#endif // SYNC_MULTIPLE_WAIT_HPP_
