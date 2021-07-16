#include "cppgen.hpp"

#include "emitter.hpp"
#include "enum_gen.hpp"
#include "generator_base.hpp"
#include "generic_gen.hpp"
#include "lidl/enumeration.hpp"
#include "lidl/union.hpp"
#include "raw_union_gen.hpp"
#include "service_stub_gen.hpp"
#include "struct_bodygen.hpp"
#include "struct_gen.hpp"
#include "union_gen.hpp"

#include <algorithm>
#include <codegen.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fstream>
#include <lidl/basic.hpp>
#include <lidl/module.hpp>
#include <lidl/view_types.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lidl::cpp {
namespace {
using codegen::do_generate;

struct cppgen {
    explicit cppgen(const module& mod)
        : m_module(&mod) {
    }

    void generate(std::ostream& str) {
        for (auto& serv : m_module->services) {
            {
                auto call_union = &serv->procedure_params_union(mod());
                auto sym        = *serv->get_scope().definition_lookup(call_union);
                auto union_name = local_name(sym);

                rename(mod(), sym, fmt::format("wire_types::{}", union_name));
            }
            {
                auto call_union = &serv->procedure_results_union(mod());
                auto sym        = *serv->get_scope().definition_lookup(call_union);
                auto union_name = local_name(sym);

                rename(mod(), sym, fmt::format("wire_types::{}", union_name));
            }
        }

        for (auto& e : m_module->enums) {
            m_sections.merge_before(codegen::do_generate<enum_gen>(mod(), e.get()));
        }

        for (auto& u : mod().unions) {
            if (u->raw) {
                m_sections.merge_before(do_generate<raw_union_gen>(mod(), u.get()));
            } else {
                m_sections.merge_before(do_generate<union_gen>(mod(), u.get()));
            }
        }

        for (auto& s : mod().structs) {
            m_sections.merge_before(do_generate<struct_gen>(mod(), s.get()));
        }

        for (auto& service : mod().services) {
            m_sections.merge_before(
                do_generate<better_service_generator>(mod(), service.get()));
        }

        for (auto& ins : mod().instantiations) {
            m_sections.merge_before(
                do_generate<generic_gen>(mod(), ins->get_generic(), *ins));
        }

        auto& root_mod = root_module(mod());
        codegen::emitter e(root_mod, mod(), m_sections);

        str << "#pragma once\n\n#include <lidlrt/lidl.hpp>\n";
        if (!mod().services.empty()) {
            str << "#include <lidlrt/service.hpp>\n";
            str << "#include <tos/task.hpp>\n";
        }
        for (auto& imported : mod().imported_modules) {
            if (imported->src_info && imported->src_info->origin) {
                auto path = *imported->src_info->origin;
                auto file_name = std::filesystem::path(path).stem().string();
                str << fmt::format("#include <{}_generated.hpp>\n", file_name);

            }
        }
        str << '\n';

        str << e.emit() << '\n';
    }

private:
    codegen::sections m_sections;

    const module& mod() {
        return *m_module;
    }

    const module* m_module;
};

class backend : public codegen::backend {
public:
    void generate(const module& mod, const codegen::output& out) override {
        codegen::detail::current_backend = this;
        for (auto& [_, child] : mod.children) {
            std::ostringstream oss;
            cppgen gen(*child);
            gen.generate(oss);
        }

        std::ostream* str = &std::cout;

        if (out.output_path) {
            str = new std::ofstream{*out.output_path};
            if (!str->good()) {
                throw std::runtime_error("File could not be opened: " + *out.output_path);
            }
        }

        cppgen gen(mod);
        gen.generate(*str);
        str->flush();
    }
    std::string get_user_identifier(const module& mod,
                                    const lidl::name& name) const override {
        return cpp::get_user_identifier(mod, name);
    }

    std::string get_identifier(const module& mod, const lidl::name& name) const override {
        return cpp::get_identifier(mod, name);
    }
};
} // namespace

std::unique_ptr<codegen::backend> make_backend() {
    return std::make_unique<cpp::backend>();
}
} // namespace lidl::cpp