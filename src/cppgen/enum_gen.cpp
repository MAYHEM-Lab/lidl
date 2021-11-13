#include "enum_gen.hpp"

#include "cppgen.hpp"

namespace lidl::cpp {
using codegen::sections;
using codegen::section;

sections enum_gen::generate() {
    return do_generate();
}
sections enum_gen::do_generate() {
    std::vector<std::string> members;
    for (auto& [name, member] : get().all_members()) {
        members.push_back(fmt::format("{} = {}", name, member->value));
    }

    constexpr auto format = R"__(enum class {} : {} {{
            {}
        }};)__";

    section s;
    s.add_key(def_key());

    s.definition = fmt::format(
        format, name(), get_identifier(mod(), get().underlying_type), fmt::join(members, ",\n"));

    auto res = generate_traits();
    res.add(s);
    return res;
}

sections enum_gen::generate_traits() {
    std::vector<std::string> names;
    std::vector<std::string> fixed_names;
    for (auto& [name, val] : get().all_members()) {
        names.emplace_back(fmt::format("\"{}\"", name));
        fixed_names.emplace_back(fmt::format("fixed_string(\"{}\")", name));
    }

    constexpr auto format = R"__(
            template <>
            struct enum_traits<{0}> {{
                static constexpr inline std::array<std::string_view, {1}> names {{{2}}};
                static constexpr inline auto fixed_names = std::tuple({3});
                static constexpr std::string_view name = "{0}";
            }};
        )__";

    section trait_sect;
    trait_sect.add_key({this->symbol(), section_type::lidl_traits});
    trait_sect.definition =
        fmt::format(format, absolute_name(), names.size(), fmt::join(names, ", "), fmt::join(fixed_names, ", "));
    trait_sect.add_dependency(def_key());

    return sections{{std::move(trait_sect)}};
}
} // namespace lidl::cpp
