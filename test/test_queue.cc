#include "catch.hpp"

#include "containers/intrusive.hpp"
#include "random.hpp"

using namespace indecorous;

class node_t : public intrusive_node_t<node_t> {
public:
    node_t() : m_value(s_rng.generate<uint64_t>()) { }
    bool operator ==(const node_t &other) { return m_value == other.m_value; }
private:
    static pseudo_random_t s_rng;
    uint64_t m_value;
};

pseudo_random_t node_t::s_rng;

TEST_CASE("intrusive_list_t", "[containers][intrusive]") {
    node_t nodes[5];
    intrusive_list_t<node_t> list;

    REQUIRE(list.pop_front() == nullptr);
    REQUIRE(list.pop_back() == nullptr);

    list.push_back(&nodes[0]);
    list.push_back(&nodes[1]);
    list.push_back(&nodes[2]);
    REQUIRE(*list.pop_back() == nodes[2]);
    REQUIRE(*list.pop_front() == nodes[0]);
    REQUIRE(*list.pop_front() == nodes[1]);
    REQUIRE(list.pop_front() == nullptr);
    REQUIRE(list.pop_back() == nullptr);
}

TEST_CASE("mpsc_queue_t", "[containers][intrusive]") {
    node_t nodes[5];

}
