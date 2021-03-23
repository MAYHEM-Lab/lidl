#pragma once

#include <lidl/module_meta.hpp>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>

namespace lidl {
class module_loader;
struct load_context;

// Instances of this type is used to locate modules with their name.
// Currently we only have a local path resolver, however, in the future, it'll be possible
// to import modules with a URL etc. and for those, we'll need other implementations.
struct import_resolver {
    virtual auto
    resolve_import(load_context& ctx, std::string_view import_name, std::string_view wd)
        -> std::pair<std::unique_ptr<module_loader>, std::string> = 0;

    virtual ~import_resolver() = default;
};

struct module;

class module_loader {
public:
    virtual void load()                = 0;
    virtual module& get_module()       = 0;
    virtual module_meta get_metadata() = 0;
    virtual ~module_loader()           = default;
};

class path_resolver final : public import_resolver {
public:
    void add_import_path(std::string_view base_path);

    auto resolve_import(load_context& ctx,std::string_view import_name, std::string_view wd)
        -> std::pair<std::unique_ptr<module_loader>, std::string> override;

private:
    std::set<std::string> m_base_paths;
};

struct load_context {
    load_context();

    module* do_import(std::string_view import_name, std::string_view work_dir);

    void set_importer(std::unique_ptr<import_resolver> resolver) {
        importer = std::move(resolver);
    }

    module& get_root() {
        return *root_module;
    }
    std::map<std::string, module_loader*, std::less<>> import_mapping;

private:

    void perform_load(module_loader& mod, std::string_view work_dir);

    std::unique_ptr<import_resolver> importer;

    std::unique_ptr<module> root_module;
    std::set<std::unique_ptr<module_loader>> loaders;
};
} // namespace lidl