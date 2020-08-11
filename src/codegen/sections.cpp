#include "sections.hpp"
#include "codegen.hpp"

namespace lidl::codegen {
std::string section_key_t::to_string(const module& mod) {
    std::string sym;

    if (auto str = std::get_if<std::string>(&symbol); str) {
        sym = "\"" + *str + "\"";
    } else if (auto sh = std::get_if<symbol_handle>(&symbol); sh) {
        sym = current_backend()->get_identifier(mod, {*sh});

        try {
            auto t = get_type(mod, name{*sh});
            if (t->src_info) {
                auto loc = lidl::to_string(*t->src_info);
                sym = fmt::format("{} (defined at {})", sym, loc);
            }
        } catch (std::exception&) {
            // We don't care
        }
    } else {
        sym = "Nothing";
    }

    std::string typestr;
    switch (type) {
    case section_type::definition:
        typestr = "definition";
        break;
    case section_type::declaration:
        typestr = "declaration";
        break;
    case section_type::misc:
        typestr = "misc";
        break;
    }

    return fmt::format("{} for {}", typestr, sym);
}
}