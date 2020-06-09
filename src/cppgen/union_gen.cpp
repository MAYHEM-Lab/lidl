//
// Created by fatih on 6/3/20.
//

#include "union_gen.hpp"

#include "cppgen.hpp"
#include "enum_gen.hpp"
#include "struct_bodygen.hpp"

namespace lidl::cpp {
std::string union_gen::generate_getter(std::string_view member_name,
                                       const member& mem,
                                       bool is_const) {
    auto member_type = get_type(mod(), mem.type_);
    if (!member_type->is_reference_type(mod())) {
        auto type_name              = get_identifier(mod(), mem.type_);
        constexpr auto format       = R"__({}& {}() {{ return m_{}; }})__";
        constexpr auto const_format = R"__(const {}& {}() const {{ return m_{}; }})__";
        return fmt::format(
            is_const ? const_format : format, type_name, member_name, member_name);
    } else {
        // need to dereference before return
        auto& base      = std::get<lidl::name>(mem.type_.args[0]);
        auto identifier = get_identifier(mod(), base);
        if (!mem.is_nullable()) {
            constexpr auto format = R"__({}& {}() {{ return m_{}.unsafe().get(); }})__";
            constexpr auto const_format =
                R"__(const {}& {}() const {{ return m_{}.unsafe().get(); }})__";
            return fmt::format(
                is_const ? const_format : format, identifier, member_name, member_name);
        }
    }
    return "";
}

sections union_gen::do_generate() {
    std::stringstream pub;
    for (auto& [name, member] : get().members) {
        raw_struct_gen::generate_field(
            "m_" + name, get_identifier(mod(), member.type_), pub);
    }

    std::stringstream ctor;
    auto enum_name   = fmt::format("{}_alternatives", ctor_name());
    int member_index = 0;
    for (auto& [member_name, member] : get().members) {
        std::string arg_names;
        std::string initializer_list;
        const auto enum_val = get().get_enum().find_by_value(member_index++)->first;

        auto member_type = get_type(mod(), member.type_);
        auto identifier  = get_user_identifier(mod(), member.type_);
        if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
            arg_names        = fmt::format("const {}& p_{}", identifier, member_name);
            initializer_list = fmt::format("m_{0}(p_{0})", member_name);
        } else {
            arg_names        = fmt::format("const {}* p_{}", identifier, member_name);
            initializer_list = fmt::format(
                "m_{0}(p_{0} ? decltype({0}){{*p_{0}}} : decltype({0}){{nullptr}})",
                member_name);
        }

        ctor << fmt::format("{}({}) : {}, discriminator{{{}::{}}} {{}}\n",
                            ctor_name(),
                            arg_names,
                            initializer_list,
                            enum_name,
                            enum_val);
    }

    constexpr auto case_format = R"__(
            case {0}::{1}: return fn(val.{1}());
        )__";

    std::vector<std::string> cases;
    for (auto& [name, mem] : get().members) {
        cases.push_back(fmt::format(case_format, enum_name, name));
    }

    constexpr auto visitor_format = R"__(
            template <class FunT>
            friend decltype(auto) visit(const FunT& fn, const {0}& val) {{
                switch (val.alternative()) {{
                    {1}
                }}
            }}
        )__";

    auto visitor = fmt::format(visitor_format, name(), fmt::join(cases, "\n"));

    constexpr auto format = R"__(
        class {0} : ::lidl::union_base<{0}> {{
        public:
            {1} alternative() const noexcept {{
                return discriminator;
            }}

            {2}

            {5}

        private:
            {1} discriminator;
            union {{
                {3}
            }};

            {4}
        }};)__";

    std::stringstream accessors;
    for (auto& [mem_name, mem] : get().members) {
        accessors << generate_getter(mem_name, mem, true) << '\n';
        accessors << generate_getter(mem_name, mem, false) << '\n';
    }

    enum_gen en(mod(), {}, enum_name, get().get_enum());

    section s;
    s.key        = def_key();
    s.definition = fmt::format(
        format, name(), enum_name, ctor.str(), pub.str(), visitor, accessors.str());
    s.add_dependency({enum_name, section_type::definition});

    // Member types must be defined before us
    for (auto& [name, member] : get().members) {
        auto deps = def_keys_from_name(mod(), member.type_);
        for (auto& key : deps) {
            s.add_dependency(key);
        }
    }

    auto result = generate_traits();

    result.merge_before(en.generate());
    result.add(std::move(s));

    return result;
}

