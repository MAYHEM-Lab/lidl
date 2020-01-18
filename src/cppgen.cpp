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
std::optional<identifier> known_type_conversion(const identifier& name) {
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
        {"string", "lidl::string"},
        {"vector", "lidl::vector"}};

    if (auto it = basic_types.find(name.name); it != basic_types.end()) {
        identifier newname = name;
        newname.name = it->second;
        return newname;
    }

    return std::nullopt;
}

identifier get_identifier_for_type(const type& t) {
    auto name = t.name();
    if (auto basic_name = known_type_conversion(name); basic_name) {
        name = *basic_name;
    }
    return name;
}

std::string private_name_for(std::string_view name) {
    using namespace std::string_literals;
    return "m_" + std::string(name);
}

std::string generate_getter(std::string_view name, identifier type, bool is_const) {
    constexpr auto format = R"__({}& {}() {{ return m_raw.{}; }})__";
    constexpr auto const_format = R"__(const {}& {}() const {{ return m_raw.{}; }})__";
    return fmt::format(is_const ? const_format : format, to_string(type), name, name);
}

void generate_raw_struct_field(std::string_view name,
                               const type& type,
                               std::ostream& str) {
    auto ident = get_identifier_for_type(type);
    str << fmt::format("{} {};\n", to_string(ident), name);
}

void generate_struct_field(std::string_view name, const type& type, std::ostream& str) {
    str << generate_getter(name, get_identifier_for_type(type), true) << '\n';
    str << generate_getter(name, get_identifier_for_type(type), false) << '\n';
}

void generate_raw_struct(std::string_view name, const structure& s, std::ostream& str) {
    std::stringstream pub;

    for (auto& [name, member] : s.members) {
        generate_raw_struct_field(name, *member.type_, pub);
    }

    constexpr auto format = R"__(struct {} {{
        {}
    }};)__";

    str << fmt::format(format, name, pub.str()) << '\n';
}

void generate_struct(identifier name, const structure& s, std::ostream& str) {
    std::stringstream raw_part;

    auto raw_name = std::string(name.name) + "_raw";
    generate_raw_struct(raw_name, s, raw_part);

    std::stringstream struct_helper;

    for (auto& [name, member] : s.members) {
        generate_struct_field(name, *member.type_, struct_helper);
    }

    std::string param_str;
    for (auto& param : name.parameters) {
        param_str += "typename " + param.name + ",";
    }

    if (!param_str.empty()) {
        param_str = "template <" + param_str;
        param_str.back() = '>';
        param_str.push_back('\n');
    }

    constexpr auto format = R"__({}class {} {{
    public:
        {}
    private:
        {}
        {} m_raw;
    }};)__";

    str << fmt::format(format,
                       param_str,
                       name.name,
                       struct_helper.str(),
                       raw_part.str(),
                       raw_name)
        << '\n';
}
} // namespace

void generate(const module& mod, std::ostream& str) {
    for (auto& [name, s] : mod.structs) {
        if (s.is_raw()) {
            generate_raw_struct(name.name, s, str);
            str << '\n';
        } else {
            generate_struct(name, s, str);
            str << '\n';
        }
    }
}
} // namespace lidl