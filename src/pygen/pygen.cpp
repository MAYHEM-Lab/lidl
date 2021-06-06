#include "get_identifier.hpp"

#include <codegen.hpp>
#include <generator_base.hpp>
#include <lidl/algorithm.hpp>

namespace lidl::py {

std::string generate_member(const module& mod,
                            std::string_view name,
                            int offset,
                            const member& mem) {
    constexpr auto member_format = R"__({{
    "type": {0},
    "offset": {1}
}})__";

    auto member_type_name = get_wire_type_name(mod, mem.type_);

    auto type_name =
        codegen::detail::current_backend->get_identifier(mod, member_type_name);

    return fmt::format("\"{}\": {}", name, fmt::format(member_format, type_name, offset));
}

std::string reindent(std::string val, int indent) {
    auto lines      = lidl::split(val, "\n");
    std::string res = "";
    for (auto& line : lines) {
        res += fmt::format("{:{}}{}\n", "", indent, line);
    }
    res.pop_back();
    return res;
}

struct enum_gen : codegen::generator_base<enumeration> {

};

struct union_gen : codegen::generator_base<union_type> {
public:
    using generator_base::generator_base;
    codegen::sections generate() override {
        constexpr auto format = R"__(@lidlrt.make_compound
class {0}:
    members = {{
{1}
    }}

    size = {2}
)__";

        std::vector<std::string> members;
        {
            auto offset = get().layout(mod()).offset_of("discriminator").value();
//            auto member_str = generate_member(mod(), "discriminator", offset, )
        }

        auto offset = get().layout(mod()).offset_of("val").value();
        for (auto& [name, mem] : get().all_members()) {
            auto member_str = generate_member(mod(), name, offset, *mem);
            members.push_back(reindent(member_str, 8));
        }

        codegen::section sect;
        sect.definition = fmt::format(
            format, name(), fmt::join(members, ",\n"), get().wire_layout(mod()).size());

        return {{sect}};
    }
};

struct struct_gen : codegen::generator_base<structure> {
public:
    using generator_base::generator_base;
    codegen::sections generate() override {
        constexpr auto format = R"__(@lidlrt.make_compound
class {0}:
    members = {{
{1}
    }}

    size = {2}
)__";

        std::vector<std::string> members;
        for (auto& [name, mem] : get().all_members()) {
            auto offset = get().layout(mod()).offset_of(name).value();
            auto member_str = generate_member(mod(), name, offset, mem);
            members.push_back(reindent(member_str, 8));
        }

        codegen::section sect;
        sect.definition = fmt::format(
            format, name(), fmt::join(members, ",\n"), get().wire_layout(mod()).size());

        return {{sect}};
    }
};

class backend : public codegen::backend {
public:
    void generate(const module& mod, std::ostream& str) override {
        codegen::detail::current_backend = this;

        for (auto& str : mod.structs) {
            auto sects = codegen::do_generate<struct_gen>(mod, str.get());
            assert(sects.get_sections().size() == 1);
            std::cout << sects.get_sections()[0].definition << '\n';
        }

        for (auto& str : mod.unions) {
            auto sects = codegen::do_generate<union_gen>(mod, str.get());
            assert(sects.get_sections().size() == 1);
            std::cout << sects.get_sections()[0].definition << '\n';
        }
    }

    std::string get_user_identifier(const module& mod, const name& name) const override {
        abort();
        return std::string();
    }

    std::string get_identifier(const module& mod, const name& name) const override {
        return py::get_identifier(mod, name);
    }

    backend() = default;
};

std::unique_ptr<codegen::backend> make_backend() {
    return std::make_unique<py::backend>();
}
} // namespace lidl::py