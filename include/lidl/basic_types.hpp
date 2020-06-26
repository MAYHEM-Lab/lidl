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
            auto len    = std::get<int64_t>(instantiation.arguments()[1]);
            return raw_layout(static_cast<int16_t>(layout.size() * len),
                              layout.alignment());
        }
        throw std::runtime_error("Array type is not regular!");
    }

    YAML::Node bin2yaml(const module& mod,
                        const generic_instantiation& instantiation,
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
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        auto& len = std::get<int64_t>(instantiation.arguments()[1]);
        if (len != node.size()) {
            throw std::runtime_error("Array sizes do not match!");
        }

        writer.align(wire_layout(mod, instantiation).alignment());
        auto pos  = writer.tell();
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

struct string_type : type {
    YAML::Node bin2yaml(const module&, ibinary_reader& span) const override;

    raw_layout wire_layout(const module& mod) const override;
    bool is_reference_type(const module& mod) const override;

    int yaml2bin(const module& module,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;
};

struct vector_type : generic {
    vector_type()
        : generic(make_generic_declaration({{"T", "type"}})) {
    }

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation& ins) const override;

    bool is_reference(const module& mod,
                      const generic_instantiation& instantiation) const override {
        return true;
    }

    YAML::Node bin2yaml(const module& mod,
                        const generic_instantiation& instantiation,
                        ibinary_reader& span) const override;

    int yaml2bin(const module& mod,
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;
};
} // namespace lidl
