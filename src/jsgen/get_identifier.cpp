#include "get_identifier.hpp"

#include "lidl/scope.hpp"

#include <stdexcept>

namespace lidl::js {
namespace {
std::optional<std::string> known_type_conversion(const std::string_view& name) {
    std::unordered_map<std::string_view, std::string_view> basic_types{
        {"f32", "lidl.Float"},
        {"f64", "lidl.Double"},
        {"i8", "lidl.Int8"},
        {"i16", "lidl.Int16"},
        {"i32", "lidl.Int32"},
        {"i64", "lidl.Int64"},
        {"u8", "lidl.Uint8"},
        {"u16", "lidl.Uint16"},
        {"u32", "lidl.Uint32"},
        {"u64", "lidl.Uint64"},
        {"array", "lidl.Array"},
        {"optional", "lidl::optional"},
        {"string", "lidl.LidlString"},
        {"string_view", "string"},
        {"vector", "lidl::vector"},
        {"ptr", "lidl.Ptr"}};

    if (auto it = basic_types.find(name); it != basic_types.end()) {
        return std::string(it->second);
    }

    return std::nullopt;
}
} // namespace

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

    return fmt::format("{}", fmt::join(converted_parts, "."));
}

std::string get_local_identifier(const module& mod, const symbol_handle& handle) {
    auto full_path = std::vector<std::string_view>{local_name(handle)};
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

    return fmt::format("{}", fmt::join(converted_parts, "."));
}

std::string get_identifier(const module& mod, int64_t i) {
    return std::to_string(i);
}

std::string get_identifier(const module& mod, forward_decl) {
    throw std::runtime_error("unresolved forward declaration");
}

std::string get_local_js_identifier(const module& mod, const name& n) {
    auto base_name = get_local_identifier(mod, n.base);
    if (n.args.empty()) {
        return std::string(base_name);
    }
    return base_name;
}

std::string get_js_identifier(const module& mod, const name& n) {
    auto base_name = get_identifier(mod, n.base);
    if (n.args.empty()) {
        return std::string(base_name);
    }

    return base_name;

    /*std::vector<std::string> generic_args(n.args.size());
    std::transform(n.args.begin(),
                   n.args.end(),
                   generic_args.begin(),
                   [&](const generic_argument& arg) {
                     return std::visit(
                         [&](auto& arg) { return get_identifier(mod, arg); },
                         arg.get_variant());
                   });

    return fmt::format("{}<{}>", base_name, fmt::join(generic_args, ", "));*/
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
    auto& ntype = *get_type(mod, n);
    if (ntype.is_reference_type(mod)) {
        // must be a pointer instantiation
        auto& base = std::get<name>(n.args[0]);
        return get_identifier(mod, base);
    }

    return get_identifier(mod, n);
}
} // namespace lidl::js