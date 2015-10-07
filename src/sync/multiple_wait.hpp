#ifndef SYNC_MULTIPLE_WAIT_HPP_
#define SYNC_MULTIPLE_WAIT_HPP_

#include <cstddef>
#include <type_traits>
#include <vector>

#include "common.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

class coro_t;
class interruptor_t;

// Be careful to avoid deadlock when using wait_all with semaphores

class multiple_waiter_t final : private wait_callback_t {
public:
    enum class wait_type_t { ANY, ALL };
    multiple_waiter_t(wait_type_t type, size_t total);
    ~multiple_waiter_t();

    void wait();
    void item_finished(wait_result_t result);

private:
    void wait_done(wait_result_t result) override final;
    void object_moved(waitable_t *new_ptr) override final;

    void ready(wait_result_t result);

    coro_t *m_owner_coro;
    waitable_t *m_interruptor;

    bool m_ready;
    bool m_waiting;
    size_t m_needed;
    wait_result_t m_error_result;

    DISABLE_COPYING(multiple_waiter_t);
};

class multiple_wait_callback_t final : private wait_callback_t {
public:
    multiple_wait_callback_t(waitable_t *obj,
                             multiple_waiter_t *waiter);
    multiple_wait_callback_t(waitable_t &obj,
                             multiple_waiter_t *waiter);
    multiple_wait_callback_t(multiple_wait_callback_t &&other) = default;
    ~multiple_wait_callback_t();

private:
    void wait_done(wait_result_t result) override final;
    void object_moved(waitable_t *new_ptr) override final;

    waitable_t *m_obj;
    multiple_waiter_t *m_waiter;

    DISABLE_COPYING(multiple_wait_callback_t);
};

template <multiple_waiter_t::wait_type_t Type, typename Container>
void wait_generic_container(Container &c) {
    multiple_waiter_t waiter(Type, c.size());
    std::vector<multiple_wait_callback_t> waits;
    waits.reserve(c.size());
    for (auto &&item : c) {
        waits.emplace_back(item, &waiter);
    }
    waiter.wait();
}

template <typename Container>
typename std::enable_if<!std::is_base_of<waitable_t, Container>::value, void>::type
wait_any(Container &c) {
    wait_generic_container<multiple_waiter_t::wait_type_t::ANY>(c);
}

template <typename Container>
typename std::enable_if<!std::is_base_of<waitable_t, Container>::value, void>::type
wait_all(Container &c) {
    wait_generic_container<multiple_waiter_t::wait_type_t::ALL>(c);
}

template <multiple_waiter_t::wait_type_t Type, typename... Args>
void wait_generic(Args &&...args) {
    multiple_waiter_t waiter(Type, sizeof...(Args));
    UNUSED multiple_wait_callback_t waits[] =
        { multiple_wait_callback_t(std::forward<Args>(args), &waiter)... };
    waiter.wait();
}

template <bool val>
constexpr bool variadic_and() {
    return val;
}

template <bool val, bool val2, bool... Args>
constexpr bool variadic_and() {
    return val && variadic_and<val2, Args...>();
}

template <typename... Args>
typename std::enable_if<!variadic_and<std::is_base_of<waitable_t, Args>::value...>(), void>::type
wait_any(Args &&...args) {
    wait_generic<multiple_waiter_t::wait_type_t::ANY>(std::forward<Args>(args)...);
}

template <typename... Args>
typename std::enable_if<!variadic_and<std::is_base_of<waitable_t, Args>::value...>(), void>::type
wait_all(Args &&...args) {
    wait_generic<multiple_waiter_t::wait_type_t::ALL>(std::forward<Args>(args)...);
}

} // namespace indecorous

#endif // SYNC_MULTIPLE_WAIT_HPP_
