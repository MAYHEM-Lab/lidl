#include "cppgen.hpp"

#include "lidl/enumeration.hpp"
#include "lidl/union.hpp"

#include <algorithm>
#include <fmt/core.h>
#include <fmt/format.h>
#include <lidl/basic.hpp>
#include <lidl/module.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lidl {
void reference_type_pass(module& m);
void union_enum_pass(module& m);
} // namespace lidl

namespace lidl::cpp {
namespace {
bool is_anonymous(const module& mod, symbol t) {
    return !recursive_definition_lookup(*mod.symbols, t);
}

void generate_raw_struct_field(std::string_view name,
                               std::string_view type_name,
                               std::ostream& str) {
    str << fmt::format("{} {};\n", type_name, name);
}

struct sections {
    std::unordered_map<std::string, std::string> m_body;
    std::vector<std::string*> order;

    void add_body(std::string_view name, std::string content) {
        auto [it, inserted] = m_body.emplace(std::string(name), std::move(content));
        if (inserted) {
            order.emplace_back(&it->second);
        }
    }

    std::string merge_body() const {
        std::string res;
        for (auto ptr : order) {
            res += *ptr + "\n";
        }
        return res;
    }

    std::stringstream traits, std_traits;

    void merge_before(sections rhs) {
        m_body.merge(rhs.m_body);
        rhs.order.reserve(order.size() + rhs.order.size());
        std::copy(order.begin(), order.end(), std::back_inserter(rhs.order));
        order = std::move(rhs.order);

        traits << rhs.traits.str();
        std_traits << rhs.std_traits.str();
    }
};

template<class Type>
class generator_base {
public:
    generator_base(const module& mod,
                   std::string_view name,
                   std::string_view ctor_name,
                   const Type& elem)
        : m_module{&mod}
        , m_name{name}
        , m_ctor_name(ctor_name)
        , m_elem{&elem} {
    }

    generator_base(const module& mod, std::string_view name, const Type& elem)
        : generator_base(mod, name, name, elem) {
    }

protected:
    const module& mod() {
        return *m_module;
    }

    const Type& get() {
        return *m_elem;
    }

    std::string_view name() {
        return m_name;
    }

    std::string_view ctor_name() {
        return m_ctor_name;
    }

    sections m_sections;

private:
    const module* m_module;
    std::string m_name;
    std::string m_ctor_name;
    const Type* m_elem;
};

class raw_struct_gen : generator_base<structure> {
public:
    using generator_base::generator_base;

    auto& generate() {
        for (auto& [name, member] : get().members) {
            generate_raw_struct_field(
                name, get_identifier(mod(), member.type_), raw_sections.pub);
        }

        generate_raw_constructor();

        constexpr auto format = R"__(struct {} {{
            {}
            {}
        }};)__";

        m_sections.add_body(
            name(),
            fmt::format(format, name(), raw_sections.ctor.str(), raw_sections.pub.str()));
        return m_sections;
    }

private:
    void generate_raw_constructor() {
        if (get().members.empty()) {
            return;
        }
        std::vector<std::string> arg_names;
        std::vector<std::string> initializer_list;

        for (auto& [member_name, member] : get().members) {
            auto member_type = get_type(mod(), member.type_);
            auto identifier = get_user_identifier(mod(), member.type_);

            if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
                arg_names.push_back(
                    fmt::format("const {}& p_{}", identifier, member_name));
                initializer_list.push_back(fmt::format("{0}(p_{0})", member_name));
                continue;
            }
            arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
            initializer_list.push_back(fmt::format(
                "{0}(p_{0} ? decltype({0}){{*p_{0}}} : decltype({0}){{nullptr}})",
                member_name));
        }

        raw_sections.ctor << fmt::format("{}({}) : {} {{}}",
                                         ctor_name(),
                                         fmt::join(arg_names, ", "),
                                         fmt::join(initializer_list, ", "));
    }
    struct {
        std::stringstream pub, ctor;
    } raw_sections;
};

