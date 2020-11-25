//
// Created by fatih on 11/24/20.
//

#include "enum_gen.hpp"

#include "get_identifier.hpp"

namespace lidl::js {
using codegen::section;
codegen::sections enum_gen::generate() {
    std::vector<std::string> members;
    for (auto& [name, member] : get().members) {
        members.push_back(fmt::format("{} = {}", name, member.value));
    }

    constexpr auto obj_format = R"__(
class {name}Class implements lidl.Type {{
    instantiate(buf: Uint8Array) : lidl.LidlObject {{
        return new {name}(buf);
    }}

    layout() : lidl.Layout {{
        return {{
            size: {size},
            alignment: {align}
        }};
    }}

    underlying_type() {{
        return {underlying_name};
    }}
}}

class {name} extends lidl.LidlObject {{
    get_type(): {name}Class {{
        return new {name}Class();
    }}

    get value(): {name}Enum {{
        return <{name}Enum>this.underlying_obj().value;
    }}

    set value(val: {name}Enum) {{
        this.underlying_obj().value = val;
    }}

    private underlying_obj() {{
        const buf = super.buffer();
        return this.get_type().underlying_type().instantiate(buf);
    }}
}}
)__";

    constexpr auto format = R"__(enum {}Enum {{
            {}
        }};)__";

    section obj_s;
    obj_s.add_key(def_key());

    obj_s.definition = fmt::format(
        obj_format,
        fmt::arg("name", name()),
        fmt::arg("underlying_name", get_local_type_name(mod(), get().underlying_type)),
        fmt::arg("size", get().wire_layout(mod()).size()),
        fmt::arg("align", get().wire_layout(mod()).alignment()));

    section s;
    s.add_key(def_key());

    s.definition = fmt::format(format, name(), fmt::join(members, ",\n"));

    return {{obj_s, s}};
}
} // namespace lidl::js