sections union_gen::generate_traits() {
    std::vector<std::string> members;
    for (auto& [memname, member] : get().members) {
        members.push_back(
            fmt::format("member_info{{\"{1}\", &{0}::{1}, &{0}::{1}}}", name(), memname));
    }

    std::vector<std::string> names;
    for (auto& [name, member] : get().members) {
        names.emplace_back(get_user_identifier(mod(), member.type_));
    }

    std::vector<std::string> ctors;
    std::vector<std::string> ctor_names;
    int member_index = 0;
    for (auto& [member_name, member] : get().members) {
        std::string arg_names;
        std::string initializer_list;
        const auto enum_val = get().get_enum().find_by_value(member_index++)->first;

        auto member_type = get_type(mod(), member.type_);
        auto identifier  = get_user_identifier(mod(), member.type_);
        if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
            arg_names = fmt::format("const {}& p_{}", identifier, member_name);
        } else {
            arg_names = fmt::format("const {}* p_{}", identifier, member_name);
        }
        initializer_list = fmt::format("p_{0}", member_name);

        static constexpr auto format =
            R"__(static {0}& ctor_{1}(::lidl::message_builder& builder, {2}){{
                return ::lidl::create<{0}>(builder, {3});
            }})__";

        ctors.push_back(
            fmt::format(format, ctor_name(), member_name, arg_names, initializer_list));
        ctor_names.push_back("&union_traits::ctor_" + member_name);
    }

    constexpr auto format = R"__(
            template <>
            struct union_traits<{0}> {{
                static constexpr auto members = std::make_tuple({4});
                using types = meta::list<{1}>;
                static constexpr const char* name = "{0}";
                {2}
                static constexpr auto ctors = std::make_tuple({3});
            }};
        )__";

    section trait_sect;
    trait_sect.name_space = "lidl";
    trait_sect.definition = fmt::format(format,
                                        name(),
                                        fmt::join(names, ", "),
                                        fmt::join(ctors, "\n"),
                                        fmt::join(ctor_names, ", "),
                                        fmt::join(members, ", "));
    trait_sect.add_dependency(def_key());

    auto res = sections{{std::move(trait_sect)}};

    if (auto attr =
            get().attributes.get<service_call_union_attribute>("service_call_union")) {
        constexpr auto rpc_trait_format =
            R"__(template <> struct service_call_union<{}> : {} {{
        }};)__";

        section rpc_traits_sect;
        rpc_traits_sect.name_space = "lidl";
        rpc_traits_sect.definition =
            fmt::format(rpc_trait_format, attr->serv_name, name());
        rpc_traits_sect.add_dependency(def_key());
        rpc_traits_sect.add_dependency({attr->serv_name, section_type::definition});
        res.add(std::move(rpc_traits_sect));
    }

    if (auto attr =
        get().attributes.get<service_return_union_attribute>("service_return_union")) {
        constexpr auto rpc_trait_format =
            R"__(template <> struct service_return_union<{}> : {} {{
        }};)__";

        section rpc_traits_sect;
        rpc_traits_sect.name_space = "lidl";
        rpc_traits_sect.definition =
            fmt::format(rpc_trait_format, attr->serv_name, name());
        rpc_traits_sect.add_dependency(def_key());
        rpc_traits_sect.add_dependency({attr->serv_name, section_type::definition});
        res.add(std::move(rpc_traits_sect));
    }

    return res;
}
} // namespace lidl::cpp