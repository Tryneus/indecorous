#ifndef SYNC_SWAP_HPP_
#define SYNC_SWAP_HPP_

namespace indecorous {

class dispatcher_t;

class assert_no_swap_t {
public:
    assert_no_swap_t();
    ~assert_no_swap_t();
private:
    dispatcher_t *m_dispatch;
    bool m_old_swap_permitted;
};

} // namespace indecorous

#endif // SYNC_SWAP_HPP_
