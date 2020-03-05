#include "lidl/union.hpp"

#include <fstream>
#include <functional>
#include <gsl/span>
#include <iostream>
#include <lidl/basic.hpp>
#include <lyra/lyra.hpp>
#include <string_view>
#include <yaml.hpp>


namespace lidl {
void generate(const module& mod, std::ostream& str);
std::unordered_map<std::string_view, std::function<void(const module&, std::ostream&)>>
    backends{{"cpp", generate}};

struct lidlc_args {
    std::istream* input_stream;
    std::ostream* output_stream;
    std::string backend;
};

void union_enum_pass(module& m);
void reference_type_pass(module& m);
void service_pass(module& m);
void run(const lidlc_args& args) {
    auto backend = backends.find(args.backend);
    if (backend == backends.end()) {
        std::cerr << fmt::format("Unknown backend: {}\n", args.backend);
        return;
    }
        auto& ym = yaml::load_module(*args.input_stream);
        service_pass(ym);
        reference_type_pass(ym);
        union_enum_pass(ym);
        backend->second(ym, *args.output_stream);

}
} // namespace lidl

int main(int argc, char** argv) {
    bool help = false;
    bool read_from_stdin = false;
    std::string input_path;
    std::string out_path;
    std::string backend;
    auto cli =
        lyra::cli_parser() |
        lyra::opt(input_path, "input")["-f"]["--input-file"]("Input file to read from.")
            .optional() |
        lyra::opt(read_from_stdin)["-i"]["--stdin"](
            "Read the input from the standard input.")
            .optional() |
        lyra::opt(out_path,
                  "output file")["-o"]["--output-file"]("Output file to write to.")
            .optional() |
        lyra::opt(backend, "backend")["-g"]["--backend"]("Backend to use.").required() |
        lyra::help(help);
    auto res = cli.parse({argc, argv});

    if (!res) {
        std::cerr << res.errorMessage() << '\n';
        return -1;
    }

    if (help) {
        std::cout << cli << '\n';
        return 0;
    }

    if (input_path.empty() && !read_from_stdin) {
        std::cerr
            << "Either provide a filename to read from or run with -i to read from stdin";
        return -1;
    }

    lidl::lidlc_args args;
    args.input_stream = &std::cin;
    args.output_stream = &std::cout;

    if (!input_path.empty()) {
        args.input_stream = new std::ifstream{input_path};
        if (!args.input_stream->good()) {
            throw std::runtime_error("File not found: " + input_path);
        }
    }
    if (!out_path.empty()) {
        args.output_stream = new std::ofstream{out_path};
        if (!args.input_stream->good()) {
            throw std::runtime_error("File not found: " + input_path);
        }
    }
    args.backend = std::move(backend);

    lidl::run(args);

    args.output_stream->flush();
}
