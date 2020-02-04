//
// Created by fatih on 1/31/20.
//

#include <fstream>
#include <lidl/buffer.hpp>
#include <lidl/module.hpp>
#include <yaml-cpp/yaml.h>
#include <yaml.hpp>

namespace lidl {
namespace {
struct args {
    module mod;
    const type* root_type;
    buffer input_buffer;
};

void bin2yaml(const args& arg) {
}
} // namespace

void reference_type_pass(module& m);
} // namespace lidl

int main() {
    using namespace lidl;
    std::ifstream f("/home/fatih/lidl/examples/lidl.yaml");
    auto mod = yaml::load_module(f);
    reference_type_pass(mod);
    auto root = std::get<const type*>(get_symbol(*mod.symbols->name_lookup("type")));
    uint8_t a[] = {0x62, 0x61, 0x72, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00,
                   0x66, 0x6f, 0x6f, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x02, 0x00, 0x06, 0x00,
                   0x04, 0x00, 0x04, 0x00};
    auto [yaml, consumed] = root->bin2yaml(mod, a);
    std::cout << yaml << '\n';
}