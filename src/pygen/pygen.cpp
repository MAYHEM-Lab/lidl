#include "get_identifier.hpp"

#include <codegen.hpp>
#include <filesystem>
#include <fstream>
#include <generator_base.hpp>
#include <lidl/algorithm.hpp>

namespace lidl::py {
namespace {
constexpr auto member_format = R"__({{
    "type": {0},
    "offset": {1}
}})__";

std::string replace_all(std::string str, std::string_view from, std::string_view to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

std::string local_name(std::string_view cur, std::string other) {
    auto last_dot = cur.find_last_of('.');
    cur           = cur.substr(0, last_dot);
    last_dot      = cur.find_last_of('.');
    cur           = cur.substr(0, last_dot + 1);
    std::cerr << cur << " -> " << other << '\n';
    other = replace_all(other, cur, "");
    return other;
}
} // namespace

std::string generate_member(const module& mod,
                            std::string_view name,
                            int offset,
                            const member& mem,
                            const lidl::name& who) {
    auto member_type_name = get_wire_type_name(mod, mem.type_);

    auto type_name = get_local_identifier(mod, who, member_type_name);

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
public:
    using generator_base::generator_base;

    codegen::sections generate() override {
        constexpr auto format = R"__(@lidlrt.Enum
class {0}:
    base_type = {2}
    elems = {{
{1}
    }}
)__";

        std::vector<std::string> elem_strings;
        for (auto& [name, el] : get().all_members()) {
            elem_strings.push_back(fmt::format("\"{}\": {}", name, el->value));
        }
        auto all_elems = fmt::format("{}", fmt::join(elem_strings, ",\n"));
        all_elems      = reindent(std::move(all_elems), 8);

        codegen::section sect;
        sect.definition = fmt::format(format,
                                      name(),
                                      all_elems,
                                      codegen::detail::current_backend->get_identifier(
                                          mod(), get().underlying_type));

        return {{std::move(sect)}};
    }
};

struct union_gen : codegen::generator_base<union_type> {
public:
    using generator_base::generator_base;
    codegen::sections generate() override {
        constexpr auto format = R"__(@lidlrt.Union
class {0}:
{3}

    members = {{
{1}
    }}

    size = {2}
)__";

        auto enum_name = "alternatives";
        auto abs_name  = fmt::format("{}.{}", absolute_name(), enum_name);
        enum_gen en(mod(), enum_name, abs_name, get().get_enum(mod()));

        auto enum_body = reindent(en.generate().get_sections()[0].definition, 4);

        std::vector<std::string> members;
        members.push_back(reindent(fmt::format("\"{}\": {}",
                                               "alternative",
                                               fmt::format(member_format, enum_name, 0)),
                                   8));
        codegen::section sect;

        auto offset = get().layout(mod()).offset_of("val").value();
        for (auto& [name, mem] : get().all_members()) {
            auto member_str = generate_member(
                mod(),
                name,
                offset,
                *mem,
                lidl::name{recursive_definition_lookup(mod().get_scope(), this->symbol())
                               .value()});
            members.push_back(reindent(member_str, 8));
            sect.add_dependencies(codegen::def_keys_from_name(
                mod(), get_wire_type_name(mod(), mem->type_)));
        }

        sect.definition = fmt::format(format,
                                      name(),
                                      fmt::join(members, ",\n"),
                                      get().wire_layout(mod()).size(),
                                      enum_body);

        if (get().is_reference_type(mod())) {
            sect.definition += "\n" + reindent("is_ref = True", 4);
        }

        return {{sect}};
    }
};

struct struct_gen : codegen::generator_base<structure> {
public:
    using generator_base::generator_base;
    codegen::sections generate() override {
        constexpr auto format = R"__(@lidlrt.Struct
class {0}:
    members = {{
{1}
    }}

    size = {2}
)__";
        codegen::section sect;

