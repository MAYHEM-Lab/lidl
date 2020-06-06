#include "cppgen.hpp"
#include "lidl/scope.hpp"

#include <stdexcept>


namespace lidl::cpp {
namespace {
std::optional<std::string> known_type_conversion(const std::string_view& name) {
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
        {"array", "::lidl::array"},
        {"optional", "::lidl::optional"},
        {"string", "::lidl::string"},
        {"string_view", "std::string_view"},
        {"span", "tos::span"},
        {"vector", "::lidl::vector"},
        {"ptr", "::lidl::ptr"},
        {"expected", "::lidl::expected"}};

    if (auto it = basic_types.find(name); it != basic_types.end()) {
        return std::string(it->second);
    }

    return std::nullopt;
}
} // namespace

std::string get_identifier(const module& mod, const symbol_handle& handle) {
    auto base_name = std::string(nameof(handle));
    if (auto known = known_type_conversion(base_name); known) {
        base_name = *known;
    }
    return base_name;
}

std::string get_identifier(const module& mod, int64_t i) {
    return std::to_string(i);
}

std::string get_identifier(const module& mod, forward_decl) {
    throw std::runtime_error("unresolved forward declaration");
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

std::string get_user_identifier(const module& mod, const name& n) {
    auto& ntype = *get_type(mod, n);
    if (ntype.is_reference_type(mod)) {
        // must be a pointer instantiation
        auto& base = std::get<name>(n.args[0]);
        return get_identifier(mod, base);
    }

    return get_identifier(mod, n);
}
} // namespace lidl::cpp