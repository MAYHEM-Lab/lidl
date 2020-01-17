#include <gsl/span>
#include <iostream>
#include <proto/basic.hpp>
#include <proto/yaml.hpp>
#include <string_view>

namespace proto {
void generate(const module& mod, std::ostream& str);
const type* basic_type_by_name(std::string_view name);
void init_basic_types();
void run(gsl::span<std::string> args) {
    auto ym = yaml::load_module("/home/fatih/proto/examples/vec3f.yaml");
    generate(ym, std::cout);
}
} // namespace proto

int main(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);
    proto::run(args);
}
