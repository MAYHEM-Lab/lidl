#include "struct_bodygen.hpp"

namespace lidl::cpp {

sections raw_struct_gen::generate() {
    std::vector<std::string> members;
    for (auto& [name, member] : get().members) {
        members.push_back(generate_field(name, get_identifier(mod(), member.type_)));
    }

    auto ctor = generate_raw_constructor();

    constexpr auto format = R"__(struct {} {{
            {}
            {}
        }};)__";

    section def_sect;
    def_sect.key = def_key();
    def_sect.definition =
        fmt::format(format, name(), fmt::join(ctor, "\n"), fmt::join(members, "\n"));

    // Member types must be defined before us
    for (auto& [name, member] : get().members) {
        auto deps = def_keys_from_name(mod(), member.type_);
        for (auto& key : deps) {
            def_sect.add_dependency(key);
        }
    }

    sections res{{std::move(def_sect)}};

    return res;
}

std::vector<std::string> raw_struct_gen::generate_raw_constructor() {
    if (get().members.empty()) {
        return {};
    }

    std::vector<std::string> arg_names;
    std::vector<std::string> initializer_list;

    for (auto& [member_name, member] : get().members) {
        auto member_type = get_type(mod(), member.type_);
        auto identifier  = get_user_identifier(mod(), member.type_);

        if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
            arg_names.push_back(fmt::format("const {}& p_{}", identifier, member_name));
            initializer_list.push_back(fmt::format("{0}(p_{0})", member_name));
            continue;
        }
        arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
        initializer_list.push_back(
            fmt::format("{0}(p_{0} ? decltype({0}){{*p_{0}}} : decltype({0}){{nullptr}})",
                        member_name));
    }

    return {fmt::format("{}({}) : {} {{}}",
                        ctor_name(),
                        fmt::join(arg_names, ", "),
                        fmt::join(initializer_list, ", "))};
}

sections struct_body_gen::generate() {
    std::vector<std::string> accessors;
    for (auto& [name, member] : str().members) {
        accessors.push_back(generate_getter(name, member, true));
        accessors.push_back(generate_getter(name, member, false));
    }

    raw_struct_gen raw_gen(mod(), {}, "raw_t", "", str());
    auto sects = raw_gen.generate();

    auto ctor = generate_constructor();

    constexpr auto format = R"__(public:
            {}
            {}
        private:
            {}
            raw_t raw;)__";

    section def;
    def.key        = {m_symbol, section_type::definition};
    def.definition = fmt::format(format,
                                 fmt::join(ctor, "\n"),
                                 fmt::join(accessors, "\n"),
                                 sects.m_sections[0].definition);
    def.depends_on = sects.m_sections[0].depends_on;
    return {{std::move(def)}};
}

std::vector<std::string> struct_body_gen::generate_constructor() {
    std::vector<std::string> arg_names;
    std::vector<std::string> initializer_list;

    for (auto& [member_name, member] : str().members) {
        auto member_type = get_type(mod(), member.type_);
        auto identifier  = get_user_identifier(mod(), member.type_);
        initializer_list.push_back(fmt::format("p_{}", member_name));

        if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
            arg_names.push_back(fmt::format("const {}& p_{}", identifier, member_name));
            continue;
        }
        arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
    }

    return {fmt::format("{}({}) : raw{{{}}} {{}}",
                        m_ctor_name,
                        fmt::join(arg_names, ", "),
                        fmt::join(initializer_list, ", "))};
}

std::string struct_body_gen::generate_getter(std::string_view member_name,
                                             const member& mem,
                                             bool is_const) {
    auto member_type = get_type(mod(), mem.type_);
    if (!member_type->is_reference_type(mod())) {
        auto type_name              = get_identifier(mod(), mem.type_);
        constexpr auto format       = R"__({}& {}() {{ return raw.{}; }})__";
        constexpr auto const_format = R"__(const {}& {}() const {{ return raw.{}; }})__";
        return fmt::format(
            is_const ? const_format : format, type_name, member_name, member_name);
    } else {
        // need to dereference before return
        auto& base      = std::get<name>(mem.type_.args[0]);
        auto identifier = get_identifier(mod(), base);
        if (!mem.is_nullable()) {
            constexpr auto format = R"__({}& {}() {{ return raw.{}.unsafe().get(); }})__";
            constexpr auto const_format =
                R"__(const {}& {}() const {{ return raw.{}.unsafe().get(); }})__";
            return fmt::format(
                is_const ? const_format : format, identifier, member_name, member_name);
        }
    }
    return "";
}
} // namespace lidl::cpp