        std::vector<std::string> members;
        for (auto& [name, mem] : get().all_members()) {
            auto offset     = get().layout(mod()).offset_of(name).value();
            auto member_str = generate_member(
                mod(),
                name,
                offset,
                mem,
                lidl::name{recursive_definition_lookup(mod().get_scope(), this->symbol())
                               .value()});
            members.push_back(reindent(member_str, 8));
            sect.add_dependencies(
                codegen::def_keys_from_name(mod(), get_wire_type_name(mod(), mem.type_)));
        }

        sect.definition = fmt::format(
            format, name(), fmt::join(members, ",\n"), get().wire_layout(mod()).size());

        if (get().is_reference_type(mod())) {
            sect.definition += "\n" + reindent("is_ref = True", 4);
        }

        return {{sect}};
    }
};

struct service_gen : codegen::generator_base<service> {
    using generator_base::generator_base;

    codegen::sections generate() override {
        std::vector<codegen::sections> stuff;

        for (auto& [name, proc] : get().own_procedures()) {
            {
                auto& params_str = proc->params_struct(mod());
                auto sym  = *recursive_definition_lookup(mod().symbols(), &params_str);
                auto name = local_name(sym);
                auto abs_name = fmt::format("{}.{}", absolute_name(), name);
                struct_gen str_gen(mod(), name, abs_name, params_str);
                stuff.push_back(str_gen.generate());
                stuff.back().get_sections().back().definition +=
                    fmt::format("\n{0}.{1} = {1}\n", this->name(), name);
            }
            {
                auto& params_str = proc->results_struct(mod());
                auto sym  = *recursive_definition_lookup(mod().symbols(), &params_str);
                auto name = local_name(sym);
                auto abs_name = fmt::format("{}.{}", absolute_name(), name);
                struct_gen str_gen(mod(), name, abs_name, params_str);
                stuff.push_back(str_gen.generate());
                stuff.back().get_sections().back().definition +=
                    fmt::format("\n{0}.{1} = {1}\n", this->name(), name);
            }
        }

        {
            auto sym      = *recursive_definition_lookup(mod().symbols(),
                                                    &get().procedure_params_union(mod()));
            auto name     = local_name(sym);
            auto abs_name = fmt::format("{}.{}", absolute_name(), name);
            union_gen str_gen(mod(), name, abs_name, get().procedure_params_union(mod()));
            stuff.push_back(str_gen.generate());
            stuff.back().get_sections().back().definition +=
                fmt::format("\n{0}.{1} = {1}\n", this->name(), name);
        }

        {
            auto sym = *recursive_definition_lookup(
                mod().symbols(), &get().procedure_results_union(mod()));
            auto name     = local_name(sym);
            auto abs_name = fmt::format("{}.{}", absolute_name(), name);
            union_gen str_gen(
                mod(), name, abs_name, get().procedure_results_union(mod()));
            stuff.push_back(str_gen.generate());
            stuff.back().get_sections().back().definition +=
                fmt::format("\n{0}.{1} = {1}\n", this->name(), name);
        }
        constexpr auto format = R"__(class {0}:
    procedures = [{2}]


{1}

{0} = lidlrt.Service({0})
)__";

        std::vector<std::string> defs(stuff.size());
        std::transform(stuff.begin(), stuff.end(), defs.begin(), [](auto& sects) {
            return sects.get_sections()[0].definition;
        });


        codegen::section sect;

        std::vector<std::string> proc_names;
        for (auto& [name, proc] : get().all_procedures()) {
            proc_names.push_back(fmt::format("\"{}\"", name));
        }

        sect.definition = fmt::format(
            format, name(), fmt::join(defs, "\n\n"), fmt::join(proc_names, ", "));

        return {{sect}};
    }
};