class struct_body_gen {
public:
    struct_body_gen(const module& mod, std::string_view name, const structure& str)
        : m_module{&mod}
        , m_name{name}
        , m_struct{&str} {
    }

    void generate(std::ostream& stream) {
        for (auto& [name, member] : str().members) {
            generate_struct_field(name, member);
        }

        raw_struct_gen raw_gen(mod(), "raw_t", str());
        auto& res = raw_gen.generate();
        m_sections.priv << res.merge_body() << '\n';

        generate_constructor();

        std::string tags;
        if (auto attr =
                str().attributes.get<procedure_params_attribute>("procedure_params")) {
            tags = fmt::format("static constexpr auto params_for = &{}::{};",
                               attr->serv_name,
                               attr->proc_name);
        }

        constexpr auto format = R"__(
        public:
            {}
            {}
        private:
            {}
            raw_t raw;
        public:
            {}
        )__";

        stream << fmt::format(format,
                              m_sections.ctor.str(),
                              m_sections.pub.str(),
                              m_sections.priv.str(),
                              tags)
               << '\n';
    }

private:
    void generate_constructor() {
        std::vector<std::string> arg_names;
        std::vector<std::string> initializer_list;

        for (auto& [member_name, member] : str().members) {
            auto member_type = get_type(mod(), member.type_);
            auto identifier = get_user_identifier(mod(), member.type_);
            initializer_list.push_back(fmt::format("p_{}", member_name));

            if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
                arg_names.push_back(
                    fmt::format("const {}& p_{}", identifier, member_name));
                continue;
            }
            arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
        }

        m_sections.ctor << fmt::format("{}({}) : raw{{{}}} {{}}",
                                       m_name,
                                       fmt::join(arg_names, ", "),
                                       fmt::join(initializer_list, ", "));
    }

    void generate_struct_field(std::string_view name, const member& mem) {
        m_sections.pub << generate_getter(name, mem, true) << '\n';
        m_sections.pub << generate_getter(name, mem, false) << '\n';
    }

    std::string
    generate_getter(std::string_view member_name, const member& mem, bool is_const) {
        auto member_type = get_type(mod(), mem.type_);
        if (!member_type->is_reference_type(mod())) {
            auto type_name = get_identifier(mod(), mem.type_);
            constexpr auto format = R"__({}& {}() {{ return raw.{}; }})__";
            constexpr auto const_format =
                R"__(const {}& {}() const {{ return raw.{}; }})__";
            return fmt::format(
                is_const ? const_format : format, type_name, member_name, member_name);
        } else {
            // need to dereference before return
            auto& base = std::get<name>(mem.type_.args[0]);
            auto identifier = get_identifier(mod(), base);
            if (!mem.is_nullable()) {
                constexpr auto format =
                    R"__({}& {}() {{ return raw.{}.unsafe().get(); }})__";
                constexpr auto const_format =
                    R"__(const {}& {}() const {{ return raw.{}.unsafe().get(); }})__";
                return fmt::format(is_const ? const_format : format,
                                   identifier,
                                   member_name,
                                   member_name);
            }
        }
        return "";
    }

    const module& mod() {
        return *m_module;
    }

    const structure& str() {
        return *m_struct;
    }

    struct {
        std::stringstream pub, priv, ctor;
        std::stringstream traits;
    } m_sections;

    const module* m_module;
    std::string_view m_name;
    const structure* m_struct;
};

struct enum_gen : generator_base<enumeration> {
    using generator_base::generator_base;

