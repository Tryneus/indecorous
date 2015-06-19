#ifndef SYNC_MULTIPLE_WAIT_HPP_
#define SYNC_MULTIPLE_WAIT_HPP_

#include <vector>

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
    void object_moved(wait_object_t *new_ptr);

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
    void object_moved(wait_object_t *new_ptr);

    wait_object_t *m_obj;
    multiple_waiter_t *m_waiter;
};

// TODO: consider making interruptors a coro-level RAII type? - probably not worthwhile

template <multiple_waiter_t::wait_type_t Type, typename... Args>
void wait_generic(wait_object_t *interruptor, Args &&...args) {
    multiple_waiter_t waiter(Type, sizeof...(Args), interruptor);
    __attribute__((unused)) multiple_wait_callback_t waits[] =
        { multiple_wait_callback_t(std::forward<Args>(args), &waiter)... };
    waiter.wait();
}

template <typename... Args>
void wait_any(Args &&...args) {
    wait_generic<multiple_waiter_t::wait_type_t::ANY>(nullptr, std::forward<Args>(args)...);
}

template <typename... Args>
void wait_any_interruptible(wait_object_t *interruptor, Args &&...args) {
    wait_generic<multiple_waiter_t::wait_type_t::ANY>(interruptor, std::forward<Args>(args)...);
}

template <typename... Args>
void wait_all(Args &&...args) {
    wait_generic<multiple_waiter_t::wait_type_t::ALL>(nullptr, std::forward<Args>(args)...);
}

template <typename... Args>
void wait_all_interruptible(wait_object_t *interruptor, Args &&...args) {
    wait_generic<multiple_waiter_t::wait_type_t::ALL>(interruptor, std::forward<Args>(args)...);
}

template <multiple_waiter_t::wait_type_t Type, typename It>
void wait_generic_it(const It &begin, const It &end, wait_object_t *interruptor) {
    multiple_waiter_t waiter(Type, end - begin, interruptor);
    std::vector<multiple_wait_callback_t> waits;
    waits.reserve(end - begin + 1);
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
void wait_any_interruptible(wait_object_t *interruptor, const It &begin, const It &end) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ANY>(begin, end, interruptor);
}

template <typename It>
void wait_all(const It &begin, const It &end) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ALL>(begin, end, nullptr);
}

template <typename It>
void wait_all_interruptible(wait_object_t *interruptor, const It &begin, const It &end) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ALL>(begin, end, interruptor);
}

} // namespace indecorous

#endif // SYNC_MULTIPLE_WAIT_HPP_
