#include "cppgen.hpp"
#include "lidl/scope.hpp"

#include <stdexcept>


namespace lidl::cpp {
namespace {
std::unordered_map<std::string_view, std::string_view> basic_types{
    {"f32", "float"},
    {"f64", "double"},
    {"i8", "int8_t"},
    {"i16", "int16_t"},
    {"i32", "int32_t"},
    {"i64", "int64_t"},
    {"u8", "uint8_t"},
    {"u16", "uint16_t"},
    {"u32", "uint32_t"},
    {"u64", "uint64_t"},
    {"array", "lidl::array"},
    {"optional", "lidl::optional"},
    {"string", "lidl::string"},
    {"string_view", "std::string_view"},
    {"span", "tos::span"},
    {"vector", "lidl::vector"},
    {"ptr", "lidl::ptr"},
    {"expected", "lidl::expected"}};

std::vector<std::pair<symbol_handle, std::string>> rename_lookup;

std::optional<std::string> known_type_conversion(const std::string_view& name) {
    if (auto it = basic_types.find(name); it != basic_types.end()) {
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

    if(auto it = std::find_if(rename_lookup.begin(), rename_lookup.end(), [&handle, &converted_parts](auto& p) {
          return p.first == handle;
        }); it != rename_lookup.end()) {
        converted_parts.back() = it->second;
    }

    return fmt::format("{}", fmt::join(converted_parts, "::"));
}

std::string get_local_identifier(const module& mod, const symbol_handle& handle) {
    if(auto it = std::find_if(rename_lookup.begin(), rename_lookup.end(), [&handle](auto& p) {
            return p.first == handle;
        }); it != rename_lookup.end()) {
        return it->second;
    }

    auto name = std::string(local_name(handle));
    if (auto known = known_type_conversion(name); known) {
        name = *known;
    }
    return name;
}

std::string get_identifier(const module& mod, int64_t i) {
    return std::to_string(i);
}
} // namespace

void rename(const module& mod, const symbol_handle& sym, std::string rename_to) {
    rename_lookup.emplace_back(sym, std::move(rename_to));
}

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

    return fmt::format("{}<{}>", base_name, fmt::join(generic_args, ", "));
}

std::string get_local_identifier(const module& mod, const name& n) {
    auto base_name = get_local_identifier(mod, n.base);
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

    return fmt::format("{}<{}>", base_name, fmt::join(generic_args, ", "));
}

std::string get_user_identifier(const module& mod, const name& n) {
    auto& nn = deref_ptr(mod, n);
    return get_identifier(mod, nn);
}
} // namespace lidl::cpp