    sections generate() {
        do_generate();
        return std::move(m_sections);
    }

private:
    void do_generate() {
        std::stringstream pub;
        for (auto& [name, member] : get().members) {
            pub << fmt::format("{} = {},\n", name, member.value);
        }

        constexpr auto format = R"__(enum class {} : {} {{
            {}
        }};)__";

        m_sections.add_body(
            name(),
            fmt::format(
                format, name(), get_identifier(mod(), get().underlying_type), pub.str()));
        generate_traits();
    }

    void generate_traits() {
        std::vector<std::string> names;
        for (auto& [name, val] : get().members) {
            names.emplace_back(fmt::format("\"{}\"", name));
        }

        constexpr auto format = R"__(
            template <>
            struct enum_traits<{0}> {{
                static constexpr inline std::array<const char*, {1}> names {{{2}}};
                static constexpr const char* name = "{0}";
            }};
        )__";

        m_sections.traits << fmt::format(
            format, name(), names.size(), fmt::join(names, ", "));
    }
};

struct union_gen : generator_base<union_type> {
    using generator_base::generator_base;

    auto generate() {
        do_generate();
        return std::move(m_sections);
    }

private:
    void do_generate() {
        std::stringstream pub;
        for (auto& [name, member] : get().members) {
            generate_raw_struct_field(
                "m_" + name, get_identifier(mod(), member.type_), pub);
        }

        std::stringstream ctor;
        auto enum_name = fmt::format("{}_alternatives", ctor_name());
        int member_index = 0;
        for (auto& [member_name, member] : get().members) {
            std::string arg_names;
            std::string initializer_list;
            const auto enum_val = get().get_enum().find_by_value(member_index++)->first;

            auto member_type = get_type(mod(), member.type_);
            auto identifier = get_user_identifier(mod(), member.type_);
            if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
                arg_names = fmt::format("const {}& p_{}", identifier, member_name);
                initializer_list = fmt::format("m_{0}(p_{0})", member_name);
            } else {
                arg_names = fmt::format("const {}* p_{}", identifier, member_name);
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

        enum_gen en(mod(), enum_name, get().get_enum());

        m_sections.merge_before(en.generate());
        m_sections.add_body(name(),
                            fmt::format(format,
                                        name(),
                                        enum_name,
                                        ctor.str(),
                                        pub.str(),
                                        visitor,
                                        accessors.str()));
        generate_traits();
    }

    void generate_traits() {
        std::vector<std::string> members;
        for (auto& [memname, member] : get().members) {
            members.push_back(fmt::format(
                "member_info{{\"{1}\", &{0}::{1}, &{0}::{1}}}", name(), memname));
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
            auto identifier = get_user_identifier(mod(), member.type_);
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

            ctors.push_back(fmt::format(
                format, ctor_name(), member_name, arg_names, initializer_list));
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

        m_sections.traits << fmt::format(format,
                                         name(),
                                         fmt::join(names, ", "),
                                         fmt::join(ctors, "\n"),
                                         fmt::join(ctor_names, ", "),
                                         fmt::join(members, ", "));
    }

    std::string
    generate_getter(std::string_view member_name, const member& mem, bool is_const) {
        auto member_type = get_type(mod(), mem.type_);
        if (!member_type->is_reference_type(mod())) {
            auto type_name = get_identifier(mod(), mem.type_);
            constexpr auto format = R"__({}& {}() {{ return m_{}; }})__";
            constexpr auto const_format =
                R"__(const {}& {}() const {{ return m_{}; }})__";
            return fmt::format(
                is_const ? const_format : format, type_name, member_name, member_name);
        } else {
            // need to dereference before return
            auto& base = std::get<lidl::name>(mem.type_.args[0]);
            auto identifier = get_identifier(mod(), base);
            if (!mem.is_nullable()) {
                constexpr auto format =
                    R"__({}& {}() {{ return m_{}.unsafe().get(); }})__";
                constexpr auto const_format =
                    R"__(const {}& {}() const {{ return m_{}.unsafe().get(); }})__";
                return fmt::format(is_const ? const_format : format,
                                   identifier,
                                   member_name,
                                   member_name);
            }
        }
        return "";
    }
};

struct struct_gen : generator_base<structure> {
    using generator_base::generator_base;

    auto generate() {
        do_generate();
        return std::move(m_sections);
    }

private:
    void do_generate() {
        constexpr auto format = R"__(class {0} : public ::lidl::struct_base<{0}> {{
            {1}
        }};)__";

