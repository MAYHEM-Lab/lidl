//
// Created by fatih on 6/3/20.
//

#include "generic_gen.hpp"

#include "cppgen.hpp"
#include "struct_gen.hpp"
#include "union_gen.hpp"

#include <lidl/module.hpp>

namespace lidl::cpp {
using codegen::section;
using codegen::sections;
std::string generic_gen::local_full_name() {
    return get_local_identifier(mod(), get().args);
}
std::string generic_gen::full_name() {
    return get_identifier(mod(), get().args);
}

sections generic_gen::do_generate(const generic_structure& str) {
    auto par_mod = find_parent_module(&str);
    par_mod->throwaway.emplace_back(str.instantiate(*par_mod, get().args));

    auto& un = static_cast<structure&>(*par_mod->throwaway.back());
    define(par_mod->symbols(), local_full_name(), &un);

    struct_gen gen(*par_mod, local_full_name(), name(), full_name(), un);

    return gen.generate();
}

sections generic_gen::do_generate(const generic_union& u) {
    auto par_mod = find_parent_module(&u);
    par_mod->throwaway.emplace_back(u.instantiate(*par_mod, get().args));

    auto& un = static_cast<union_type&>(*par_mod->throwaway.back());
    define(par_mod->symbols(), local_full_name(), &un);

    union_gen gen(*par_mod, local_full_name(), name(), full_name(), un);

    return gen.generate();
}

namespace {
std::string declare_template(const module& mod,
                             std::string_view generic_name,
                             const basic_generic& generic) {
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

    return fmt::format(
        "template <{}>\nclass {};\n", fmt::join(params, ", "), generic_name);
}

} // namespace

sections generic_gen::generate() {
    sections common;

    auto generic_decl_key =
        section_key_t{get().get_generic(), section_type::generic_declaration};

    auto sym = resolve(mod(), get().args);

    section decl;
    decl.add_key(generic_decl_key);
    decl.definition = declare_template(mod(), name(), *get().get_generic());
    common.add(std::move(decl));

    if (auto genstr = dynamic_cast<const generic_structure*>(get().get_generic())) {
        auto res = do_generate(*genstr);
        for (auto& sec : res.get_sections()) {
            if (std::any_of(sec.keys().begin(), sec.keys().end(), [](auto& key) {
                    return key.type == section_type::definition;
                })) {
                sec.definition = "template <>\n" + sec.definition;
                sec.add_dependency(generic_decl_key);
                sec.add_key({sym, section_type::definition});
            }
        }
        common.merge_before(res);
    } else if (auto genun = dynamic_cast<const generic_union*>(get().get_generic())) {
        auto res = do_generate(*genun);
        for (auto& sec : res.get_sections()) {
            if (std::any_of(sec.keys().begin(), sec.keys().end(), [](auto& key) {
                    return key.type == section_type::definition;
                })) {
                sec.definition = "template <>\n" + sec.definition;
                sec.add_dependency(generic_decl_key);
                sec.add_key({sym, section_type::definition});
            }
        }
        common.merge_before(res);
    } else {
        // This is not a user defined generic, so no point in emitting it.
        return {};
    }

    return common;
}
} // namespace lidl::cpp