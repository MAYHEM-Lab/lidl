#include "struct_gen.hpp"

#include "get_identifier.hpp"

#include <fmt/format.h>
#include <lidl/types.hpp>
#include <lidl/view_types.hpp>

namespace lidl::js {
sections struct_gen::generate() {
    constexpr auto format = R"__(class {name} extends lidl.StructBase {{
    constructor(data) {{ super({name}); this._data = data; }}
    {members}

    static _metadata = {metadata};
}})__";

    std::vector<std::string> members(get().members.size());
    std::transform(get().members.begin(),
                   get().members.end(),
                   members.begin(),
                   [this](auto& member_entry) {
                       return generate_member(member_entry.first, member_entry.second);
                   });

    section def;
    def.body = fmt::format(format,
                           fmt::arg("name", name()),
                           fmt::arg("members", fmt::join(members, "\n")),
                           fmt::arg("metadata", generate_metadata()));

    return sections{{std::move(def)}};
}

std::string struct_gen::generate_member(std::string_view mem_name, const member& mem) {
    auto mem_type = get_type(mod(), mem.type_);
    if (!mem_type->is_reference_type(mod()) &&
        !dynamic_cast<const view_type*>(mem_type)) {
        // This is a basic type
        constexpr auto format = R"__(get {mem_name}() {{
            const mem = {name}._metadata.members.{mem_name};
            const buf = this._data.subarray(mem.offset, mem.offset + mem.type._metadata.layout.size);
            return new mem.type(buf);
        }}

        set {mem_name}(val) {{ this.{mem_name}.data = val; }})__";

        return fmt::format(format, fmt::arg("mem_name", mem_name), fmt::arg("name", name()));
    }

    return "";
}

std::string struct_gen::generate_metadata() {
    constexpr auto format = R"__({{
    name: "{name}",
    members: {{
        {member_offsets}
    }}
}})__";

    std::vector<std::string> member_offsets(get().members.size());
    std::transform(get().members.begin(),
                   get().members.end(),
                   member_offsets.begin(),
                   [this](auto& member_entry) {
                       auto& t  = member_entry.second.type_;
                       auto typ = get_type(mod(), t);
                       return fmt::format(
                           "{} : {{ offset: {}, type: {} }}",
                           member_entry.first,
                           get().layout(mod()).offset_of(member_entry.first).value(),
                           get_local_js_identifier(mod(), t));
                   });

    return fmt::format(format,
                       fmt::arg("name", name()),
                       fmt::arg("member_offsets", fmt::join(member_offsets, ",\n")));
}
} // namespace lidl::js
