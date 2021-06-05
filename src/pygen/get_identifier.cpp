#include "get_identifier.hpp"

#include <fmt/format.h>
#include <lidl/scope.hpp>
#include <unordered_map>

namespace lidl::py {
namespace {
std::unordered_map<std::string_view, std::string_view> basic_lidl_types{
    {"f32", "lidlrt.F32"},
    {"f64", "lidlrt.F64"},
    {"i8", "lidlrt.I8"},
    {"i16", "lidlrt.I16"},
    {"i32", "lidlrt.I32"},
    {"i64", "lidlrt.I64"},
    {"u8", "lidlrt.U8"},
    {"u16", "lidlrt.U16"},
    {"u32", "lidlrt.U32"},
    {"u64", "lidlrt.U64"},
    {"bool", "lidlrt.Bool"},
    {"ptr", "lidlrt.Pointer"},
    {"string", "lidlrt.String"},
    {"array", "lidlrt.Array"}
};

std::vector<std::pair<symbol_handle, std::string>> rename_lookup;

std::optional<std::string> known_type_conversion(const std::string_view& name) {
    if (auto it = basic_lidl_types.find(name); it != basic_lidl_types.end()) {
        return std::string(it->second);
    }

    return std::nullopt;
}

std::string get_identifier(const module& mod, const symbol_handle& handle) {
    auto full_path = absolute_name(handle);
    std::vector<std::string> converted_parts(full_path.size());
    std::transform(full_path.begin(),
                   full_path.end(),
                   converted_parts.begin(),
                   [](auto part) -> std::string {
                       if (auto known = known_type_conversion(part); known) {
                           return *known;
                       }
                       return std::string(part);
                   });

    if (auto it = std::find_if(
            rename_lookup.begin(),
            rename_lookup.end(),
            [&handle, &converted_parts](auto& p) { return p.first == handle; });
        it != rename_lookup.end()) {
        converted_parts.back() = it->second;
    }

    return fmt::format("{}", fmt::join(converted_parts, "."));
}

std::string get_identifier(const module& mod, int64_t i) {
    return std::to_string(i);
}
} // namespace

std::string get_identifier(const module& mod, const name& n) {
    auto base_name = get_identifier(mod, n.base);
    if (n.args.empty()) {
        return std::string(base_name);
    }

    std::vector<std::string> generic_args(n.args.size());
    std::transform(n.args.begin(),
                   n.args.end(),
                   generic_args.begin(),
                   [&](const generic_argument& arg) {
                       return std::visit(
                           [&](auto& arg) { return get_identifier(mod, arg); },
                           arg.get_variant());
                   });

    return fmt::format("{}({})", base_name, fmt::join(generic_args, ", "));
}
} // namespace lidl::py