        std::stringstream body;
        struct_body_gen(mod(), name(), get()).generate(body);

        m_sections.add_body(name(), fmt::format(format, name(), body.str()));
        generate_traits();
    }

    void generate_traits() {
        std::vector<std::string> member_types;
        for (auto& [memname, member] : get().members) {
            auto identifier = get_user_identifier(mod(), member.type_);
            member_types.push_back(identifier);
        }

        std::vector<std::string> members;
        for (auto& [memname, member] : get().members) {
            members.push_back(fmt::format(
                "member_info{{\"{1}\", &{0}::{1}, &{0}::{1}}}", name(), memname));
        }

        std::vector<std::string> ctor_types{""};
        std::vector<std::string> ctor_args{""};
        for (auto& [member_name, member] : get().members) {
            auto member_type = get_type(mod(), member.type_);
            auto identifier = get_user_identifier(mod(), member.type_);
            ctor_args.push_back(fmt::format("p_{}", member_name));

            if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
                ctor_types.push_back(
                    fmt::format("const {}& p_{}", identifier, member_name));
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

        m_sections.traits << fmt::format(format,
                                         name(),
                                         fmt::join(members, ", "),
                                         fmt::join(ctor_types, ", "),
                                         fmt::join(ctor_args, ", "),
                                         fmt::join(member_types, ", "));

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

        m_sections.std_traits << fmt::format(std_format, name(), members.size());
        m_sections.std_traits << fmt::format("{}", fmt::join(tuple_elements, "\n"));
    }
};

struct generic_gen : generator_base<generic_instantiation> {
    using generator_base::generator_base;

    auto generate() {
        m_sections.merge_before(do_generate());
        return std::move(m_sections);
    }

private:
    std::string full_name() {
        return get_identifier(mod(), get().get_name());
    }

    sections do_generate(const generic_structure& str) {
        module tmpmod;
        tmpmod.symbols = mod().symbols->add_child_scope();
        tmpmod.structs.emplace_back(str.instantiate(mod(), get()));
        reference_type_pass(tmpmod);

        struct_gen gen(tmpmod, full_name(), name(), tmpmod.structs.back());
        return std::move(gen.generate());
    }

    sections do_generate(const generic_union& u) {
        module tmpmod;
        tmpmod.symbols = mod().symbols->add_child_scope();
        tmpmod.unions.emplace_back(u.instantiate(mod(), get()));
        reference_type_pass(tmpmod);
        union_enum_pass(tmpmod);

        union_gen gen(tmpmod, full_name(), name(), tmpmod.unions.back());

        return std::move(gen.generate());
    }

    sections do_generate() {
        if (auto genstr = dynamic_cast<const generic_structure*>(&get().generic_type())) {
            auto res = do_generate(*genstr);
            res.m_body[full_name()] = "template <>\n" + res.m_body[full_name()];
            return res;
        }
        if (auto genun = dynamic_cast<const generic_union*>(&get().generic_type())) {
            auto res = do_generate(*genun);
            res.m_body[full_name()] = "template <>\n" + res.m_body[full_name()];
            return res;
        }
        return sections();
    }
};

void declare_template(const module& mod,
                      std::string_view generic_name,
                      const generic& generic,
                      std::ostream& str) {
    std::vector<std::string> params;
    for (auto& [name, param] : generic.declaration) {
        std::string type_name;
        if (dynamic_cast<type_parameter*>(param.get())) {
            type_name = "typename";
        } else {
            type_name = "int32_t";
        }
        params.emplace_back(fmt::format("{} {}", type_name, name));
    }

    str << fmt::format(
        "template <{}>\nclass {};\n", fmt::join(params, ", "), generic_name);
}

struct cppgen {
    explicit cppgen(const module& mod)
        : m_module(&mod) {
    }

