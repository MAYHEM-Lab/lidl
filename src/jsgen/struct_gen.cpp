#include "struct_gen.hpp"

#include "get_identifier.hpp"

#include <fmt/format.h>
#include <lidl/types.hpp>
#include <lidl/view_types.hpp>

namespace lidl::js {
sections struct_gen::generate() {
    constexpr auto format = R"__(
class {name}Class implements lidl.StructType {{
    instantiate(data: Uint8Array): {name} {{
        return new {name}(data);
    }}

    {layout}

    {members_def}
}};

class {name} extends lidl.StructObject {{
    get_type() : {name}Class {{
        return new {name}Class();
    }}

    {members}
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
                           fmt::arg("layout", generate_layout()),
                           fmt::arg("members_def", generate_members()));

    return sections{{std::move(def)}};
}

std::string struct_gen::generate_member(std::string_view mem_name, const member& mem) {
    auto mem_type = get_type(mod(), mem.type_);
    if (!mem_type->is_reference_type(mod()) &&
        !dynamic_cast<const view_type*>(mem_type)) {
        // This is a basic type
        constexpr auto format = R"__(get {mem_name}() {{
            const mem = this.get_type().members().{mem_name};
            return mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size)).value;
        }}

        set {mem_name}(val) {{
            const mem = this.get_type().members().{mem_name};
            mem.type.instantiate(this.sliceBuffer(mem.offset, mem.type.layout().size)).value = val;
        }})__";

        return fmt::format(
            format, fmt::arg("mem_name", mem_name), fmt::arg("name", name()));
    }

    return "";
}

std::string struct_gen::generate_members() const {
    constexpr auto format = R"__(
    members() {{
        return {{
            {member_data}
        }};
    }}
)__";

    std::vector<std::string> member_offsets(get().members.size());
    std::transform(get().members.begin(),
                   get().members.end(),
                   member_offsets.begin(),
                   [this](auto& member_entry) {
                       auto& t  = member_entry.second.type_;
                       auto typ = get_type(mod(), t);
                       return fmt::format(
                           "{} : {{ offset: {}, type: new {}Class() }}",
                           member_entry.first,
                           get().layout(mod()).offset_of(member_entry.first).value(),
                           get_local_js_identifier(mod(), t));
                   });

    return fmt::format(format,
                       fmt::arg("member_data", fmt::join(member_offsets, ",\n")));
}

std::string struct_gen::generate_layout() const {
    auto layout = get().layout(mod());

    static constexpr auto format = R"__(
    layout(): Layout {{
        return {{
            size: {size},
            alignment: {align}
        }};
    }}
)__";

    return fmt::format(format,
                       fmt::arg("size", layout.get().size()),
                       fmt::arg("align", layout.get().alignment()));
}
} // namespace lidl::js
