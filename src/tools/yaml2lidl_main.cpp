//
// Created by fatih on 1/31/20.
//

#include "passes.hpp"

#include <fstream>
#include <lidl/buffer.hpp>
#include <lidl/module.hpp>
#include <yaml-cpp/yaml.h>
#include <yaml.hpp>

struct ostream_writer : lidl::ibinary_writer {
    std::ostream* str;
    int written = 0;

    void write_raw(gsl::span<const char> data) override {
        str->write(data.data(), data.size());
        written += data.size();
    }

    int tell() override {
        return written;
    }
};

struct yaml2lidl_args {
    std::string root_type;
    std::string schema_path;
    std::string yaml_path;
};

void yaml2lidl(yaml2lidl_args args) {
    using namespace lidl;

    auto root_mod = std::make_unique<module>();
    root_mod->add_child("", basic_module());

    std::ifstream datafile(args.yaml_path);
    YAML::Node yaml_root = YAML::Load(datafile);
    if (auto meta = yaml_root["$lidlmeta"]; meta) {
        if (auto root = meta["root_type"]; root) {
            args.root_type = root.as<std::string>();
        }
    }

    if (args.schema_path.empty()) {
        std::cerr << "Missing schema path!\n";
        exit(1);
    }
    if (args.root_type.empty()) {
        std::cerr << "Missing root type!\n";
        exit(1);
    }

    std::ifstream schema(args.schema_path);
    auto& mod = yaml::load_module(*root_mod, schema);

    run_passes_until_stable(mod);

    auto root = std::get<const type*>(
        get_symbol(*recursive_name_lookup(*mod.symbols, args.root_type)));

    ostream_writer output;
    output.str = &std::cout;

    root->yaml2bin(mod, yaml_root, output);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << fmt::format("Usage {} path_to_yaml path_to_schema [root_type]\n",
                                 argv[1]);
    }

    yaml2lidl_args args;
    args.yaml_path = argv[1];

    if (argc > 2)
        args.schema_path = argv[2];
    if (argc > 3)
        args.root_type = argv[3];

    yaml2lidl(std::move(args));
}