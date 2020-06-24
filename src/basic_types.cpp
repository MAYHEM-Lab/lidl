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
        auto pos      = writer.tell();
        writer.write(diff);
        return pos;
    }
    throw std::runtime_error("pointee must be a regular type");
}


std::unique_ptr<module> basic_module() {
    auto basic_mod = std::make_unique<module>();

    auto add_type = [&](std::string_view name, std::unique_ptr<type> t) {
        basic_mod->basic_types.emplace_back(std::move(t));
        return define(*basic_mod->symbols, name, basic_mod->basic_types.back().get());
    };

    auto add_generic = [&](std::string_view name, std::unique_ptr<generic> t) {
        basic_mod->basic_generics.emplace_back(std::move(t));
        define(*basic_mod->symbols, name, basic_mod->basic_generics.back().get());
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

    auto str = add_type("string", std::make_unique<string_type>());

    add_type("string_view", std::make_unique<view_type>(name{str}));

    add_generic("ptr", std::make_unique<pointer_type>());
    add_generic("vector", std::make_unique<vector_type>());
    add_generic("array", std::make_unique<array_type>());
    return basic_mod;
}

const generic_instantiation& module::create_or_get_instantiation(const name& ins) const {
    auto it = std::find_if(
        name_ins.begin(), name_ins.end(), [&](auto& p) { return p.first == ins; });
    if (it != name_ins.end()) {
        return *it->second;
    }

    instantiations.emplace_back(ins);
    name_ins.emplace_back(ins, &instantiations.back());
    return instantiations.back();
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
    auto& arg    = std::get<name>(instantiation.arguments()[0]);
    auto pointee = get_type(mod, arg);
    Expects(pointee != nullptr);

    int init_pos = 0;
    if (pointee->is_reference_type(mod)) {
        auto actual_pointee = get_type(mod, std::get<name>(arg.args[0].get_variant()));

        std::vector<int> positions;
        for (auto& elem : node) {
            writer.align(actual_pointee->wire_layout(mod).alignment());
            positions.push_back(actual_pointee->yaml2bin(mod, elem, writer));
        }

        writer.align(2);
        init_pos = writer.tell();
        for (auto pos : positions) {
            YAML::Node ptr_node(pos);
            writer.align(pointee->wire_layout(mod).alignment());
            pointee->yaml2bin(mod, ptr_node, writer);
        }
    } else {
        init_pos = writer.tell();
        for (auto& elem : node) {
            writer.align(pointee->wire_layout(mod).alignment());
            pointee->yaml2bin(mod, elem, writer);
        }
    }

    writer.align(2);
    auto pos      = writer.tell();
    uint16_t diff = writer.tell() - init_pos;
    writer.write(diff);
    return pos;
}

std::pair<YAML::Node, size_t>
vector_type::bin2yaml(const module& mod,
                      const generic_instantiation& instantiation,
                      gsl::span<const uint8_t> span) const {
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

int string_type::yaml2bin(const module& module,
                          const YAML::Node& node,
                          ibinary_writer& writer) const {
    /**
     * A string is encoded as the contents of the string, plus a pointer for length.
     * We need to pre-align the string so that the pointer ends up on a 2 byte boundary,
     * and is well-aligned.
     * If we don't do the pre-alignment, the buffer ends up looking like this for odd
     * length strings, for instance "hello":
     *
     * |h|e|l|l|o|0|6|0|
     *
     * That 0 byte was inserted due to aligning the pointer to a 2 byte boundary. This
     * causes to pointer to be 1 byte away from the beginning of the string, and will
     * place 6 into the pointer. Since the size of a string is equal to it's pointer's
     * offset, any lidl runtime will think this is a 6 element string, whereas the user
     * probably intended it to be a 5 element string.
     *
     * Therefore, to put the pointer on a 2 byte address, we need to _pre-align_ the
     * string so that it looks like this:
     *
     * |0|h|e|l|l|o|5|0|
     *
     * This way, the length is 5 as expected.
     */
    auto str     = node.as<std::string>();
    auto pre_pad = (writer.tell() + str.size()) % 2;
    std::vector<char> pad(pre_pad);
    writer.write_raw(pad);
    writer.write_raw(str);
    writer.align(2); // Unnecessary, but kept for completeness
    auto pos     = writer.tell();
    uint16_t len = str.size();
    writer.write(len);
    return pos;
}

std::pair<YAML::Node, size_t> string_type::bin2yaml(const module&,
                                                    gsl::span<const uint8_t> span) const {
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
} // namespace lidl
