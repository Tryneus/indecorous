#include "test.hpp"

#include "io/file.hpp"
#include "sync/coro_map.hpp"

using namespace indecorous;

const char file_data[] = {'l','o','r','e','m',' ','i','p','s','u','m'};
const size_t buffer_size = sizeof(file_data);

SIMPLE_TEST(file, simple, 1, "[io][file]") {
    char buffer[buffer_size];
    file_t file("./file_test.txt", O_RDWR | O_CREAT | O_TRUNC);

    file.read(0, buffer, buffer_size).then([] (int res) {
            debugf("read response");
            CHECK(res == EINVAL);
        }).wait();
    file.write(0, file_data, buffer_size).then([] (int res) {
            debugf("write response");
            CHECK(res == 0);
        }).wait();
    file.read(0, buffer, buffer_size).then([&] (int res) {
            debugf("read response");
            CHECK(res == 0);
            CHECK(memcmp(file_data, buffer, buffer_size) == 0);
        }).wait();
}

SIMPLE_TEST(file, read_write, 1, "[io][file]") {
    char buffer[buffer_size];
    file_t reader("./file_test.txt", O_RDONLY | O_CREAT | O_TRUNC);
    file_t writer("./file_test.txt", O_WRONLY);
    debugf("test buffer: %p", &buffer[0]);

    reader.write(0, file_data, buffer_size).then([] (int res) {
            CHECK(res == EBADF);
        }).wait();

    writer.read(0, buffer, buffer_size).then([] (int res) {
            CHECK(res == EBADF);
        }).wait();

    coro_map(buffer_size, [&] (size_t i) {
            writer.write(i, &file_data[i], 1).then([&] (int write_res) {
                    CHECK(write_res == 0);
                }).then([&] () {
                    return reader.read(i, &buffer[i], 1);
                }).then([&] (int read_res) {
                    CHECK(read_res == 0);
                    CHECK(buffer[i] == file_data[i]);
                }).wait();
        });

    CHECK(memcmp(file_data, buffer, buffer_size) == 0);
}

