#include "catch.hpp"

#include <unistd.h>

#include <thread>
#include <vector>

#include "containers/intrusive.hpp"
#include "coro/barrier.hpp"

using namespace indecorous;

class node_t : public intrusive_node_t<node_t> {
public:
    node_t() : m_value(s_next_value++) { }
    bool operator ==(const node_t &other) { return m_value == other.m_value; }
private:
    static uint64_t s_next_value;
    uint64_t m_value;
};

uint64_t node_t::s_next_value = 0;

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

TEST_CASE("mpsc_queue_t/single_thread", "[containers][intrusive]") {
    node_t nodes[5];

    mpsc_queue_t<node_t> queue;

    queue.push(&nodes[0]);
    REQUIRE(*queue.pop() == nodes[0]);

    queue.push(&nodes[0]);
    queue.push(&nodes[1]);
    queue.push(&nodes[2]);
    queue.push(&nodes[3]);
    REQUIRE(*queue.pop() == nodes[0]);
    REQUIRE(*queue.pop() == nodes[1]);
    REQUIRE(*queue.pop() == nodes[2]);
    REQUIRE(*queue.pop() == nodes[3]);
}

const size_t produce_count = 100000;
const size_t producer_count = 10;

void mpsc_queue_producer(mpsc_queue_t<node_t> *queue, thread_barrier_t *barrier) {
    barrier->wait();
    for (size_t i = 0; i < produce_count; ++i) {
        queue->push(new node_t());
    }
}

void mpsc_queue_consumer(mpsc_queue_t<node_t> *queue, thread_barrier_t *barrier) {
    barrier->wait();
    for (size_t i = 0; i < produce_count * producer_count; ++i) {
        node_t *node = nullptr;
        while (node == nullptr) {
            node = queue->pop();
            if (node == nullptr) { usleep(100); }
        }
        delete node;
    }
}

TEST_CASE("mpsc_queue_t/multi_thread", "[containers][intrusive]") {
    mpsc_queue_t<node_t> queue;
    thread_barrier_t barrier(producer_count + 1);

    std::vector<std::thread> threads;
    for (size_t i = 0; i < 5; ++i) {
        threads.emplace_back(mpsc_queue_producer, &queue, &barrier);
    }
    threads.emplace_back(mpsc_queue_consumer, &queue, &barrier);

    for (auto &&thread : threads) {
        thread.join();
    }
}
