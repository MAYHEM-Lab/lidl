#include <cmath>
#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>
#include <lidl/types.hpp>
#include <string_view>
#include <unordered_map>

namespace lidl {
namespace {
struct basic_type : value_type {
    explicit basic_type(int bits)
        : size_in_bits(bits) {
    }

    virtual raw_layout wire_layout(const module&) const override {
        return raw_layout(size_in_bytes(), size_in_bytes());
    }

    size_t size_in_bytes() const {
        return std::ceil(size_in_bits / 8.f);
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

    bool is_unsigned;
};

struct half_type : basic_type {
    explicit half_type()
        : basic_type(16) {
    }
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
};
} // namespace

struct vector_type : generic {
    vector_type()
        : generic(make_generic_declaration({{"T", "type"}})) {
    }

    virtual raw_layout wire_layout(const module& mod,
                                   const struct generic_instantiation&) const override {
        return raw_layout{2, 2};
    }

    bool is_reference(const module& mod,
                      const struct generic_instantiation& instantiation) const override {
        return true;
    }

    std::pair<YAML::Node, size_t>
    bin2yaml(const module& module,
             const struct generic_instantiation& instantiation,
             gsl::span<const uint8_t> span) const override {
        auto ptr_span = span.subspan(span.size() - 2, 2);
        uint16_t off{0};
        memcpy(&off, ptr_span.data(), ptr_span.size());

        YAML::Node arr;
        if (off == 0) {
            return {arr, 2};
        }

        auto& arg = std::get<name>(instantiation.arguments()[0]);
        if (auto pointee = get_type(arg); pointee) {
            auto layout = pointee->wire_layout(module);
            auto len = off / layout.size();

            for (int i = 0; i < len; ++i) {
                auto obj_span =
                    span.subspan(0, span.size() - 2 - off + (i + 1) * layout.size());
                auto [yaml, consumed] = pointee->bin2yaml(module, obj_span);
                arr.push_back(std::move(yaml));
            }

            return {std::move(arr), layout.size() * len};
        }

        throw std::runtime_error("pointee must be a regular type");
    }
};

pointer_type::pointer_type()
    : generic(make_generic_declaration({{"T", "type"}})) {
}

raw_layout pointer_type::wire_layout(const module& mod,
                                     const struct generic_instantiation&) const {
    return raw_layout{2, 2};
}

std::pair<YAML::Node, size_t>
pointer_type::bin2yaml(const module& module,
                       const struct generic_instantiation& instantiation,
                       gsl::span<const uint8_t> span) const {
    auto& arg = std::get<name>(instantiation.arguments()[0]);
    if (auto pointee = get_type(arg); pointee) {
        auto ptr_span = span.subspan(span.size() - 2, 2);
        uint16_t off{0};
        memcpy(&off, ptr_span.data(), ptr_span.size());
        auto obj_span =
            span.subspan(0, span.size() - 2 - off + pointee->wire_layout(module).size());
        auto [yaml, consumed] = pointee->bin2yaml(module, obj_span);
        return {std::move(yaml), consumed + 2};
    }
    throw std::runtime_error("pointee must be a regular type");
}

struct array_type : generic {
    array_type()
        : generic(make_generic_declaration({{"T", "type"}, {"Size", "i32"}})) {
    }

    bool is_reference(const module& mod,
                      const struct generic_instantiation& instantiation) const override {
        auto arg = std::get<name>(instantiation.arguments()[0]);
        if (auto regular = get_type(arg); regular) {
            return regular->is_reference_type(mod);
        }
        return false;
    }

    virtual raw_layout
    wire_layout(const module& mod,
                const struct generic_instantiation& instantiation) const override {
        if (is_reference(mod, instantiation)) {
            return {2, 2};
        }
        auto& arg = std::get<name>(instantiation.arguments()[0]);
        if (auto regular = get_type(arg); regular) {
            auto layout = regular->wire_layout(mod);
            auto len = std::get<int64_t>(instantiation.arguments()[1]);
            return raw_layout(layout.size() * len, layout.alignment());
        }
        throw std::runtime_error("Array type is not regular!");
    }

    std::pair<YAML::Node, size_t>
    bin2yaml(const module& module,
             const struct generic_instantiation& instantiation,
             gsl::span<const uint8_t> span) const override {
        if (is_reference(module, instantiation)) {
            throw std::runtime_error("not implemented");
        }

        YAML::Node arr;
        auto& arg = std::get<name>(instantiation.arguments()[0]);
        if (auto pointee = get_type(arg); pointee) {
            auto layout = pointee->wire_layout(module);
            auto len = std::get<int64_t>(instantiation.arguments()[1]);

            for (int i = 0; i < len; ++i) {
                auto obj_span =
                    span.subspan(0, span.size() - (len - i - 1) * layout.size());
                auto [yaml, consumed] = pointee->bin2yaml(module, obj_span);
                arr.push_back(std::move(yaml));
            }

            return {std::move(arr), layout.size() * len};
        }

        throw std::runtime_error("pointee must be a regular type");
    }
};

void add_basic_types(module& m) {
    auto add_type = [&](std::string_view name, std::unique_ptr<type> t) {
        m.basic_types.emplace_back(std::move(t));
        define(*m.symbols, name, m.basic_types.back().get());
    };

    auto add_generic = [&](std::string_view name, std::unique_ptr<generic> t) {
        m.basic_generics.emplace_back(std::move(t));
        define(*m.symbols, name, m.basic_generics.back().get());
    };

    add_type("bool", std::make_unique<integral_type>(1, false));

    add_type("i8", std::make_unique<integral_type>(8, false));
    add_type("i16", std::make_unique<integral_type>(16, false));
    add_type("i32", std::make_unique<integral_type>(32, false));
    add_type("i64", std::make_unique<integral_type>(64, false));

    add_type("u8", std::make_unique<integral_type>(8, true));
    add_type("u16", std::make_unique<integral_type>(16, true));
    add_type("u32", std::make_unique<integral_type>(32, true));
    add_type("u64", std::make_unique<integral_type>(64, true));

    add_type("f16", std::make_unique<half_type>());
    add_type("f32", std::make_unique<float_type>());
    add_type("f64", std::make_unique<double_type>());

    add_type("string", std::make_unique<string_type>());

    add_generic("ptr", std::make_unique<pointer_type>());
    add_generic("vector", std::make_unique<vector_type>());
    add_generic("array", std::make_unique<array_type>());
}
} // namespace lidl
