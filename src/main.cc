#include "include/rpc.hpp"

const handler_id_t read_handler_t::id = handler_id_t::assign();

int main() {
    remote_target_t target;

    target.noreply_call<read_handler_t>(std::string("stuff"), true);

    read_handler_t::handle(read_message_t::parse(&target.stream));

    return 0;
}
