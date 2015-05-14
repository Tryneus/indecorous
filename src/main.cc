#include <map>
#include <set>
#include <vector>

#include "handler.hpp"
#include "hub.hpp"
#include "message.hpp"
#include "target.hpp"
#include "serialize.hpp"
#include "serialize_stl.hpp"

class non_copyable_t {
private:
    non_copyable_t(non_copyable_t &&other) : val(other.val), x(std::move(other.x)) { }
    non_copyable_t(std::string _x, uint64_t _val) : val(_val), x(_x) { }
    non_copyable_t(const non_copyable_t &) = delete;
    non_copyable_t &operator = (const non_copyable_t &) = delete;

    uint64_t val;
    std::string x;

    MAKE_SERIALIZABLE(non_copyable_t, val, x);
};

class data_t {
public:
    data_t() { }
    data_t(data_t &&other) = default;

    uint64_t x;
    //uint64_t y;
    std::list<non_copyable_t> y;
    //std::map<non_copyable_t, non_copyable_t> y;
    uint64_t z;

private:
    data_t(const data_t &) = delete;
    data_t &operator = (const data_t &) = delete;
    MAKE_SERIALIZABLE(data_t, x, y, z);
};

class wrapped_data_t {
    std::map<std::set<uint64_t>, std::string> d;
    MAKE_SERIALIZABLE(wrapped_data_t, d);
};

class read_callback_t {
public:
    static int call(std::string, bool, data_t) {
        return -1;
    }
};

template<>
const handler_id_t unique_handler_t<read_callback_t>::unique_id = handler_id_t::assign();

int main() {
    message_hub_t hub;
    local_target_t target(&hub);
    handler_t<read_callback_t> read_handler(&hub);

    data_t d;
    target.noreply_call<read_callback_t>(std::string("stuff"), true, std::move(d));

    read_message_t message = read_message_t::parse(&target.stream);
    read_handler.internal_handler.handle(&message);

    return 0;
}
