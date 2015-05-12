#include "handler.hpp"
#include "hub.hpp"
#include "message.hpp"
#include "target.hpp"
#include "serialize_stl.hpp"

class movable_object_t {
public:
    movable_object_t(movable_object_t &&other) : data(std::move(other.data)) {
        total_moves += 1;
    }
    movable_object_t &operator = (movable_object_t &&other) {
        total_moves += 1;
        data = std::move(other.data);
        return *this;
    }
    movable_object_t(int _data) : data(_data) { }

    DECLARE_SERIALIZABLE(movable_object_t);

    static size_t total_moves;
    int data;

private:
    movable_object_t(const movable_object_t &) = delete;
    movable_object_t &operator = (const movable_object_t &) = delete;
};

size_t movable_object_t::total_moves = 0;

size_t movable_object_t::serialized_size() const {
    return ::serialized_size<uint64_t>(data);
}

int movable_object_t::serialize(write_message_t *msg) {
    return ::serialize<uint64_t>(msg, data);
}

movable_object_t movable_object_t::deserialize(read_message_t *msg) {
    return movable_object_t(::deserialize<uint64_t>(msg));
}

class read_handler_t : public handler_t<read_handler_t, int, std::string, bool, movable_object_t> {
public:
    read_handler_t(message_hub_t *hub) : handler_t(hub) { }

    static
    int call(std::string s, bool flag, movable_object_t item) {
        printf("called with %s, %s, %d\n", s.c_str(), flag ? "true" : "false", item.data);
        return -1;
    }
};


template<>
const handler_id_t unique_handler_t<read_handler_t>::unique_id = handler_id_t::assign();

int main() {
    message_hub_t hub;
    local_target_t target(&hub);
    read_handler_t read_handler(&hub);

    target.noreply_call<read_handler_t>(std::string("stuff"), true, movable_object_t(5));

    read_message_t message = read_message_t::parse(&target.stream);
    read_handler.handle(&message);

    printf("Total moves of movable_object_t: %zu\n", movable_object_t::total_moves);

    return 0;
}
