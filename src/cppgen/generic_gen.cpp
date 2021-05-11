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
    lidl::module tmpmod(const_cast<module*>(&mod()), str.src_info);
    tmpmod.name_space = find_parent_module(&str)->name_space;
    define(mod().symbols(), "#", &tmpmod);
    auto instantiated = str.instantiate(tmpmod, get().args);
    tmpmod.structs.emplace_back(
        std::unique_ptr<structure>(static_cast<structure*>(instantiated.release())));
    auto& un = *tmpmod.structs.back();
    define(tmpmod.symbols(), local_full_name(), &un);

    struct_gen gen(tmpmod, local_full_name(), name(), full_name(), un);
    auto res = gen.generate();
    mod().symbols().undefine("#");

    return res;
}

sections generic_gen::do_generate(const generic_union& u) {
    lidl::module tmpmod(const_cast<module*>(&mod()), u.src_info);
    tmpmod.name_space = find_parent_module(&u)->name_space;
    define(mod().symbols(), "#", &tmpmod);
    auto instantiated = u.instantiate(tmpmod, get().args);
    tmpmod.unions.emplace_back(
        std::unique_ptr<union_type>(static_cast<union_type*>(instantiated.release())));
    auto& un = *tmpmod.unions.back();
    define(tmpmod.symbols(), local_full_name(), &un);

    union_gen gen(tmpmod, local_full_name(), name(), full_name(), un);

    auto res = gen.generate();
    mod().symbols().undefine("#");

    return res;
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