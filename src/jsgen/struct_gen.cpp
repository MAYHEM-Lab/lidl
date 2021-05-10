#include "struct_gen.hpp"

#include "get_identifier.hpp"
#include "jsgen.hpp"

#include <fmt/format.h>
#include <lidl/types.hpp>
#include <lidl/view_types.hpp>

namespace lidl::js {
using codegen::section;
using codegen::sections;
sections struct_gen::generate() {
    constexpr auto format = R"__(
export class {name}Class implements lidl.StructType {{
    instantiate(data: Uint8Array): {name} {{
        return new {name}(data);
    }}

    {ctor}

    {layout}

    {members_def}
}}

export class {name} extends {struct_base} {{
    get_type() : {name}Class {{
        return new {name}Class();
    }}

    {members}
}})__";

    std::vector<std::string> members(get().own_members().size());
    std::transform(get().own_members().begin(),
                   get().own_members().end(),
                   members.begin(),
                   [this](auto& member_entry) {
                       return generate_member(member_entry.first, member_entry.second);
                   });

    std::string struct_base = "lidl.StructObject";
    if (get().is_value(mod())) {
        struct_base = "lidl.ValueStructObject";
    }

    section def;
    def.definition = fmt::format(format,
                                 fmt::arg("name", name()),
                                 fmt::arg("members", fmt::join(members, "\n")),
                                 fmt::arg("layout", generate_layout_getter(mod(), get())),
                                 fmt::arg("members_def", generate_members()),
                                 fmt::arg("struct_base", struct_base),
                                 fmt::arg("ctor", generate_ctor()));
    def.add_key(def_key());

    return sections{{std::move(def)}};
}

std::string struct_gen::generate_member(std::string_view mem_name, const member& mem) {
    auto mem_type = get_wire_type(mod(), mem.type_);
    if (mem_type->is_value(mod())) {
        // This is a basic type
        constexpr auto format = R"__(get {mem_name}() {{
            return this.member_by_name("{mem_name}").value;
        }}

        set {mem_name}(val) {{
            this.member_by_name("{mem_name}").value = val;
        }})__";

        return fmt::format(
            format, fmt::arg("mem_name", mem_name), fmt::arg("name", name()));
    }
    if (mem_type->is_reference_type(mod())) {
        constexpr auto format = R"__(get {mem_name}() {{
            return this.member_by_name("{mem_name}").value;
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

    std::vector<std::string> member_offsets(get().own_members().size());
    std::transform(get().own_members().begin(),
                   get().own_members().end(),
                   member_offsets.begin(),
                   [this](auto& member_entry) {
                       auto& t  = member_entry.second.type_;
                       auto typ = get_type(mod(), t);
                       return fmt::format(
                           "{} : {{ offset: {}, type: {} }}",
                           member_entry.first,
                           get().layout(mod()).offset_of(member_entry.first).value(),
                           get_local_type_name(mod(), t));
                   });

    return fmt::format(format, fmt::arg("member_data", fmt::join(member_offsets, ",\n")));
}

std::string struct_gen::generate_ctor() const {
    constexpr auto format = R"__(
    create({args}) : {name} {{
        const init = {{
            {init_map}
        }};
        return <{name}>lidl.CreateObject(this, mb, init);
    }}
)__";

    std::vector<std::string> args(get().own_members().size() + 1);
    args.front() = "mb: lidl.MessageBuilder";
    std::transform(get().own_members().begin(),
                   get().own_members().end(),
                   args.begin() + 1,
                   [this](auto& mem) {
                       auto& t = mem.second.type_;
                       return fmt::format(
                           "{}: {}", mem.first, get_local_user_obj_name(mod(), t));
                   });

    std::vector<std::string> init(get().own_members().size());
    std::transform(get().own_members().begin(),
                   get().own_members().end(),
                   init.begin(),
                   [this](auto& mem) {
                       auto& t = mem.second.type_;
                       return fmt::format("{}: {}", mem.first, mem.first);
                   });

    return fmt::format(format,
                       fmt::arg("args", fmt::join(args, ", ")),
                       fmt::arg("name", name()),
                       fmt::arg("init_map", fmt::join(init, ",\n")));
}
} // namespace lidl::js
