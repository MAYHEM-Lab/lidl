#pragma once

#include <gsl/span>
#include <vector>
#include <cstdint>

namespace lidl {
struct ibinary_writer {
    template <class T>
    void write(const T& t) {
        write_raw({reinterpret_cast<const char*>(&t), sizeof t});
    }

    void write_raw_string(std::string_view str) {
        write_raw({str.data(), ssize_t(str.size())});
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

struct ibinary_reader {
    virtual void seek(int bytes, std::ios::seekdir dir) = 0;

    void align(int alignment) {
        while(tell() % alignment != 0) {
            seek(1);
        }
    }

    void seek(int bytes) {
        seek(bytes, std::ios::cur);
    }

    virtual int tell() = 0;

    template <class T>
    T read_object() {
        static_assert(std::is_trivially_copyable_v<T>);
        uint8_t data[sizeof(T)];
        read(data);
        T t;
        memcpy(&t, data, sizeof t);
        return t;
    }

    std::vector<uint8_t> read_bytes(int bytes) {
        std::vector<uint8_t> data(bytes);
        read(data);
        return data;
    }

    virtual gsl::span<uint8_t> read(gsl::span<uint8_t>) = 0;

    virtual ~ibinary_reader() = default;
};
}