#include "lidl/layout.hpp"

#include <cmath>
#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/basic_types.hpp>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>
#include <lidl/types.hpp>
#include <lidl/view_types.hpp>
#include <string_view>
#include <unordered_map>

namespace lidl {
pointer_type::pointer_type(module& mod)
    : generic_wire_type(&mod, {}, make_generic_declaration({{"T", "type"}})) {
}

raw_layout pointer_type::wire_layout(const module& mod, const name&) const {
    return raw_layout{2, 2};
}

YAML::Node pointer_type::bin2yaml(const module& mod,
                                  const name& instantiation,
                                  ibinary_reader& reader) const {
    auto& arg = std::get<name>(instantiation.args[0]);
    if (auto pointee = get_wire_type(mod, arg); pointee) {
        auto off = reader.read_object<int16_t>();
        reader.seek(-off);
        return pointee->bin2yaml(mod, reader);
    }
    throw std::runtime_error("pointee must be a regular type");
}

int pointer_type::yaml2bin(const module& mod,
                           const name& instantiation,
                           const YAML::Node& node,
                           ibinary_writer& writer) const {
    auto& arg = std::get<name>(instantiation.args[0]);
    if (auto pointee = get_type(mod, arg); pointee) {
        auto pointee_pos = node.as<int>();
        writer.align(2);
        uint16_t diff = writer.tell() - pointee_pos;
        auto pos      = writer.tell();
        writer.write(diff);
        return pos;
    }
    throw std::runtime_error("pointee must be a regular type");
}

type_categories pointer_type::category(const module& mod,
                                       const name& instantiation) const {
    return type_categories::reference;
}

std::unique_ptr<module> basic_module() {
    auto basic_mod = std::make_unique<module>();

    basic_mod->set_imported();

    auto add_type = [&](std::string_view name, std::unique_ptr<type> t) {
        t->set_intrinsic();
        basic_mod->basic_types.emplace_back(std::move(t));
        return define(basic_mod->symbols(), name, basic_mod->basic_types.back().get());
    };

    auto add_generic = [&](std::string_view name, std::unique_ptr<generic_type> t) {
        t->set_intrinsic();
        basic_mod->basic_generics.emplace_back(std::move(t));
        define(basic_mod->symbols(), name, basic_mod->basic_generics.back().get());
    };

    auto add_view = [&](std::string_view name, std::unique_ptr<view_type> t) {
        t->set_intrinsic();
        basic_mod->basic_views.emplace_back(std::move(t));
        define(basic_mod->symbols(), name, basic_mod->basic_views.back().get());
    };

    auto add_generic_view = [&](std::string_view name,
                                std::unique_ptr<generic_view_type> t) {
        t->set_intrinsic();
        basic_mod->basic_generic_views.emplace_back(std::move(t));
        define(basic_mod->symbols(), name, basic_mod->basic_generic_views.back().get());
    };

    add_type("bool", std::make_unique<bool_type>());

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

    auto str = add_type("string", std::make_unique<string_type>());

    add_view("string_view", std::make_unique<known_view_type>(name{str}));
    add_generic_view("span", std::make_unique<generic_span_type>(*basic_mod));

    add_generic("ptr", std::make_unique<pointer_type>(*basic_mod));
    add_generic("vector", std::make_unique<vector_type>(*basic_mod));
    add_generic("array", std::make_unique<array_type>(*basic_mod));
    return basic_mod;
}

array_type::array_type(module& mod)
    : generic_wire_type(
    &mod, {}, make_generic_declaration({{"T", "type"}, {"Size", "i32"}})) {
}

name array_type::get_wire_type_name_impl(const module& mod, const name& your_name) const {
    auto wire_name_of_arg = get_wire_type_name(mod, your_name.args.front().as_name());
    return name{your_name.base, {wire_name_of_arg, your_name.args.at(1)}};
}

type_categories array_type::category(const module& mod, const name& instantiation) const {
    auto& arg    = instantiation.args.front().as_name();
    auto regular = get_wire_type(mod, arg);
    assert(regular);
    return regular->category(mod);
}

int vector_type::yaml2bin(const module& mod,
                          const name& instantiation,
                          const YAML::Node& node,
                          ibinary_writer& writer) const {
    auto& arg    = std::get<name>(instantiation.args[0]);
    auto pointee = get_wire_type(mod, arg);
    Expects(pointee != nullptr);

    if (pointee->is_reference_type(mod)) {
        auto actual_pointee =
            get_wire_type(mod, std::get<name>(arg.args[0].get_variant()));

        std::vector<int> positions;
        for (auto& elem : node) {
            writer.align(actual_pointee->wire_layout(mod).alignment());
            positions.push_back(actual_pointee->yaml2bin(mod, elem, writer));
        }

        auto pos = writer.tell();
        writer.align(2);
        writer.write<int16_t>(node.size());

        for (auto pos : positions) {
            YAML::Node ptr_node(pos);
            writer.align(pointee->wire_layout(mod).alignment());
            pointee->yaml2bin(mod, ptr_node, writer);
        }

        return pos;
    } else {
        auto pos = writer.tell();
        writer.align(2);
        writer.write<int16_t>(node.size());

        for (auto& elem : node) {
            writer.align(pointee->wire_layout(mod).alignment());
            pointee->yaml2bin(mod, elem, writer);
        }
        return pos;
    }
}

YAML::Node vector_type::bin2yaml(const module& mod,
                                 const name& instantiation,
                                 ibinary_reader& reader) const {
    auto& arg = std::get<name>(instantiation.args[0]);
    if (auto pointee = get_wire_type(mod, arg); pointee) {
        auto size = reader.read_object<int16_t>();
        reader.seek(2);

        auto layout = pointee->wire_layout(mod);
        reader.align(layout.alignment());

        auto begin = reader.tell();

        auto arr = YAML::Node();
        for (int i = 0; i < size; ++i) {
            reader.seek(begin + i * layout.size(), std::ios::beg);
            auto yaml = pointee->bin2yaml(mod, reader);
            arr.push_back(yaml);
        }

        return arr;
    }

    throw std::runtime_error("pointee must be a regular type");
}

name vector_type::get_wire_type_name_impl(const module& mod,
                                          const name& your_name) const {
    auto ptr_sym = recursive_name_lookup(mod.symbols(), "ptr").value();

    auto wire_name_of_arg = get_wire_type_name(mod, your_name.args.front().as_name());

    return name{ptr_sym, {name{your_name.base, {wire_name_of_arg}}}};
}

vector_type::vector_type(module& mod)
    : generic_reference_type(&mod, {}, make_generic_declaration({{"T", "type"}})) {
}

int string_type::yaml2bin(const module& module,
                          const YAML::Node& node,
                          ibinary_writer& writer) const {
    auto str = node.as<std::string>();
    writer.align(2);
    auto pos = writer.tell();
    writer.write<int16_t>(str.size());
    writer.write_raw_string(str);
    return pos;
}

YAML::Node string_type::bin2yaml(const module&, ibinary_reader& reader) const {
    /**
     * 2 bytes length + lenght many chars
     */

    auto len = reader.read_object<int16_t>();
    reader.seek(2);
    auto raw_str = reader.read_bytes(len);
    return YAML::Node(
        std::string(reinterpret_cast<const char*>(raw_str.data()), raw_str.size()));
}

YAML::Node bool_type::bin2yaml(const module& module, ibinary_reader& reader) const {
    return YAML::Node(reader.read_object<bool>());
}

int bool_type::yaml2bin(const module& mod,
                        const YAML::Node& node,
                        ibinary_writer& writer) const {
    return 0;
}
} // namespace lidl
