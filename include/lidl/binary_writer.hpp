#pragma once

#include <gsl/span>

namespace lidl {
struct ibinary_writer {
    template <class T>
    void write(const T& t) {
        write_raw({reinterpret_cast<const char*>(&t), sizeof t});
    }

    void align(int alignment) {
        while((tell() % alignment) != 0) {
            char c = 0;
            write(c);
        }
    }

    virtual int tell() = 0;
    virtual void write_raw(gsl::span<const char> data) = 0;
    virtual ~ibinary_writer() = default;
};

}