#include <gsl/span>
#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/yaml.hpp>
#include <string_view>
#include <fstream>

namespace lidl {
void generate(const module& mod, std::ostream& str);
void run(gsl::span<std::string> args) {
    auto ym = yaml::load_module(args.size() > 1 ? args[1] : "C:\\Users\\mfati\\lidl\\examples\\strings.yaml");
    if (args.size() <= 2) {
        generate(ym, std::cout);
    } else {
        std::ofstream out_file(args[2]);
        generate(ym, out_file);
    }
}
} // namespace lidl

int main(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);
    lidl::run(args);
}
