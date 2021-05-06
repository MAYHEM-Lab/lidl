#include "struct_bodygen.hpp"

namespace lidl::cpp {
using codegen::sections;
sections raw_struct_gen::generate() {
    std::cerr << fmt::format("Generating raw struct {}\n", absolute_name());
    std::vector<std::string> members;
    for (auto& [name, member] : get().all_members()) {
        auto member_type_name =
            get_type(mod(), member.type_)->get_wire_type_name(mod(), member.type_);
        members.push_back(generate_field(name, get_identifier(mod(), member_type_name)));
    }

    auto ctor = generate_raw_constructor();

    constexpr auto format = R"__(struct {} {{
            {}
            {}
        }};)__";

    section def_sect;
    def_sect.add_key(def_key());
    def_sect.definition =
        fmt::format(format, name(), fmt::join(ctor, "\n"), fmt::join(members, "\n"));

    // Member types must be defined before us
    for (auto& [name, member] : get().all_members()) {
        auto deps = codegen::def_keys_from_name(mod(), member.type_);
        for (auto& key : deps) {
            def_sect.add_dependency(key);
        }
    }

    sections res{{std::move(def_sect)}};
    return res;
}

bool raw_struct_gen::is_constexpr() const {
    if (get().is_reference_type(mod())) {
        return false;
    }

    return true;
}

std::vector<std::string> raw_struct_gen::generate_raw_constructor() {
    if (get().all_members().empty()) {
        return {};
    }

    std::vector<std::string> arg_names;
    std::vector<std::string> initializer_list;

    for (auto& [member_name, member] : get().all_members()) {
        auto member_type_name =
            get_type(mod(), member.type_)->get_wire_type_name(mod(), member.type_);

        auto identifier = get_user_identifier(mod(), member_type_name);

        if (!member.is_nullable()) {
            arg_names.push_back(fmt::format("const {}& p_{}", identifier, member_name));
            initializer_list.push_back(fmt::format("{0}(p_{0})", member_name));
            continue;
        }
        arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
        initializer_list.push_back(
            fmt::format("{0}(p_{0} ? decltype({0}){{*p_{0}}} : decltype({0}){{nullptr}})",
                        member_name));
    }

    return {(is_constexpr() ? "constexpr " : "") +
            fmt::format("{}({}) : {} {{}}",
                        ctor_name(),
                        fmt::join(arg_names, ", "),
                        fmt::join(initializer_list, ", "))};
}

sections struct_body_gen::generate() {
    std::cerr << fmt::format("Generating struct body for {}\n", m_ctor_name);
    std::vector<std::string> accessors;
    for (auto& [name, member] : str().all_members()) {
        accessors.push_back(generate_getter(name, member, true));
        accessors.push_back(generate_getter(name, member, false));
    }

    raw_struct_gen raw_gen(mod(), "raw_t", "raw_t", "", str());
    auto sects = raw_gen.generate();

    auto ctor = generate_constructor();

    constexpr auto format = R"__(public:
            {}
            {}
        private:
            {}
            [[maybe_unused]] raw_t raw;)__";

    section def;
    def.add_key({&str(), section_type::definition});
    def.definition = fmt::format(format,
                                 fmt::join(ctor, "\n"),
                                 fmt::join(accessors, "\n"),
                                 sects.get_sections().at(0).definition);
    def.depends_on = sects.get_sections().at(0).depends_on;
    return {{std::move(def)}};
}

std::vector<std::string> struct_body_gen::generate_constructor() {
    std::vector<std::string> arg_names;
    std::vector<std::string> initializer_list;

    for (auto& [member_name, member] : str().all_members()) {
        auto member_type_name =
            get_type(mod(), member.type_)->get_wire_type_name(mod(), member.type_);

        auto identifier = get_user_identifier(mod(), member_type_name);

        initializer_list.push_back(fmt::format("p_{}", member_name));

        if (!member.is_nullable()) {
            arg_names.push_back(fmt::format("const {}& p_{}", identifier, member_name));
            continue;
        }
        arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
    }

    return {(is_constexpr() ? "constexpr " : "") +
            fmt::format("{}({}) : raw{{{}}} {{}}",
                        m_ctor_name,
                        fmt::join(arg_names, ", "),
                        fmt::join(initializer_list, ", "))};
}

std::string struct_body_gen::generate_getter(std::string_view member_name,
                                             const member& mem,
                                             bool is_const) {
    assert(!mem.is_nullable());

    std::cerr << fmt::format("Generating getter for {}::{}\n", m_ctor_name, member_name);
    constexpr auto format       = R"__({}& {}() {{ return raw.{}; }})__";
    constexpr auto const_format = R"__(const {}& {}() const {{ return raw.{}; }})__";

    auto member_type_name =
        get_type(mod(), mem.type_)->get_wire_type_name(mod(), mem.type_);

    auto identifier = get_identifier(mod(), deref_ptr(mod(), member_type_name));
    return (is_constexpr() ? "constexpr " : "") +
           fmt::format(
               is_const ? const_format : format, identifier, member_name, member_name);
}

bool struct_body_gen::is_constexpr() const {
    if (str().is_reference_type(mod())) {
        return false;
    }

    return true;
}
} // namespace lidl::cpp