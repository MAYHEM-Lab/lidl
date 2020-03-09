#pragma once
#include "generics.hpp"

namespace lidl {
struct array_type : generic {
    array_type()
        : generic(make_generic_declaration({{"T", "type"}, {"Size", "i32"}})) {
    }

    bool is_reference(const module& mod,
                      const generic_instantiation& instantiation) const override;

    virtual raw_layout
    wire_layout(const module& mod,
                const generic_instantiation& instantiation) const override {
        if (is_reference(mod, instantiation)) {
            return {2, 2};
        }
        auto& arg = std::get<name>(instantiation.arguments()[0]);
        if (auto regular = get_type(mod, arg); regular) {
            auto layout = regular->wire_layout(mod);
            auto len = std::get<int64_t>(instantiation.arguments()[1]);
            return raw_layout(static_cast<int16_t>(layout.size() * len),
                              layout.alignment());
        }
        throw std::runtime_error("Array type is not regular!");
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& mod,
                                           const generic_instantiation& instantiation,
                                           gsl::span<const uint8_t> span) const override {
        YAML::Node arr;
        auto& arg = std::get<name>(instantiation.arguments()[0]);
        if (auto pointee = get_type(mod, arg); pointee) {
            auto layout = pointee->wire_layout(mod);
            auto len = std::get<int64_t>(instantiation.arguments()[1]);

            for (int i = 0; i < len; ++i) {
                auto obj_span =
                    span.subspan(0, span.size() - (len - i - 1) * layout.size());
                auto [yaml, consumed] = pointee->bin2yaml(mod, obj_span);
                arr.push_back(std::move(yaml));
            }

            return {std::move(arr), layout.size() * len};
        }

        throw std::runtime_error("pointee must be a regular type");
    }

    int yaml2bin(const module& mod,
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        auto& len = std::get<int64_t>(instantiation.arguments()[1]);
        if (len != node.size()) {
            throw std::runtime_error("Array sizes do not match!");
        }

        writer.align(wire_layout(mod, instantiation).alignment());
        auto pos = writer.tell();
        auto& arg = std::get<name>(instantiation.arguments()[0]);
        if (auto pointee = get_type(mod, arg); pointee) {
            for (auto& elem : node) {
                pointee->yaml2bin(mod, elem, writer);
            }
        }
        return pos;
    }
};

struct basic_type : value_type {
    explicit basic_type(int bits)
        : size_in_bits(bits) {
    }

    virtual raw_layout wire_layout(const module&) const override {
        return raw_layout(static_cast<int16_t>(size_in_bytes()),
                          static_cast<int16_t>(size_in_bytes()));
    }

    size_t size_in_bytes() const {
        return static_cast<size_t>(std::ceil(size_in_bits / 8.f));
    }

    int32_t size_in_bits;
};

struct integral_type : basic_type {
    explicit integral_type(int bits, bool unsigned_)
        : basic_type(bits)
        , is_unsigned(unsigned_) {
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& mod,
                                           gsl::span<const uint8_t> span) const override {
        auto layout = wire_layout(mod);
        auto s = span.subspan(span.size() - layout.size(), layout.size());
        if (is_unsigned) {
            uint64_t x{0};
            memcpy(reinterpret_cast<char*>(&x), s.data(), s.size());
            return {YAML::Node(x), s.size()};
        } else {
            int64_t x{0};
            memcpy(reinterpret_cast<char*>(&x), s.data(), s.size());
            return {YAML::Node(x), s.size()};
        }
    }

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        if (is_unsigned) {
            auto data = node.as<uint64_t>();
            auto layout = wire_layout(mod);
            std::vector<char> buf(layout.size());
            memcpy(buf.data(), &data, layout.size());
            writer.align(layout.alignment());
            auto pos = writer.tell();
            writer.write_raw(buf);
            return pos;
        } else {
            auto data = node.as<int64_t>();
            auto layout = wire_layout(mod);
            std::vector<char> buf(layout.size());
            memcpy(buf.data(), &data, layout.size());
            writer.align(layout.alignment());
            auto pos = writer.tell();
            writer.write_raw(buf);
            return pos;
        }
    }

    bool is_unsigned;
};

struct float_type : basic_type {
    explicit float_type()
        : basic_type(32) {
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& mod,
                                           gsl::span<const uint8_t> span) const override {
        auto layout = wire_layout(mod);
        auto s = span.subspan(span.size() - layout.size(), layout.size());
        float x{0};
        memcpy(reinterpret_cast<char*>(&x), s.data(), s.size());
        return {YAML::Node(x), s.size()};
    }

    int yaml2bin(const module& module,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        writer.align(4);
        auto pos = writer.tell();
        writer.write(node.as<float>());
        return pos;
    }
};

struct double_type : basic_type {
    explicit double_type()
        : basic_type(64) {
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& mod,
                                           gsl::span<const uint8_t> span) const override {
        auto layout = wire_layout(mod);
        auto s = span.subspan(span.size() - layout.size(), layout.size());
        double x{0};
        memcpy(reinterpret_cast<char*>(&x), s.data(), s.size());
        return {YAML::Node(x), s.size()};
    }

    int yaml2bin(const module& module,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        writer.align(8);
        auto pos = writer.tell();
        writer.write(node.as<double>());
        return pos;
    }
};

struct string_type : reference_type {
    std::pair<YAML::Node, size_t> bin2yaml(const module&,
                                           gsl::span<const uint8_t> span) const override {
        auto ptr_span = span.subspan(span.size() - 2, 2);
        uint16_t off{0};
        memcpy(&off, ptr_span.data(), ptr_span.size());
        std::string s(off, 0);
        auto str_span = span.subspan(span.size() - 2 - off, off);
        memcpy(s.data(), str_span.data(), str_span.size());
        while (s.back() == 0) {
            s.pop_back();
        }
        return {YAML::Node(s), off + 2};
    }

    int yaml2bin(const module& module,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;
};

struct vector_type : generic {
    vector_type()
        : generic(make_generic_declaration({{"T", "type"}})) {
    }

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation&) const override {
        return raw_layout{2, 2};
    }

    bool is_reference(const module& mod,
                      const generic_instantiation& instantiation) const override {
        return true;
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& mod,
                                           const generic_instantiation& instantiation,
                                           gsl::span<const uint8_t> span) const override {
        auto ptr_span = span.subspan(span.size() - 2, 2);
        uint16_t off{0};
        memcpy(&off, ptr_span.data(), ptr_span.size());

        YAML::Node arr;
        if (off == 0) {
            return {arr, 2};
        }

        auto& arg = std::get<name>(instantiation.arguments()[0]);
        if (auto pointee = get_type(mod, arg); pointee) {
            auto layout = pointee->wire_layout(mod);
            auto len = off / layout.size();

            for (int i = 0; i < len; ++i) {
                auto obj_span =
                    span.subspan(0, span.size() - 2 - off + (i + 1) * layout.size());
                auto [yaml, consumed] = pointee->bin2yaml(mod, obj_span);
                arr.push_back(std::move(yaml));
            }

            return {std::move(arr), layout.size() * len};
        }

        throw std::runtime_error("pointee must be a regular type");
    }

    int yaml2bin(const module& mod,
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;
};
}
