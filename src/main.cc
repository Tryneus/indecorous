#include "handler.hpp"
#include "hub.hpp"
#include "message.hpp"
#include "target.hpp"
#include "serialize_stl.hpp"

class read_callback_t {
public:
    static int call(std::string s, bool flag) {
        printf("called with %s, %s\n", s.c_str(), flag ? "true" : "false");
        return -1;
    }
};

class read_handler_t : public handler_t<read_callback_t> {
public:
    read_handler_t(message_hub_t *hub) : handler_t(hub) { }
};

template<>
const handler_id_t unique_handler_t<read_callback_t>::unique_id = handler_id_t::assign();

int main() {
    message_hub_t hub;
    local_target_t target(&hub);
    read_handler_t read_handler(&hub);

    target.noreply_call<read_handler_t>(std::string("stuff"), true);

    read_message_t message = read_message_t::parse(&target.stream);
    read_handler.internal_handler.handle(&message);

    return 0;
}
