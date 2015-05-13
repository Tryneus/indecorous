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

    std::vector<uint64_t> y;

    size_t serialized_size() const {
        return full_serialized_size(y);
    }

    int serialize(write_message_t *msg) &&{
        return full_serialize(msg, std::move(y));
    }

    data_t(decltype(y) &&_y) :
         y(std::move(_y)) { }

    static data_t deserialize(read_message_t *msg) {
        return data_t(deserializer_t<decltype(y)>::run(msg));
    }

//    uint64_t x;
//    std::vector<uint64_t> y;
//    std::map<std::set<uint64_t>, std::string> z;

//    size_t serialized_size() const {
//        return full_serialized_size(x, y, z);
//    }
//
//    int serialize(write_message_t *msg) && {
//        return full_serialize(msg, x, y, z);
//    }
//
//    data_t(decltype(x) &&_x, decltype(y) &&_y, decltype(z) &&_z) :
//        x(std::move(_x)), y(std::move(_y)), z(std::move(_z)) { }
//
//    static data_t deserialize(read_message_t *msg) {
//        return data_t(deserialize<decltype(x)>(msg),
//                      deserialize<decltype(y)>(msg),
//                      deserialize<decltype(z)>(msg));
//    }
private:
    data_t(const data_t &) = delete;
    data_t &operator = (const data_t &) = delete;
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
