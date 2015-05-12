#include "include/rpc.hpp"

class read_handler_t : public base_handler_t<read_handler_t, int, std::string, bool> {
public:
    static const handler_id_t id;
    static // TODO: make this not static
    int call(std::string s, bool flag) {
        printf("called with %s, %s\n", s.c_str(), flag ? "true" : "false");
        return -1;
    }
};

const handler_id_t read_handler_t::id = handler_id_t::assign();

int main() {
    remote_target_t target;

    target.noreply_call<read_handler_t>(std::string("stuff"), true);

    read_message_t message = read_message_t::parse(&target.stream);
    read_handler_t::handle(&message);

    return 0;
}
