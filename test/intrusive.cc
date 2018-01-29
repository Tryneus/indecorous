#include "test.hpp"
#include "utils.hpp"

using namespace indecorous;

struct test_object_t : public intrusive_node_t<test_object_t> {
    test_object_t() = default;
    test_object_t(test_object_t &&) = default;
    ~test_object_t() = default;
};

TEST_CASE("intrusive/push_back", "[container][intrusive]") {
    intrusive_list_t<test_object_t> list;
    CHECK(list.size() == 0);

    test_object_t a;
    CHECK(!a.in_a_list());
    list.push_back(&a);
    CHECK(list.size() == 1);
    CHECK(a.in_a_list());

    test_object_t b;
    CHECK(!b.in_a_list());
    list.push_back(&b);
    CHECK(list.size() == 2);
    CHECK(b.in_a_list());

    test_object_t c;
    CHECK(!c.in_a_list());
    list.push_back(&c);
    CHECK(list.size() == 3);
    CHECK(c.in_a_list());

    list.remove(&b);
    CHECK(list.size() == 2);
    CHECK(!b.in_a_list());

    list.push_back(&b);
    CHECK(list.size() == 3);
    CHECK(b.in_a_list());
    
    list.clear([] (test_object_t *) {});
    CHECK(list.size() == 0);
    CHECK(!a.in_a_list());
    CHECK(!b.in_a_list());
    CHECK(!c.in_a_list());
}

TEST_CASE("intrusive/move-1", "[container][intrusive][]") {
    test_object_t object;
    intrusive_list_t<test_object_t> list;
    intrusive_list_t<test_object_t> other;

    CHECK(list.size() == 0);
    CHECK(other.size() == 0);
    CHECK(!object.in_a_list());

    list.push_back(&object);
    CHECK(list.size() == 1);
    CHECK(other.size() == 0);
    CHECK(object.in_a_list());

    list.swap(&other);
    CHECK(list.size() == 0);
    CHECK(other.size() == 1);
    CHECK(object.in_a_list());

    list.swap(&other);
    CHECK(list.size() == 1);
    CHECK(other.size() == 0);
    CHECK(object.in_a_list());

    list.remove(&object);
    CHECK(list.size() == 0);
    CHECK(other.size() == 0);
    CHECK(!object.in_a_list());
}

TEST_CASE("intrusive/move-2", "[container][intrusive][]") {
    test_object_t object;
    intrusive_list_t<test_object_t> list;

    CHECK(list.size() == 0);
    CHECK(!object.in_a_list());

    list.push_back(&object);
    CHECK(list.size() == 1);
    CHECK(object.in_a_list());

    {
        intrusive_list_t<test_object_t> other(std::move(list));
        CHECK(list.size() == 0);
        CHECK(other.size() == 1);
        CHECK(object.in_a_list());

        other.remove(&object);
        CHECK(list.size() == 0);
        CHECK(other.size() == 0);
        CHECK(!object.in_a_list());
    }

    CHECK(list.size() == 0);
    CHECK(!object.in_a_list());
}

TEST_CASE("intrusive/move-3", "[container][intrusive][]") {
    test_object_t object;
    intrusive_list_t<test_object_t> list;

    CHECK(list.size() == 0);
    CHECK(!object.in_a_list());

    list.push_back(&object);
    CHECK(list.size() == 1);
    CHECK(object.in_a_list());

    {
        intrusive_list_t<test_object_t> other(std::move(list));
        CHECK(list.size() == 0);
        CHECK(other.size() == 1);
        CHECK(object.in_a_list());

        list.swap(&other);
        CHECK(list.size() == 1);
        CHECK(other.size() == 0);
        CHECK(object.in_a_list());
    }

    list.remove(&object);
    CHECK(list.size() == 0);
    CHECK(!object.in_a_list());
}

TEST_CASE("intrusive/move-4", "[container][intrusive][]") {
    test_object_t object;
    intrusive_list_t<test_object_t> list;
    intrusive_list_t<test_object_t> other;

    CHECK(list.size() == 0);
    CHECK(other.size() == 0);
    CHECK(!object.in_a_list());

    list.push_back(&object);
    CHECK(list.size() == 1);
    CHECK(other.size() == 0);
    CHECK(object.in_a_list());

    list.swap(&other);
    CHECK(list.size() == 0);
    CHECK(other.size() == 1);
    CHECK(object.in_a_list());

    other.remove(&object);
    CHECK(list.size() == 0);
    CHECK(other.size() == 0);
    CHECK(!object.in_a_list());
}
