#include "random.hpp"

#include <string.h>

namespace indecorous {

random_t::~random_t() { }

template <typename gen_t>
void random_t::fill_internal(void *buffer, size_t length, gen_t *generator) {
    typedef typename gen_t::val_t val_t;
    union wrapped_t { val_t t; char c[sizeof(val_t)]; } w;
    for (size_t i = 0; i < (length / sizeof(val_t)); i += sizeof(val_t)) {
        w.t = generator->next();
        memcpy(&static_cast<char *>(buffer)[i], &w.c[0], sizeof(val_t));
    }

    size_t remainder = length % sizeof(val_t);
    if (remainder > 0) {
        w.t = generator->next();
        memcpy(&static_cast<char *>(buffer)[length - remainder], &w.c[0], remainder);
    }
}

true_random_t::true_random_t() { }

void true_random_t::fill(void *buffer, size_t length) {
    fill_internal(buffer, length, this);
}

true_random_t::val_t true_random_t::next() {
    return dev();
}

pseudo_random_t::pseudo_random_t() {
    true_random_t r;
    dev.seed(r.generate<val_t>());
}

void pseudo_random_t::fill(void *buffer, size_t length) {
    fill_internal(buffer, length, this);
}

pseudo_random_t::val_t pseudo_random_t::next() {
    return dev();
}

uuid_t::uuid_t() { }

uuid_t uuid_t::generate(random_t *r) {
    uuid_t new_uuid;
    r->fill(&new_uuid.buffer[0], sizeof(new_uuid.buffer));
    // TODO: standards compliance for v4 UUIDs
    return new_uuid;
}

uuid_t uuid_t::nil() {
    uuid_t new_uuid;
    memset(&new_uuid.buffer[0], 0, sizeof(new_uuid.buffer));
    return new_uuid;
}

// Perform a timing-sensitive comparison so if someone uses an ID as a security
// measure, it won't be susceptible to timing attacks.
// TODO: make sure the compiler isn't optimizing this - check libsodium sodium_memcmp
bool uuid_t::operator ==(const uuid_t &other) const {
    bool res = true;
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        res = res && (buffer[i] == other.buffer[i]);
    }
    return res;
}

} // namespace indecorous
