//
// Created by fatih on 6/3/20.
//

#include "generic_gen.hpp"

#include "../passes.hpp"
#include "cppgen.hpp"
#include "struct_gen.hpp"
#include "union_gen.hpp"

namespace lidl::cpp {
using codegen::section;
using codegen::sections;
std::string generic_gen::local_full_name() {
    return get_local_identifier(mod(), get().get_name());
}
std::string generic_gen::full_name() {
    return get_identifier(mod(), get().get_name());
}

sections generic_gen::do_generate(const generic_structure& str) {
    module tmpmod;
    tmpmod.name_space = mod().name_space;
    // TODO: Create an unrelated module here
    tmpmod.symbols = mod().symbols->add_child_scope("tmp");
    tmpmod.structs.emplace_back(
        *dynamic_cast<structure*>(str.instantiate(mod(), get()).get()));
    run_passes_until_stable(tmpmod);

    struct_gen gen(
        tmpmod, symbol(), local_full_name(), name(), full_name(), tmpmod.structs.back());
    return std::move(gen.generate());
}

sections generic_gen::do_generate(const generic_union& u) {
    module tmpmod;
    tmpmod.name_space = mod().name_space;
    // TODO: Create an unrelated module here
    tmpmod.symbols = mod().symbols->add_child_scope("tmp");
    tmpmod.unions.emplace_back(
        std::move(*dynamic_cast<union_type*>(u.instantiate(mod(), get()).get())));
    run_passes_until_stable(tmpmod);

    union_gen gen(
        tmpmod, symbol(), local_full_name(), name(), full_name(), tmpmod.unions.back());

    return std::move(gen.generate());
}

namespace {
std::string declare_template(const module& mod,
                             std::string_view generic_name,
                             const generic& generic) {
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

sections generic_gen::do_generate() {
    sections common;

    section decl;
    decl.name_space = mod().name_space;
    decl.add_key(decl_key());
    decl.definition = declare_template(mod(), name(), get().generic_type());
    common.add(std::move(decl));

    if (auto genstr = dynamic_cast<const generic_structure*>(&get().generic_type())) {
        auto res = do_generate(*genstr);
        for (auto& sec : res.get_sections()) {
            if (std::any_of(sec.keys.begin(), sec.keys.end(), [](auto& key) {
                    return key.type == section_type::definition;
                })) {
                sec.definition = "template <>\n" + sec.definition;
                sec.add_dependency(decl_key());
            }
        }
        common.merge_before(res);
    } else if (auto genun = dynamic_cast<const generic_union*>(&get().generic_type())) {
        auto res = do_generate(*genun);
        for (auto& sec : res.get_sections()) {
            if (std::any_of(sec.keys.begin(), sec.keys.end(), [](auto& key) {
                    return key.type == section_type::definition;
                })) {
                sec.definition = "template <>\n" + sec.definition;
                sec.add_dependency(decl_key());
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