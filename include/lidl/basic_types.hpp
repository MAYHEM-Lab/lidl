#pragma once
#include "generics.hpp"

namespace lidl {
struct array_type : generic_wire_type {
    array_type(module& mod);

    type_categories category(const module& mod, const name& instantiation) const override;

    virtual raw_layout wire_layout(const module& mod,
                                   const name& instantiation) const override {
        if (category(mod, instantiation) == type_categories::reference) {
            return {2, 2};
        }
        auto& arg = std::get<name>(instantiation.args[0]);
        if (auto regular = get_wire_type(mod, arg); regular) {
            auto layout = regular->wire_layout(mod);
            auto len    = std::get<int64_t>(instantiation.args[1]);
            return raw_layout(static_cast<int16_t>(layout.size() * len),
                              layout.alignment());
        }
        throw std::runtime_error("Array type is not regular!");
    }

    YAML::Node bin2yaml(const module& mod,
                        const name& instantiation,
                        ibinary_reader& span) const override {
        throw std::runtime_error("Not implemented!");
        //
        //        YAML::Node arr;
        //        auto& arg = std::get<name>(instantiation.arguments()[0]);
        //        if (auto pointee = get_type(mod, arg); pointee) {
        //            auto layout = pointee->wire_layout(mod);
        //            auto len    = std::get<int64_t>(instantiation.arguments()[1]);
        //
        //            for (int i = 0; i < len; ++i) {
        //                auto obj_span =
        //                    span.subspan(0, span.size() - (len - i - 1) *
        //                    layout.size());
        //                auto [yaml, consumed] = pointee->bin2yaml(mod, obj_span);
        //                arr.push_back(std::move(yaml));
        //            }
        //
        //            return {std::move(arr), layout.size() * len};
        //        }
        //
        //        throw std::runtime_error("pointee must be a regular type");
    }

    int yaml2bin(const module& mod,
                 const name& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        auto& len = std::get<int64_t>(instantiation.args[1]);
        if (len != node.size()) {
            throw std::runtime_error("Array sizes do not match!");
        }

        writer.align(wire_layout(mod, instantiation).alignment());
        auto pos  = writer.tell();
        auto& arg = std::get<name>(instantiation.args[0]);
        if (auto pointee = get_wire_type(mod, arg); pointee) {
            for (auto& elem : node) {
                pointee->yaml2bin(mod, elem, writer);
            }
        }
        return pos;
    }

    name get_wire_type_name_impl(const module& mod, const name& your_name) const override;
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

struct bool_type : basic_type {
    bool_type()
        : basic_type(1) {
    }

    YAML::Node bin2yaml(const module& module, ibinary_reader& reader) const override;
    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;
};

struct integral_type : basic_type {
    explicit integral_type(int bits, bool unsigned_)
        : basic_type(bits)
        , is_unsigned(unsigned_) {
    }

    YAML::Node bin2yaml(const module& mod, ibinary_reader& reader) const override {
        auto layout = wire_layout(mod);
        auto data   = reader.read_bytes(layout.size());
        if (is_unsigned) {
            uint64_t x{0};
            memcpy(reinterpret_cast<char*>(&x), data.data(), data.size());
            return YAML::Node(x);
        } else {
            int64_t x{0};
            memcpy(reinterpret_cast<char*>(&x), data.data(), data.size());
            return YAML::Node(x);
        }
    }

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        if (is_unsigned) {
            auto data   = node.as<uint64_t>();
            auto layout = wire_layout(mod);
            std::vector<char> buf(layout.size());
            memcpy(buf.data(), &data, layout.size());
            writer.align(layout.alignment());
            auto pos = writer.tell();
            writer.write_raw(buf);
            return pos;
        } else {
            auto data   = node.as<int64_t>();
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

    YAML::Node bin2yaml(const module& mod, ibinary_reader& reader) const override {
        return YAML::Node(reader.read_object<float>());
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

    YAML::Node bin2yaml(const module& mod, ibinary_reader& reader) const override {
        return YAML::Node(reader.read_object<double>());
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
    YAML::Node bin2yaml(const module&, ibinary_reader& span) const override;

    int yaml2bin(const module& module,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;
};

struct vector_type : generic_reference_type {
    vector_type(module& mod);

    YAML::Node bin2yaml(const module& mod,
                        const name& instantiation,
                        ibinary_reader& span) const override;

    int yaml2bin(const module& mod,
                 const name& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;

    name get_wire_type_name_impl(const module& mod, const name& your_name) const override;
};
} // namespace lidl
