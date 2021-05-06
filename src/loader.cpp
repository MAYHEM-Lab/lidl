#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <lidl/loader.hpp>
#include <lidl/module.hpp>
#include <optional>

namespace lidl {
std::unique_ptr<module_loader> make_yaml_loader(load_context& root,
                                                std::istream& file,
                                                std::optional<std::string> origin);
std::unique_ptr<module_loader> make_frontend_loader(load_context& root,
                                                    std::istream& file,
                                                    std::optional<std::string> origin);


namespace fs = std::filesystem;

void path_resolver::add_import_path(std::string_view base_path) {
    fs::path p(base_path);
    if (!fs::is_directory(p)) {
        std::cerr << base_path << " is not a directory!";
        return;
    }
    m_base_paths.emplace(base_path);
}

std::optional<fs::path> search_in_path(const fs::path& in, const fs::path& import) {
    auto iter = std::find_if(
        fs::directory_iterator(in), fs::directory_iterator(), [&](const fs::path& entry) {
            if (import.has_extension()) {
                return entry.filename() == import.filename();
            }
            return entry.stem() == import.stem();
        });
    if (iter == fs::directory_iterator()) {
        return {};
    }
    std::cerr << iter->path() << '\n';
    return fs::canonical(*iter);
}

std::pair<std::unique_ptr<module_loader>, std::string> path_resolver::resolve_import(
    load_context& ctx, std::string_view import_name, std::string_view wd) {
    auto import_path = fs::path(import_name);
    std::optional<fs::path> found;

    if (import_path.is_absolute()) {
        if (fs::exists(import_path)) {
            found = fs::canonical(import_path);
        }
    } else if (import_name.substr(0, 2) == "./") {
        // relative import
        found = search_in_path(wd, import_path);
    } else {
        for (auto& base : m_base_paths) {
            if (auto p = search_in_path(base, import_path); p) {
                found = p;
                break;
            }
        }
    }

    if (!found) {
        return {nullptr, ""};
    }

    if (found->extension() != ".yaml" && found->extension() != ".lidl") {
        return {nullptr, found->string()};
    }

    std::ifstream import_file(found->string());
    if (!import_file.good()) {
        std::cerr << "Could not open " << *found << '\n';
        return {nullptr, found->string()};
    }

    if (found->extension() == ".yaml") {
        return {make_yaml_loader(ctx, import_file, found->string()), found->string()};
    }

    return {make_frontend_loader(ctx, import_file, found->string()), found->string()};
}

load_context::load_context() {
    root_module = std::make_unique<module>();
    root_module->add_child("", basic_module());
}

module* load_context::do_import(std::string_view import_name, std::string_view work_dir) {
    auto [loader, key] = importer->resolve_import(*this, import_name, work_dir);
    if (!loader) {
        return nullptr;
    }

    if (auto it = import_mapping.find(key); it != import_mapping.end()) {
        return &it->second->get_module();
    }

    auto [ins_it, ins] = loaders.emplace(std::move(loader));
    import_mapping.emplace(key, ins_it->get());
    perform_load(**ins_it, fs::path(key).parent_path().string());
    return &(*ins_it)->get_module();
}

void load_context::perform_load(module_loader& loader, std::string_view work_dir) {
    auto meta = loader.get_metadata();
    for (auto& import : meta.imports) {
        std::cerr << "Processing import " << import << '\n';
        if (!do_import(import, work_dir)) {
            std::cerr << "Importing " << import << " failed!\n";
            exit(1);
        }
        std::cerr << "Done\n";
    }

    auto& mod = loader.get_module();
    loader.load();

    mod.finalize();
}
} // namespace lidl