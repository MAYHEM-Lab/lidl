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
} // namespace lidl

int main() {
    using namespace lidl;
    std::ifstream f("/home/fatih/lidl/examples/lidl.yaml");
    auto mod = yaml::load_module(f);
    auto root = std::get<const type*>(*mod.symbols.lookup("type"));
    std::array<uint8_t, 20> a{0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00, 0x06, 0x00, 0x2a, 0x00,
                              0x54, 0x00, 0xa8, 0x00, 0x06, 0x00, 0x0a, 0x00, 0x04, 0x00};
    auto [yaml, consumed] = (*root).bin2yaml(mod, a);
    std::cout << yaml << '\n';
}