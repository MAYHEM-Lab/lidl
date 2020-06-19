//
// Created by fatih on 6/3/20.
//

#include "generic_gen.hpp"

#include "cppgen.hpp"
#include "struct_gen.hpp"
#include "union_gen.hpp"

namespace lidl {
void reference_type_pass(module& m);
void union_enum_pass(module& m);
} // namespace lidl

namespace lidl::cpp {
std::string generic_gen::full_name() {
    return get_identifier(mod(), get().get_name());
}

sections generic_gen::do_generate(const generic_structure& str) {
    module tmpmod;
    //TODO: Create an unrelated module here
    tmpmod.symbols = mod().symbols->add_child_scope("tmp");
    tmpmod.structs.emplace_back(str.instantiate(mod(), get()));
    reference_type_pass(tmpmod);

    struct_gen gen(tmpmod, symbol(), full_name(), name(), tmpmod.structs.back());
    return std::move(gen.generate());
}

sections generic_gen::do_generate(const generic_union& u) {
    module tmpmod;
    //TODO: Create an unrelated module here
    tmpmod.symbols = mod().symbols->add_child_scope("tmp");
    tmpmod.unions.emplace_back(u.instantiate(mod(), get()));
    reference_type_pass(tmpmod);
    union_enum_pass(tmpmod);

    union_gen gen(tmpmod, symbol(), full_name(), name(), tmpmod.unions.back());

    return std::move(gen.generate());
}

sections generic_gen::do_generate() {
    //TODO
    if (auto genstr = dynamic_cast<const generic_structure*>(&get().generic_type())) {
        auto res = do_generate(*genstr);
//        res.m_body[full_name()] = "template <>\n" + res.m_body[full_name()];
        return res;
    }
    if (auto genun = dynamic_cast<const generic_union*>(&get().generic_type())) {
        auto res = do_generate(*genun);
//        res.m_body[full_name()] = "template <>\n" + res.m_body[full_name()];
        return res;
    }
    return sections();
}
}