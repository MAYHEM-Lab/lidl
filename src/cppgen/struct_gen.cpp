//
// Created by fatih on 6/3/20.
//

#include "struct_gen.hpp"

#include "cppgen.hpp"
#include "struct_bodygen.hpp"

namespace lidl::cpp {
sections struct_gen::do_generate() {
    constexpr auto format = R"__(class {0} : public ::lidl::struct_base<{0}> {{
            {1}
        }};)__";

    std::stringstream body;
    struct_body_gen(mod(), name(), get()).generate(body);

    section s;
    s.name = fmt::format("{}_def", name());
    s.definition = fmt::format(format, name(), body.str());

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
        members.push_back(
            fmt::format("member_info{{\"{1}\", &{0}::{1}, &{0}::{1}}}", name(), memname));
    }

    std::vector<std::string> ctor_types{""};
    std::vector<std::string> ctor_args{""};
    for (auto& [member_name, member] : get().members) {
        auto member_type = get_type(mod(), member.type_);
        auto identifier = get_user_identifier(mod(), member.type_);
        ctor_args.push_back(fmt::format("p_{}", member_name));

        if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
            ctor_types.push_back(fmt::format("const {}& p_{}", identifier, member_name));
            continue;
        }
        ctor_types.push_back(fmt::format("const {}* p_{}", identifier, member_name));
    }

    constexpr auto format = R"__(
            template <>
            struct struct_traits<{0}> {{
                using raw_members = meta::list<{4}>;
                static constexpr auto members = std::make_tuple({1});
                static constexpr auto arity = std::tuple_size_v<decltype(members)>;
                static constexpr const char* name = "{0}";
                static {0}& ctor(::lidl::message_builder& builder{2}) {{
                    return ::lidl::create<{0}>(builder{3});
                }}
            }};
        )__";

    section trait_sect;
    trait_sect.name = fmt::format("{}_lidl_trait", name());
    trait_sect.name_space = "lidl";
    trait_sect.definition = fmt::format(format,
                                        name(),
                                        fmt::join(members, ", "),
                                        fmt::join(ctor_types, ", "),
                                        fmt::join(ctor_args, ", "),
                                        fmt::join(member_types, ", "));
    trait_sect.add_dependency(fmt::format("{}_def", name()));

    constexpr auto std_format = R"__(
            template <>
            struct tuple_size<{}> : std::integral_constant<std::size_t, {}> {{
            }};
        )__";

    std::vector<std::string> tuple_elements;
    int idx = 0;
    for (auto& [memname, member] : get().members) {
        tuple_elements.push_back(fmt::format(
            "template <> struct tuple_element<{}, {}> {{ using type = {}; }};",
            idx++,
            name(),
            get_user_identifier(mod(), member.type_)));
    }


    section std_trait_sect;
    std_trait_sect.name = fmt::format("{}_lidl_trait", name());
    std_trait_sect.name_space = "std";
    std_trait_sect.definition = fmt::format(format,
                                            name(),
                                            fmt::join(members, ", "),
                                            fmt::join(ctor_types, ", "),
                                            fmt::join(ctor_args, ", "),
                                            fmt::join(member_types, ", ")) +
                                fmt::format(std_format, name(), members.size()) +
                                fmt::format("{}", fmt::join(tuple_elements, "\n"));
    std_trait_sect.add_dependency(fmt::format("{}_def", name()));

    return sections{{std::move(trait_sect), std::move(std_trait_sect)}};
}


} // namespace lidl::cpp