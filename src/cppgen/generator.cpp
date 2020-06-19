#include "generator.hpp"
#include "cppgen.hpp"
#include <fmt/format.h>

namespace lidl::cpp {
std::string section_key_t::to_string(const module& mod) {
    std::string sym;

    if (auto str = std::get_if<std::string>(&symbol); str) {
        sym = "\"" + *str + "\"";
    } else if (auto sh = std::get_if<symbol_handle>(&symbol); sh) {
        sym = get_identifier(mod, {*sh});
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