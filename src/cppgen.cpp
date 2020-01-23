#include <fmt/core.h>
#include <fmt/format.h>
#include <lidl/basic.hpp>
#include <lidl/module.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace lidl {
namespace {
std::optional<std::string> known_type_conversion(const std::string_view& name) {
    std::unordered_map<std::string_view, std::string_view> basic_types{
        {"f32", "float"},
        {"f64", "double"},
        {"i8", "int8_t"},
        {"i16", "int16_t"},
        {"i32", "int32_t"},
        {"i64", "int64_t"},
        {"u8", "uint8_t"},
        {"u16", "uint16_t"},
        {"u32", "uint32_t"},
        {"u64", "uint64_t"},
        {"array", "::lidl::array"},
        {"optional", "::lidl::optional"},
        {"ptr", "::lidl::ptr"},
        {"string", "::lidl::string"},
        {"vector", "::lidl::vector"}};

    if (auto it = basic_types.find(name); it != basic_types.end()) {
        return std::string(it->second);
    }

    return std::nullopt;
}

std::string get_identifier_for_type(const module& mod, const type& t) {
    auto sym = mod.syms.reverse_lookup(&t);

    if (!sym) {
        auto gen_it = mod.generated.find(&t);
        if (gen_it == mod.generated.end()) {
            throw std::runtime_error("wtf");
        }
        bool first = false;
        std::string s;
        for (auto& piece : gen_it->second) {
            if (auto num = std::get_if<int64_t>(&piece); num) {
                if (!first) {
                    throw std::runtime_error("shouldn't happen");
                }
                s += std::to_string(*num) + ", ";
            } else if (auto sym = std::get_if<const symbol*>(&piece); sym) {
                auto name = std::string(mod.syms.name_of(*sym));
                if (auto res = known_type_conversion(name); res) {
                    name = *res;
                }
                if (!first) {
                    s = name + "<";
                    first = true;
                    continue;
                }
                s += name + ", ";
            }
        }
        s.pop_back();
        s.back() = '>';
        return s;
    }

    auto name = std::string(mod.syms.name_of(sym));
    if (auto res = known_type_conversion(name); res) {
        name = *res;
    }
    return name;
}

std::string private_name_for(std::string_view name) {
    using namespace std::string_literals;
    return "m_" + std::string(name);
}

std::string
generate_getter(std::string_view name, std::string_view type_name, bool is_const) {
    constexpr auto format = R"__({}& {}() {{ return m_raw.{}; }})__";
    constexpr auto const_format = R"__(const {}& {}() const {{ return m_raw.{}; }})__";
    return fmt::format(is_const ? const_format : format, type_name, name, name);
}

void generate_raw_struct_field(std::string_view name,
                               std::string_view type_name,
                               std::ostream& str) {
    str << fmt::format("{} {};\n", type_name, name);
}

void generate_struct_field(std::string_view name,
                           std::string_view type_name,
                           std::ostream& str) {
    str << generate_getter(name, type_name, true) << '\n';
    str << generate_getter(name, type_name, false) << '\n';
}

void generate_raw_struct(const module& m,
                         std::string_view name,
                         const structure& s,
                         std::ostream& str) {
    std::stringstream pub;

    for (auto& [name, member] : s.members) {
        generate_raw_struct_field(name, get_identifier_for_type(m, *member.type_), pub);
    }

    constexpr auto format = R"__(struct {} {{
        {}
    }};)__";

    str << fmt::format(format, name, pub.str()) << '\n';
}

void generate_struct(const module& m,
                     std::string_view name,
                     const structure& s,
                     std::ostream& str) {
    std::stringstream raw_part;

    auto raw_name = std::string(name) + "_raw";
    generate_raw_struct(m, raw_name, s, raw_part);

    std::stringstream struct_helper;

    for (auto& [name, member] : s.members) {
        generate_struct_field(
            name, get_identifier_for_type(m, *member.type_), struct_helper);
    }

    constexpr auto format = R"__(class {} {{
    public:
        {}
    private:
        {}
        {} m_raw;
    }};)__";

    str << fmt::format(format, name, struct_helper.str(), raw_part.str(), raw_name)
        << '\n';
}

void generate_static_asserts(const module& mod,
                             std::string_view name,
                             const type& t,
                             std::ostream& str) {
    constexpr auto size_format =
        R"__(static_assert(sizeof({}) == {}, "Size of {} does not match the wire size!");)__";
    constexpr auto align_format =
        R"__(static_assert(alignof({}) == {}, "Alignment of {} does not match the wire alignment!");)__";

    str << fmt::format(size_format, name, t.wire_layout(mod).size(), name) << '\n';
    str << fmt::format(align_format, name, t.wire_layout(mod).alignment(), name) << '\n';
}

void generate_makers(const module& mod,
                     std::string_view name,
                     const structure& s,
                     std::ostream& str);
} // namespace

void generate(const module& mod, std::ostream& str) {
    str << "#pragma once\n\n#include <lidl/lidl.hpp>\n\n";
    for (auto& [name, s] : mod.structs) {
        if (s.is_raw(mod)) {
            generate_raw_struct(mod, name, s, str);
            str << '\n';
        } else {
            generate_struct(mod, name, s, str);
            str << '\n';
        }
        generate_static_asserts(mod, name, s, str);
    }

    for (auto& [name, s] : mod.generic_structs) {
        std::cerr << "Codegen for generic structs not supported!";
    }
}
} // namespace lidl