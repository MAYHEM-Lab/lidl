#include "sections.hpp"

#include "codegen.hpp"

namespace lidl::codegen {
std::string abs_name_for_mod(module& mod) {
    auto& root_mod = root_module(mod);
    auto symopt = recursive_definition_lookup(root_mod.get_scope(), &mod);
    if (!symopt) {
        report_user_error(error_type::fatal, {}, "Cannot determine namespace");
        exit(1);
    }
    return current_backend()->get_identifier(mod, name{*symopt});
}
std::string compute_namespace_for_section(const section_key_t& key) {
    switch (key.type) {
    case section_type::lidl_traits:
    case section_type::service_descriptor:
    case section_type::validator:
        return "lidl";
    case section_type::std_traits:
        return "std";
    default:
        return abs_name_for_mod(const_cast<module&>(*find_parent_module(key.symbol())));
    }
}

std::vector<section_key_t> def_keys_from_name(const module& mod, const name& nm) {
    std::vector<section_key_t> all;
    all.emplace_back(nm.base,
                     nm.args.empty() ? section_type::definition
                                     : section_type::generic_declaration);
    if (!nm.args.empty() && !get_symbol(nm.base)->is_intrinsic()) {
        auto ins = resolve(mod, nm);
        all.emplace_back(ins, section_type::definition);
    }
    for (auto& arg : nm.args) {
        if (auto n = std::get_if<name>(&arg.get_variant()); n) {
            auto sub = def_keys_from_name(mod, *n);
            all.insert(all.end(), sub.begin(), sub.end());
        }
    }
    return all;
}

std::string section_key_t::to_string(const module& mod) const {
    std::string sym;
    auto sh = recursive_definition_lookup(mod.symbols(), symbol());

    if (sh) {
        sym = current_backend()->get_identifier(mod, name{*sh});

        try {
            auto t = resolve(mod, name{*sh});
            if (t->src_info) {
                auto loc = lidl::to_string(*t->src_info);
                sym      = fmt::format("{} (defined at {})", sym, loc);
            }
        } catch (std::exception&) {
            // We don't care
        }
    }

    std::string typestr;
    switch (type) {
    case section_type::definition:
        typestr = "definition";
        break;
    case section_type::wire_types:
        typestr = "wire_types";
        break;
    case section_type::service_descriptor:
        typestr = "service_descriptor";
        break;
    case section_type::sync_server:
        typestr = "sync_server";
        break;
    case section_type::async_server:
        typestr = "async_server";
        break;
    case section_type::stub:
        typestr = "stub";
        break;
    case section_type::async_stub:
        typestr = "async_stub";
        break;
    case section_type::zerocopy_stub:
        typestr = "zerocopy_stub";
        break;
    case section_type::async_zerocopy_stub:
        typestr = "async_zerocopy_stub";
        break;
    case section_type::generic_declaration:
        typestr = "generic_declaration";
        break;
    case section_type::lidl_traits:
        typestr = "lidl_traits";
        break;
    case section_type::std_traits:
        typestr = "std_traits";
        break;
    case section_type::eq_operator:
        typestr = "eq_operator";
        break;
    case section_type::validator:
        typestr = "validator";
        break;
    }

    assert(!typestr.empty());

    return fmt::format("{} for {}", typestr, sym);
}
} // namespace lidl::codegen