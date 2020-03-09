#include "lidl/layout.hpp"

#include <cmath>
#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>
#include <lidl/types.hpp>
#include <lidl/basic_types.hpp>
#include <string_view>
#include <unordered_map>

namespace lidl {
pointer_type::pointer_type()
    : generic(make_generic_declaration({{"T", "type"}})) {
}

raw_layout pointer_type::wire_layout(const module& mod,
                                     const generic_instantiation&) const {
    return raw_layout{2, 2};
}

std::pair<YAML::Node, size_t>
pointer_type::bin2yaml(const module& mod,
                       const generic_instantiation& instantiation,
                       gsl::span<const uint8_t> span) const {
    auto& arg = std::get<name>(instantiation.arguments()[0]);
    if (auto pointee = get_type(mod, arg); pointee) {
        auto ptr_span = span.subspan(span.size() - 2, 2);
        int16_t off{0};
        memcpy(&off, ptr_span.data(), ptr_span.size());
        auto obj_span =
            span.subspan(0, span.size() - 2 - off + pointee->wire_layout(mod).size());
        auto [yaml, consumed] = pointee->bin2yaml(mod, obj_span);
        return {std::move(yaml), consumed + 2};
    }
    throw std::runtime_error("pointee must be a regular type");
}

int pointer_type::yaml2bin(const module& mod,
                           const generic_instantiation& instantiation,
                           const YAML::Node& node,
                           ibinary_writer& writer) const {
    auto& arg = std::get<name>(instantiation.arguments()[0]);
    if (auto pointee = get_type(mod, arg); pointee) {
        auto pointee_pos = node.as<int>();
        writer.align(2);
        uint16_t diff = writer.tell() - pointee_pos;
        auto pos = writer.tell();
        writer.write(diff);
        return pos;
    }
    throw std::runtime_error("pointee must be a regular type");
}


namespace {
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

    add_type("f32", std::make_unique<float_type>());
    add_type("f64", std::make_unique<double_type>());

    add_type("string", std::make_unique<string_type>());

    add_generic("ptr", std::make_unique<pointer_type>());
    add_generic("vector", std::make_unique<vector_type>());
    add_generic("array", std::make_unique<array_type>());
}
}

const module& get_root_module() {
    static auto m = [] {
        module m;
        add_basic_types(m);
        return m;
    }();
    return m;
}

bool array_type::is_reference(const module& mod,
                              const generic_instantiation& instantiation) const {
    auto arg = std::get<name>(instantiation.arguments()[0]);
    if (auto regular = get_type(mod, arg); regular) {
        return regular->is_reference_type(mod);
    }
    return false;
}

int vector_type::yaml2bin(const module& mod,
                          const generic_instantiation& instantiation,
                          const YAML::Node& node,
                          ibinary_writer& writer) const {
    auto init_pos = writer.tell();
    auto& arg = std::get<name>(instantiation.arguments()[0]);
    if (auto pointee = get_type(mod, arg); pointee) {
        for (auto& elem : node) {
            pointee->yaml2bin(mod, elem, writer);
        }
    }
    writer.align(2);
    auto pos = writer.tell();
    uint16_t diff = writer.tell() - init_pos;
    writer.write(diff);
    return pos;
}

const type * pointer_type::pointee_type(const module& mod,
                                        const generic_instantiation& instantiation) const {
    auto& arg = std::get<name>(instantiation.arguments()[0]);
    if (auto pointee = get_type(mod, arg); pointee) {
        return pointee;
    }
    return nullptr;
}
} // namespace lidl
