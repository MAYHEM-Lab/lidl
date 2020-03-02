//
// Created by fatih on 1/31/20.
//

#include <fstream>
#include <lidl/buffer.hpp>
#include <lidl/module.hpp>
#include <yaml-cpp/yaml.h>
#include <yaml.hpp>

namespace lidl {
void service_pass(module& m);
void reference_type_pass(module& m);
void union_enum_pass(module& m);
} // namespace lidl

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
    std::ifstream schema(argv[1]);
    auto mod = yaml::load_module(schema);
    service_pass(mod);
    reference_type_pass(mod);
    union_enum_pass(mod);
    auto root = std::get<const type*>(get_symbol(*mod.symbols->name_lookup(argv[2])));
    std::ifstream datafile(argv[3]);
    YAML::Node yaml_root = YAML::Load(datafile);
    ostream_writer output;
    output.str = &std::cout;
    root->yaml2bin(mod, yaml_root, output);
}