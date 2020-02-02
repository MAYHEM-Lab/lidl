//
// Created by fatih on 1/31/20.
//

#include <lidl/module.hpp>

namespace lidl {
void reference_type_pass(module& m) {
    auto ptr_sym = m.symbols.lookup("ptr");
    auto ptr = std::get<const generic*>(*ptr_sym);

    for (auto& s : m.structs) {
        for (auto& [name, member] : s.members) {
            // Member isn't a ref type
            if (!member.type_->is_reference_type(m)) {
                continue;
            }

            if (auto gen = dynamic_cast<const generic_instantiation*>(member.type_);
                gen) {
                if (auto ptr = dynamic_cast<const pointer_type*>(&gen->generic_type());
                    ptr) {
                    // The member is already a pointer
                    continue;
                }

                for (auto& arg : gen->arguments()) {
                    if (auto sym = std::get_if<const symbol*>(&arg); sym) {
                        if (auto t = std::get_if<const type*>(*sym); t) {
                            // Convert any type T into ::lidl::ptr<T>
                            // Must register the name with symbol table?
                            const symbol* symb = m.symbols.reverse_lookup(*t);
                            if (!symb) {
                                auto gen_if = m.generated.find(member.type_);
                                if (gen_if == m.generated.end()) {
                                    throw std::runtime_error("where did this come from?");
                                }

                                std::vector<generic_argument> args{ptr_sym, &gen_if->first};
                                auto ptr_to_type = new generic_instantiation(*ptr, args);
                                m.generated.emplace(ptr_to_type, args);
                                member.type_ = ptr_to_type;
                                continue;
                            }

                            std::vector<generic_argument> args{ptr_sym, symb};
                            auto ptr_to_type = new generic_instantiation(*ptr, args);
                            m.generated.emplace(ptr_to_type, args);
                            member.type_ = ptr_to_type;
                        }
                    }
                }
            }

            // Convert any type T into ::lidl::ptr<T>
            // Must register the name with symbol table?
            const symbol* symb = m.symbols.reverse_lookup(member.type_);
            if (!symb) {
                auto gen_if = m.generated.find(member.type_);
                if (gen_if == m.generated.end()) {
                    throw std::runtime_error("where did this come from?");
                }

                std::vector<generic_argument> args{ptr_sym, &gen_if->first};
                auto ptr_to_type = new generic_instantiation(*ptr, args);
                m.generated.emplace(ptr_to_type, args);
                member.type_ = ptr_to_type;
                continue;
            }

            std::vector<generic_argument> args{ptr_sym, symb};
            auto ptr_to_type = new generic_instantiation(*ptr, args);
            m.generated.emplace(ptr_to_type, args);
            member.type_ = ptr_to_type;
        }
    }
}
} // namespace lidl