    void generate(std::ostream& str) {
        std::stringstream forward_decls;
        forward_decls << fmt::format("struct {};\n", name());

        for (auto& generic : mod().generic_unions) {
            if (is_anonymous(mod(), &generic)) {
                continue;
            }

            auto name = nameof(*mod().symbols->definition_lookup(&generic));
            declare_template(mod(), name, generic, forward_decls);
        }

        for (auto& generic : mod().generic_structs) {
            if (is_anonymous(mod(), &generic)) {
                continue;
            }

            auto name = nameof(*mod().symbols->definition_lookup(&generic));
            declare_template(mod(), name, generic, forward_decls);
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(mod(), &s)) {
                continue;
            }

            auto name = nameof(*mod().symbols->definition_lookup(&s));
            forward_decls << "class " << name << ";\n";
        }

        for (auto& s : mod().unions) {
            if (is_anonymous(mod(), &s)) {
                continue;
            }
            auto name = nameof(*mod().symbols->definition_lookup(&s));
            forward_decls << "class " << name << ";\n";
        }

        for (auto& e : m_module->enums) {
            if (is_anonymous(*m_module, &e)) {
                continue;
            }

            auto name = nameof(*m_module->symbols->definition_lookup(&e));
            enum_gen generator(mod(), name, e);
            m_sections.merge_before(generator.generate());
        }

        for (auto& ins : mod().instantiations) {
            auto name = nameof(*mod().symbols->definition_lookup(&ins.generic_type()));
            auto generator = generic_gen(mod(), name, ins);
            m_sections.merge_before(generator.generate());
        }

        for (auto& u : mod().unions) {
            auto name = nameof(*mod().symbols->definition_lookup(&u));
            auto generator = union_gen(mod(), name, u);
            m_sections.merge_before(generator.generate());
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(*m_module, &s)) {
                continue;
            }
            auto name = nameof(*mod().symbols->definition_lookup(&s));
            auto generator = struct_gen(mod(), name, s);
            m_sections.merge_before(generator.generate());
        }

        generate_module_traits();
        if (!mod().name_space.empty()) {
            str << fmt::format("namespace {} {{\n", mod().name_space);
        }
        str << forward_decls.str() << '\n';
        str << m_sections.merge_body() << '\n';
        if (!mod().name_space.empty()) {
            str << "}\n";
        }
        str << "namespace lidl {\n" << m_sections.traits.str() << "\n}\n";
        str << "namespace std {\n" << m_sections.std_traits.str() << "\n}\n";
    }

private:
    void generate_module_traits() {
        static constexpr auto format = R"__(
            template<>
            struct module_traits<{0}> {{
                static constexpr const char* name = "{0}";
                using symbols = meta::list<{1}>;
            }};
        )__";

        std::vector<std::string> symbol_names;
        for (auto& s : m_sections.m_body) {
            symbol_names.emplace_back(s.first);
        }

        m_sections.traits << fmt::format(format, name(), fmt::join(symbol_names, ", "));
    }

    sections m_sections;

    std::string m_name = "mod";

    std::string_view name() {
        return m_name;
    }

    const module& mod() {
        return *m_module;
    }

    const module* m_module;
};
} // namespace

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

void generate_procedure(const module& mod,
                        std::string_view proc_name,
                        const procedure& proc,
                        std::ostream& str) {
    constexpr auto decl_format = "    virtual {} {}({}) = 0;";

    std::vector<std::string> params;
    for (auto& [param_name, param] : proc.parameters) {
        if (get_type(mod, param)->is_reference_type(mod)) {
            if (param.args.empty()) {
                throw std::runtime_error(
                    fmt::format("Not a pointer: {}", get_identifier(mod, param)));
            }
            auto identifier = get_identifier(mod, std::get<name>(param.args.at(0)));
            params.emplace_back(fmt::format("const {}& {}", identifier, param_name));
        } else {
            auto identifier = get_identifier(mod, param);
            params.emplace_back(fmt::format("const {}& {}", identifier, param_name));
        }
    }

    std::string ret_type_name;
    auto ret_type = get_type(mod, proc.return_types.at(0));
    ret_type_name = get_user_identifier(mod, proc.return_types.at(0));
    if (ret_type->is_reference_type(mod)) {
        ret_type_name = fmt::format("const {}&", ret_type_name);
        params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
    }

    str << fmt::format(decl_format, ret_type_name, proc_name, fmt::join(params, ", "));
}

