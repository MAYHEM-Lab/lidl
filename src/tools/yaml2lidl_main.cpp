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

int main(int argc, char** argv) {
    using namespace lidl;

    auto root_mod = std::make_unique<module>();
    root_mod->add_child("", basic_module());

    std::ifstream schema(argv[1]);
    auto& mod = yaml::load_module(*root_mod, schema);

    run_passes_until_stable(mod);

    auto root =
        std::get<const type*>(get_symbol(*recursive_name_lookup(*mod.symbols, argv[2])));

    std::ifstream datafile(argv[3]);
    YAML::Node yaml_root = YAML::Load(datafile);

    ostream_writer output;
    output.str = &std::cout;

    root->yaml2bin(mod, yaml_root, output);
}