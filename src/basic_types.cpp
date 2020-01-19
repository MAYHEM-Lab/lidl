#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>
#include <lidl/types.hpp>
#include <string_view>
#include <unordered_map>

namespace lidl {
namespace {
struct basic_type : type {
    explicit basic_type(int bits)
        : size_in_bits(bits) {
    }

    virtual bool is_raw(const module&) const override {
        return true;
    }

    int32_t size_in_bits;
};

struct integral_type : basic_type {
    explicit integral_type(int bits, bool unsigned_)
        : basic_type(bits)
        , is_unsigned(unsigned_) {
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
};

struct double_type : basic_type {
    explicit double_type()
        : basic_type(64) {
    }
};

struct string_type : type {};
} // namespace


struct optional_type : generic_type {
    optional_type()
        : generic_type(make_generic_declaration({{"T", "type"}})) {
    }

    bool is_raw(const module& mod,
                const struct generic_instantiation& instantiation) const override {
        auto arg = std::get<const symbol*>(instantiation.arguments()[0]);
        if (auto regular = std::get_if<const type*>(arg); regular) {
            return (*regular)->is_raw(mod);
        }
        return false;
    }
};

struct vector_type : generic_type {
    vector_type()
        : generic_type(make_generic_declaration({{"T", "type"}})) {
    }
};

struct pointer_type : generic_type {
    pointer_type()
        : generic_type(make_generic_declaration({{"T", "type"}})) {
    }
};

struct array_type : generic_type {
    array_type()
        : generic_type(make_generic_declaration({{"T", "type"}, {"Size", "i32"}})) {
    }

    bool is_raw(const module& mod,
                const struct generic_instantiation& instantiation) const override {
        auto arg = std::get<const symbol*>(instantiation.arguments()[0]);
        if (auto regular = std::get_if<const type*>(arg); regular) {
            return (*regular)->is_raw(mod);
        }
        return false;
    }
};

void add_basic_types(symbol_table& db) {
    db.define("i8", std::make_unique<integral_type>(8, false));
    db.define("i16", std::make_unique<integral_type>(16, false));
    db.define("i32", std::make_unique<integral_type>(32, false));
    db.define("i64", std::make_unique<integral_type>(64, false));

    db.define("u8", std::make_unique<integral_type>(8, true));
    db.define("u16", std::make_unique<integral_type>(16, true));
    db.define("u32", std::make_unique<integral_type>(32, true));
    db.define("u64", std::make_unique<integral_type>(64, true));

    db.define("f16", std::make_unique<half_type>());
    db.define("f32", std::make_unique<float_type>());
    db.define("f64", std::make_unique<double_type>());

    db.define("string", std::make_unique<string_type>());

    db.define("ptr", std::make_unique<pointer_type>());
    db.define("vector", std::make_unique<vector_type>());
    db.define("array", std::make_unique<array_type>());
    db.define("optional", std::make_unique<optional_type>());
}
} // namespace lidl
