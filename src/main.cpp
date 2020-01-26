#include <fstream>
#include <gsl/span>
#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/yaml.hpp>
#include <string_view>

namespace lidl {
structure service_params_struct(const module& mod,
                                std::string_view serv,
                                std::string_view name,
                                const procedure& proc) {
    structure s;
    for (auto& [name, param] : proc.parameters) {
        member m;
        m.type_ = param;
        s.members.emplace_back(name, m);
    }
    s.attributes.add(std::make_shared<procedure_params_attribute>(
        std::string(serv), std::string(name), proc));
    return s;
}

void service_pass(module& mod) {
    for (auto& [name, service] : mod.services) {
        for (auto& [proc_name, proc] : service.procedures) {
            auto args_struct = service_params_struct(mod, name, proc_name, proc);
            mod.structs.emplace_back(fmt::format("{}_params_t", proc_name), args_struct);
        }
    }
}

void generate(const module& mod, std::ostream& str);
void run(gsl::span<std::string> args) {
    auto ym = yaml::load_module(
        args.size() > 1 ? args[1] : "C:\\Users\\mfati\\lidl\\examples\\vec3f.yaml");
    service_pass(ym);
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