void generate_service_descriptor(const module& mod,
                                 std::string_view service_name,
                                 const service& service,
                                 std::ostream& str) {
    std::vector<std::string> tuple_types(service.procedures.size());
    std::transform(service.procedures.begin(),
                   service.procedures.end(),
                   tuple_types.begin(),
                   [&](auto& proc) {
                       auto param_name =
                           get_identifier(mod,
                                          name{*mod.symbols->definition_lookup(
                                              std::get<procedure>(proc).params_struct)});
                       auto res_name =
                           get_identifier(mod,
                                          name{*mod.symbols->definition_lookup(
                                              std::get<procedure>(proc).results_struct)});
                       return fmt::format(
                           "::lidl::procedure_descriptor<&{0}::{1}, {2}, {3}>{{\"{1}\"}}",
                           service_name,
                           std::get<std::string>(proc),
                           param_name,
                           res_name);
                   });
    str << fmt::format("template <> class service_descriptor<{}> {{\npublic:\n",
                       service_name);
    str << fmt::format("static constexpr inline auto procedures = std::make_tuple({});\n",
                       fmt::join(tuple_types, ", "));
    str << fmt::format("static constexpr inline std::string_view name = \"{}\";\n",
                       service_name);
    str << fmt::format(
        "using params_union = {};",
        get_identifier(
            mod, name{*mod.symbols->definition_lookup(service.procedure_params_union)}));
    str << fmt::format(
        "using results_union = {};",
        get_identifier(
            mod, name{*mod.symbols->definition_lookup(service.procedure_results_union)}));
    str << "};";
}

void generate_service(const module& mod,
                      std::string_view name,
                      const service& service,
                      std::ostream& str) {
    std::vector<std::string> inheritance;
    for (auto& base : service.extends) {
        inheritance.emplace_back(nameof(base.base));
    }

    str << fmt::format("class {}{} {{\npublic:\n", name, inheritance.empty() ? "" : fmt::format(" : public {}", fmt::join(inheritance, ", public ")));
    for (auto& [name, proc] : service.procedures) {
        generate_procedure(mod, name, proc, str);
        str << '\n';
    }
    str << fmt::format("    virtual ~{}() = default;\n", name);
    str << "};";
    str << '\n';
}
} // namespace lidl::cpp

namespace lidl {
void generate(const module& mod, std::ostream& str) {
    for (auto& [_, child] : mod.children) {
        generate(child, str);
    }

    using namespace cpp;
    str << "#pragma once\n\n#include <lidl/lidl.hpp>\n";
    if (!mod.services.empty()) {
        str << "#include <lidl/service.hpp>\n";
    }
    str << '\n';

    if (!mod.name_space.empty()) {
        str << fmt::format("namespace {} {{\n", mod.name_space);
    }
    for (auto& service : mod.services) {
        Expects(!is_anonymous(mod, &service));
        auto name = nameof(*mod.symbols->definition_lookup(&service));
        generate_service(mod, name, service, str);
        str << '\n';
    }
    if (!mod.name_space.empty()) {
        str << "}\n";
    }

    cppgen gen(mod);
    gen.generate(str);

    for (auto& service : mod.services) {
        auto name = nameof(*mod.symbols->definition_lookup(&service));
        str << "namespace lidl {\n";
        generate_service_descriptor(mod, name, service, str);
        str << '\n';
        str << "}\n";
    }
}
} // namespace lidl