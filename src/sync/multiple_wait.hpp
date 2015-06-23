#ifndef SYNC_MULTIPLE_WAIT_HPP_
#define SYNC_MULTIPLE_WAIT_HPP_

#include <vector>

#include "sync/wait_object.hpp"

namespace indecorous {

class coro_t;

// Be careful to avoid deadlock when using wait_all with semaphores

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

template <multiple_waiter_t::wait_type_t Type, typename Container>
void wait_generic_it(Container &c, wait_object_t *interruptor) {
    multiple_waiter_t waiter(Type, c.size(), interruptor);
    std::vector<multiple_wait_callback_t> waits;
    waits.reserve(c.size() + 1);
    for (auto &&item : c) {
        waits.emplace_back(item, &waiter);
    }
    waiter.wait();
}

template <typename Container>
void wait_any_it(Container &c) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ANY>(c, nullptr);
}

template <typename Container>
void wait_any_it_interruptible(wait_object_t *interruptor, Container &c) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ANY>(c, interruptor);
}

template <typename Container>
void wait_all_it(Container &c) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ALL>(c, nullptr);
}

template <typename Container>
void wait_all_it_interruptible(wait_object_t *interruptor, Container &c) {
    wait_generic_it<multiple_waiter_t::wait_type_t::ALL>(c, interruptor);
}

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

} // namespace indecorous

#endif // SYNC_MULTIPLE_WAIT_HPP_
