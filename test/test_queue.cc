#include "catch.hpp"

#include <unistd.h>

#include <thread>
#include <vector>

#include "containers/intrusive.hpp"
#include "coro/barrier.hpp"
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
const size_t producer_count = 5;

void mpsc_queue_producer(mpsc_queue_t<node_t> *queue, thread_barrier_t *barrier) {
    printf("producer started\n");
    barrier->wait();
    for (size_t i = 0; i < produce_count; ++i) {
        queue->push(new node_t());
    }
    printf("producer exiting\n");
}

void mpsc_queue_consumer(mpsc_queue_t<node_t> *queue, thread_barrier_t *barrier) {
    printf("consumer started\n");
    barrier->wait();
    for (size_t i = 0; i < produce_count * producer_count; ++i) {
        node_t *node = nullptr;
        while (node == nullptr) {
            node = queue->pop();
            if (node == nullptr) { usleep(10000); }
        }
        delete node;
        if (i % produce_count == 0) printf("consumed %zu items\n", i);
    }
    printf("consumer exiting\n");
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