class backend : public codegen::backend {
public:
    void generate(const module& mod, const codegen::output& str) override {
        if (!str.output_path) {
            throw std::runtime_error("Python backend requires an output path!");
        }

        auto root_abs_name =
            absolute_name(recursive_definition_lookup(root_module(mod).get_scope(),
                                                      const_cast<module*>(&mod))
                              .value());
        auto mod_path = fmt::format("{}", fmt::join(root_abs_name, "/"));


        std::filesystem::path base{*str.output_path};

        for (auto& part : root_abs_name) {
            base /= part;
            if (std::filesystem::exists(base)) {
                assert(!std::filesystem::exists(base / "__init__.py"));
            } else {
                std::filesystem::create_directories(base);
            }
        }

        std::filesystem::create_directories(base);

        // For every module, create a directory
        // For every element, create a file
        // Emit imports correctly
        codegen::detail::current_backend = this;

        for (auto& x : mod.services) {
            x->procedure_params_union(mod);
            x->procedure_results_union(mod);

            auto sects = codegen::do_generate<service_gen>(mod, x.get());
            assert(sects.get_sections().size() == 1);
            auto sym  = *recursive_definition_lookup(mod.symbols(), x.get());
            auto name = local_name(sym);
            std::ofstream out_file{base /
                                   std::filesystem::path(fmt::format("{}.py", name))};
            out_file << "import lidlrt\n";
            for (auto& dep : sects.get_sections()[0].depends_on) {
                auto import_str = import_string(mod, dep.symbol());
                if (import_str.empty()) {
                    // Must be a lidl type, already imported
                    continue;
                }
                out_file << fmt::format("import {}\n", import_str);
            }
            out_file << '\n';
            out_file << sects.get_sections()[0].definition << '\n';
        }

        for (auto& x : mod.enums) {
            auto sects = codegen::do_generate<enum_gen>(mod, x.get());
            assert(sects.get_sections().size() == 1);
            auto sym  = *recursive_definition_lookup(mod.symbols(), x.get());
            auto name = local_name(sym);
            std::ofstream out_file{base /
                                   std::filesystem::path(fmt::format("{}.py", name))};
            out_file << "import lidlrt\n";
            for (auto& dep : sects.get_sections()[0].depends_on) {
                auto import_str = import_string(mod, dep.symbol());
                if (import_str.empty()) {
                    // Must be a lidl type, already imported
                    continue;
                }
                out_file << fmt::format("import {}\n", import_str);
            }
            out_file << '\n';
            out_file << sects.get_sections()[0].definition << '\n';
        }

        for (auto& x : mod.structs) {
            auto sects = codegen::do_generate<struct_gen>(mod, x.get());
            assert(sects.get_sections().size() == 1);
            auto sym  = *recursive_definition_lookup(mod.symbols(), x.get());
            auto name = local_name(sym);
            std::ofstream out_file{base /
                                   std::filesystem::path(fmt::format("{}.py", name))};
            out_file << "import lidlrt\n";
            for (auto& dep : sects.get_sections()[0].depends_on) {
                auto import_str = import_string(mod, dep.symbol());
                if (import_str.empty()) {
                    // Must be a lidl type, already imported
                    continue;
                }
                out_file << fmt::format("import {}\n", import_str);
            }
            out_file << '\n';
            out_file << sects.get_sections()[0].definition << '\n';
        }

        for (auto& x : mod.unions) {
            auto sects = codegen::do_generate<union_gen>(mod, x.get());
            assert(sects.get_sections().size() == 1);
            auto sym  = *recursive_definition_lookup(mod.symbols(), x.get());
            auto name = local_name(sym);
            std::ofstream out_file{base /
                                   std::filesystem::path(fmt::format("{}.py", name))};
            out_file << "import lidlrt\n";
            for (auto& dep : sects.get_sections()[0].depends_on) {
                auto import_str = import_string(mod, dep.symbol());
                if (import_str.empty()) {
                    // Must be a lidl type, already imported
                    continue;
                }
                out_file << fmt::format("import {}\n", import_str);
            }
            out_file << '\n';
            out_file << sects.get_sections()[0].definition << '\n';
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