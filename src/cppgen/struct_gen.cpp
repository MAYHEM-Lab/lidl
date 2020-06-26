//
// Created by fatih on 6/3/20.
//

#include "struct_gen.hpp"

#include "cppgen.hpp"
#include "struct_bodygen.hpp"

#include <lidl/service.hpp>


namespace lidl::cpp {
sections struct_gen::do_generate() {
    constexpr auto format = R"__(class {0} : public ::lidl::struct_base<{0}> {{
            {1}
        }};)__";

    auto body = struct_body_gen(mod(), symbol(), name(), ctor_name(), get()).generate();

    section s;
    s.keys.push_back(def_key());
    s.definition = fmt::format(format, name(), body.m_sections[0].definition);
    s.depends_on = body.m_sections[0].depends_on;
    s.name_space = mod().name_space;

    auto result = generate_traits();
    result.add(std::move(s));
    return result;
}

sections struct_gen::generate_traits() {
    std::vector<std::string> member_types;
    for (auto& [memname, member] : get().members) {
        auto identifier = get_user_identifier(mod(), member.type_);
        member_types.push_back(identifier);
    }

    std::vector<std::string> members;
    for (auto& [memname, member] : get().members) {
        members.push_back(fmt::format(
            "member_info{{\"{1}\", &{0}::{1}, &{0}::{1}}}", absolute_name(), memname));
    }

    std::vector<std::string> ctor_types{""};
    std::vector<std::string> ctor_args{""};
    for (auto& [member_name, member] : get().members) {
        auto member_type = get_type(mod(), member.type_);
        auto identifier  = get_user_identifier(mod(), member.type_);
        ctor_args.push_back(fmt::format("p_{}", member_name));

        if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
            ctor_types.push_back(fmt::format("const {}& p_{}", identifier, member_name));
            continue;
        }
        ctor_types.push_back(fmt::format("const {}* p_{}", identifier, member_name));
    }

    constexpr auto format = R"__(template <>
            struct struct_traits<{0}> {{
                using raw_members = meta::list<{4}>;
                static constexpr auto members = std::make_tuple({1});
                static constexpr auto arity = std::tuple_size_v<decltype(members)>;
                static constexpr const char* name = "{0}";
                static {0}& ctor(::lidl::message_builder& builder{2}) {{
                    return ::lidl::create<{0}>(builder{3});
                }}
            }};)__";

    section trait_sect;
    trait_sect.name_space = "lidl";
    trait_sect.definition = fmt::format(format,
                                        absolute_name(),
                                        fmt::join(members, ", "),
                                        fmt::join(ctor_types, ", "),
                                        fmt::join(ctor_args, ", "),
                                        fmt::join(member_types, ", "));
    trait_sect.add_dependency(def_key());

    constexpr auto std_format = R"__(template <>
            struct tuple_size<{}> : std::integral_constant<std::size_t, {}> {{
            }};)__";

    std::vector<std::string> tuple_elements;
    int idx = 0;
    for (auto& [memname, member] : get().members) {
        tuple_elements.push_back(fmt::format(
            "template <> struct tuple_element<{}, {}> {{ using type = {}; }};",
            idx++,
            absolute_name(),
            get_user_identifier(mod(), member.type_)));
    }

    section std_trait_sect;
    std_trait_sect.name_space = "std";
    std_trait_sect.definition = fmt::format(std_format, absolute_name(), members.size()) +
                                fmt::format("{}", fmt::join(tuple_elements, "\n"));
    std_trait_sect.add_dependency(def_key());

    auto res = sections{{std::move(trait_sect), std::move(std_trait_sect)}};

    if (auto attr =
            get().attributes.get<procedure_params_attribute>("procedure_params")) {
        constexpr auto rpc_trait_format = R"__(template <> struct rpc_param_traits<{}> {{
            static constexpr auto params_for = &{}::{};
        }};)__";

        auto serv_handle    = *recursive_definition_lookup(*mod().symbols, attr->serv);
        auto serv_full_name = get_identifier(mod(), {serv_handle});

        section rpc_traits_sect;
        rpc_traits_sect.name_space = "lidl";
        rpc_traits_sect.definition = fmt::format(
            rpc_trait_format, absolute_name(), serv_full_name, attr->proc_name);
        rpc_traits_sect.add_dependency(def_key());
        rpc_traits_sect.add_dependency({serv_handle, section_type::definition});
        res.add(std::move(rpc_traits_sect));
    }

    return res;
}


} // namespace lidl::cpp