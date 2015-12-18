#include "test.hpp"
#include "utils.hpp"

using namespace indecorous;

TEST_CASE("utils/strprintf", "[utils][strprintf]") {
    std::string s;

    s = strprintf("1, 2, 3, %d, 5, %d, 7", 4, 6);
    CHECK(s == "1, 2, 3, 4, 5, 6, 7");

    for(size_t i = 0; i < 2; ++i) {
        s = strprintf("The system is %sdown.", i ? "" : "not ");
        if (i) {
            CHECK(s == "The system is down.");
        } else {
            CHECK(s == "The system is not down.");
        }
    }
}

TEST_CASE("utils/string_error", "[utils][string_error]") {
    std::string s;

    s = string_error(0);
    CHECK(s == "Success");

    s = string_error(EPERM);
    CHECK(s == "Operation not permitted");

    s = string_error(ENOENT);
    CHECK(s == "No such file or directory");
}
