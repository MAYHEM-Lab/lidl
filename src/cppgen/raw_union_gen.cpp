//
// Created by fatih on 6/3/20.
//

#include "raw_union_gen.hpp"

#include "cppgen.hpp"
#include "enum_gen.hpp"
#include "struct_bodygen.hpp"

namespace lidl::cpp {
using codegen::section;
using codegen::sections;

std::string raw_union_gen::generate_getter(std::string_view member_name,
                                           const member& mem,
                                           bool is_const) {
    assert(!mem.is_nullable());

    auto member_type_name = get_wire_type_name(mod(), mem.type_);

    constexpr auto format =
        R"__({0}& {1}() {{
            return m_{1};
        }})__";
    constexpr auto const_format =
        R"__(const {0}& {1}() const {{
            return m_{1};
        }})__";

    auto type_name = get_identifier(mod(), deref_ptr(mod(), member_type_name));
    return fmt::format(is_const ? const_format : format, type_name, member_name);
}

sections raw_union_gen::do_generate() {
    std::vector<std::string> members;
    for (auto& [name, member] : get().all_members()) {
        members.push_back(raw_struct_gen::generate_field(
            fmt::format("m_{}", name), get_identifier(mod(), member->type_)));
    }

    std::vector<std::string> ctors;
    int member_index = 0;
    for (auto& [member_name, member] : get().all_members()) {
        std::string arg_names;
        std::string initializer_list;

        auto identifier  = get_user_identifier(mod(), member->type_);
        if (!member->is_nullable()) {
            arg_names        = fmt::format("const {}& p_{}", identifier, member_name);
            initializer_list = fmt::format("m_{0}(p_{0})", member_name);
        } else {
            arg_names        = fmt::format("const {}* p_{}", identifier, member_name);
            initializer_list = fmt::format(
                "m_{0}(p_{0} ? decltype({0}){{*p_{0}}} : decltype({0}){{nullptr}})",
                member_name);
        }

        ctors.push_back(
            fmt::format("{}({}) : {} {{}}", ctor_name(), arg_names, initializer_list));
    }

    constexpr auto format = R"__(class {0} : ::lidl::union_base<{0}> {{
        public:
            {1}

            {3}

        private:
            union {{
                {2}
            }};
        }};)__";

    std::vector<std::string> accessors;
    for (auto& [mem_name, mem] : get().all_members()) {
        accessors.push_back(generate_getter(mem_name, *mem, true));
        accessors.push_back(generate_getter(mem_name, *mem, false));
    }

    section s;
    s.add_key(def_key());
    s.definition = fmt::format(format,
                               name(),
                               fmt::join(ctors, "\n"),
                               fmt::join(members, "\n"),
                               fmt::join(accessors, "\n"));

    // Member types must be defined before us
    for (auto& [name, member] : get().all_members()) {
        auto deps = codegen::def_keys_from_name(mod(), member->type_);
        for (auto& key : deps) {
            s.add_dependency(key);
        }
    }

    auto result = generate_traits();

    result.add(std::move(s));
    return result;
}

sections raw_union_gen::generate_traits() {
    std::vector<std::string> members;
    for (auto& [memname, member] : get().all_members()) {
        members.push_back(fmt::format(
            "member_info{{\"{1}\", &{0}::{1}, &{0}::{1}}}", absolute_name(), memname));
    }

    std::vector<std::string> names;
    for (auto& [name, member] : get().all_members()) {
        names.emplace_back(get_user_identifier(mod(), member->type_));
    }

    std::vector<std::string> ctors;
    std::vector<std::string> ctor_names;
    int member_index = 0;
    for (auto& [member_name, member] : get().all_members()) {
        std::string arg_names;
        std::string initializer_list;
        const auto enum_val = get().get_enum(mod()).find_by_value(member_index++)->first;

        auto identifier  = get_user_identifier(mod(), member->type_);
        if (!member->is_nullable()) {
            arg_names = fmt::format("const {}& p_{}", identifier, member_name);
        } else {
            arg_names = fmt::format("const {}* p_{}", identifier, member_name);
        }
        initializer_list = fmt::format("p_{0}", member_name);

        static constexpr auto format =
            R"__(static {0}& ctor_{1}(::lidl::message_builder& builder, {2}){{
                return ::lidl::create<{0}>(builder, {3});
            }})__";

        ctors.emplace_back(fmt::format(
            format, absolute_name(), member_name, arg_names, initializer_list));
        ctor_names.emplace_back("&union_traits::ctor_" + std::string(member_name));
    }

    constexpr auto format = R"__(template <>
            struct union_traits<{0}> {{
                static constexpr auto members = std::make_tuple({4});
                using types = meta::list<{1}>;
                static constexpr const char* name = "{0}";
                {2}
                static constexpr auto ctors = std::make_tuple({3});
            }};)__";

    section trait_sect;
    trait_sect.definition = fmt::format(format,
                                        absolute_name(),
                                        fmt::join(names, ", "),
                                        fmt::join(ctors, "\n"),
                                        fmt::join(ctor_names, ", "),
                                        fmt::join(members, ", "));
    trait_sect.add_key({symbol(), section_type::lidl_traits});
    trait_sect.add_dependency(def_key());

    auto res = sections{{std::move(trait_sect)}};

    return res;
}
} // namespace lidl::cpp
