#include "sections.hpp"

#include "codegen.hpp"

namespace lidl::codegen {
std::string section_key_t::to_string(const module& mod) {
    std::string sym;
    auto sh = recursive_definition_lookup(mod.symbols(), symbol);

    if (sh) {
        sym = current_backend()->get_identifier(mod, name{*sh});

        try {
            auto t = get_type(mod, name{*sh});
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
    case section_type::service_params_union:
        typestr = "service_params_union";
        break;
    case section_type::service_return_union:
        typestr = "service_return_union";
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