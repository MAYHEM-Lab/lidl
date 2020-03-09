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

int main(int argc, char** argv) {
    using namespace lidl;
    std::ifstream schema(argv[1]);
    auto& mod = yaml::load_module(schema);
    service_pass(mod);
    reference_type_pass(mod);
    union_enum_pass(mod);
    auto root = std::get<const type*>(get_symbol(*recursive_name_lookup(*mod.symbols, argv[2])));
    std::ifstream datafile(argv[3]);
    std::vector<uint8_t> data(std::istream_iterator<uint8_t>(datafile), std::istream_iterator<uint8_t>{});
    auto [yaml, consumed] = root->bin2yaml(mod, data);
    std::cout << yaml << '\n';
}