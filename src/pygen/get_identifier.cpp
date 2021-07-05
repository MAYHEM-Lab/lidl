#include "get_identifier.hpp"

#include <fmt/format.h>
#include <lidl/base.hpp>
#include <lidl/module.hpp>
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
    {"array", "lidlrt.Array"},
    {"vector", "lidlrt.Vector"}};

std::vector<std::pair<symbol_handle, std::string>> rename_lookup;

std::optional<std::string_view> known_type_conversion(const std::string_view& name) {
    if (auto it = basic_lidl_types.find(name); it != basic_lidl_types.end()) {
        return it->second;
    }

    return std::nullopt;
}

qualified_name python_import_name(const symbol_handle& handle) {
    auto path      = path_to_root(handle);
    auto full_path = path_to_name(path);

    if (full_path.size() != 1) {
        // In python, each lidl element gets their own file, and thus, their module.
        // Therefore, we find the first lidl module in a path, and repeat the next
        // element.

        auto first_mod = std::find_if(
            path.begin(), path.end(), [](const base* el) { return el->is_module(); });
        assert(first_mod != path.end());
        auto idx = std::distance(path.begin(), first_mod);

        // We have the index, but the path vector is from an element to root.
        // So, for a human readable a.b.c, we'll have in path (c, b, a, _root_)
        // Therefore, the index is not immediately usable in the full_path vector

        auto full_path_idx = path.size() - idx - 1; // revert it

        // Duplicate the name at full_path_idx
        full_path.erase(full_path.begin() + full_path_idx + 1, full_path.end());
    }

    return full_path;
}

qualified_name python_absolute_name(const symbol_handle& handle) {
    auto path      = path_to_root(handle);
    auto full_path = path_to_name(path);

    if (full_path.size() != 1) {
        // In python, each lidl element gets their own file, and thus, their module.
        // Therefore, we find the first lidl module in a path, and repeat the next
        // element.

        auto first_mod = std::find_if(
            path.begin(), path.end(), [](const base* el) { return el->is_module(); });
        assert(first_mod != path.end());
        auto idx = std::distance(path.begin(), first_mod);

        // We have the index, but the path vector is from an element to root.
        // So, for a human readable a.b.c, we'll have in path (c, b, a, _root_)
        // Therefore, the index is not immediately usable in the full_path vector

        auto full_path_idx = path.size() - idx - 1; // revert it

        // Duplicate the name at full_path_idx
        full_path.insert(full_path.begin() + full_path_idx,
                         *(full_path.begin() + full_path_idx));
    }
    return full_path;
}


std::string get_identifier(const module& mod, const symbol_handle& handle) {
    auto full_path = python_absolute_name(handle);
    std::vector<std::string_view> converted_parts(full_path.size());
    std::transform(full_path.begin(),
                   full_path.end(),
                   converted_parts.begin(),
                   [](const auto& part) -> std::string_view {
                       if (auto known = known_type_conversion(part); known) {
                           return *known;
                       }
                       return part;
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


std::string get_local_identifier(const module& mod, const symbol_handle& handle) {
    return std::string(local_name(handle));
}

std::string get_local_identifier(const module& mod, int64_t i) {
    return std::to_string(i);
}
} // namespace


std::string get_local_identifier(const module& mod, const name& from, const name& n) {
    auto f = resolve(mod, from);

    auto base_name = get_identifier(mod, n.base);
    if (get_symbol(n.base)->parent() == f->parent()) {
        base_name = fmt::format(
            "{}.{}",
            get_local_identifier(
                mod, recursive_definition_lookup(mod.get_scope(), f->parent()).value()),
            get_local_identifier(mod, n.base));
        assert(n.args.empty());
    }

    if (n.args.empty()) {
        return std::string(base_name);
    }

    std::vector<std::string> generic_args(n.args.size());
    std::transform(
        n.args.begin(),
        n.args.end(),
        generic_args.begin(),
        [&](const generic_argument& arg) {
            return std::visit(
                [&](auto& arg) {
                    if constexpr (!std::is_convertible_v<decltype(arg), int64_t>) {
                        return get_local_identifier(mod, from, arg);
                    } else {
                        return get_identifier(mod, arg);
                    }
                },
                arg.get_variant());
        });

    return fmt::format("{}({})", base_name, fmt::join(generic_args, ", "));
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

    return fmt::format("{}({})", base_name, fmt::join(generic_args, ", "));
}

std::string import_string(const module& mod, const base* sym) {
    auto abs_name =
        python_import_name(recursive_definition_lookup(mod.get_scope(), sym).value());
    if (abs_name.size() == 1) {
        // Must be a lidl type, already imported
        return "";
    }
    return fmt::format("{}", fmt::join(abs_name, "."));
}
} // namespace lidl::py