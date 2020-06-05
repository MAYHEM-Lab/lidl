#include "enum_gen.hpp"

#include "cppgen.hpp"

namespace lidl::cpp {

sections enum_gen::generate() {
    return do_generate();
}
sections enum_gen::do_generate() {
    std::stringstream pub;
    for (auto& [name, member] : get().members) {
        pub << fmt::format("{} = {},\n", name, member.value);
    }

    constexpr auto format = R"__(enum class {} : {} {{
            {}
        }};)__";

    section s;
    s.key = def_key();

    s.definition = fmt::format(
        format, name(), get_identifier(mod(), get().underlying_type), pub.str());

    auto res = generate_traits();
    res.add(s);
    return res;
}

sections enum_gen::generate_traits() {
    std::vector<std::string> names;
    for (auto& [name, val] : get().members) {
        names.emplace_back(fmt::format("\"{}\"", name));
    }

    constexpr auto format = R"__(
            template <>
            struct enum_traits<{0}> {{
                static constexpr inline std::array<const char*, {1}> names {{{2}}};
                static constexpr const char* name = "{0}";
            }};
        )__";

    section trait_sect;
    trait_sect.name_space = "lidl";
    trait_sect.definition =
        fmt::format(format, name(), names.size(), fmt::join(names, ", "));
    trait_sect.add_dependency(def_key());

    return sections{{std::move(trait_sect)}};
}
} // namespace lidl::cpp