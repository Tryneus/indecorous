#include <map>
#include <set>
#include <vector>

#include "handler.hpp"
#include "hub.hpp"
#include "message.hpp"
#include "target.hpp"
#include "serialize.hpp"
#include "serialize_stl.hpp"

class data_t {
public:
    data_t() { }
    data_t(data_t &&other) = default;

    uint64_t x;
    std::vector<uint64_t> y;
    std::map<std::set<uint64_t>, std::string> z;

private:
    data_t(const data_t &) = delete;
    data_t &operator = (const data_t &) = delete;
    SERIALIZABLE(data_t, x, y, z);
};

class read_callback_t {
public:
    static int call(std::string s, bool flag, data_t d) {
        printf("called with %s, %s, %zu\n",
               s.c_str(), flag ? "true" : "false", d.y.size());
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
