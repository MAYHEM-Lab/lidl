#include "lidl/union.hpp"

#include <codegen.hpp>
#include <filesystem>
#include <fstream>
#include <functional>
#include <gsl/span>
#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/loader.hpp>
#include <lyra/lyra.hpp>
#include <string_view>
#include <yaml.hpp>

namespace lidl {
#if defined(ENABLE_CPP)
namespace cpp {
std::unique_ptr<codegen::backend> make_backend();
}
#endif
#if defined(ENABLE_JS)
namespace js {
std::unique_ptr<codegen::backend> make_backend();
}
#endif
#if defined(ENABLE_PY)
namespace py {
std::unique_ptr<codegen::backend> make_backend();
}
#endif
std::unordered_map<std::string_view, std::function<std::unique_ptr<codegen::backend>()>>
    backends {
#if defined(ENABLE_CPP)
    {"cpp", cpp::make_backend},
#endif
#if defined(ENABLE_JS)
    {"js", js::make_backend}, {"ts", js::make_backend},
#endif
#if defined(ENABLE_PY)
    {"py", py::make_backend},
#endif
};

struct lidlc_args {
    std::istream* input_stream;
    std::ostream* output_stream;
    std::string backend;
    std::optional<std::string> origin;
    std::vector<std::string> import_paths;
    bool just_details = false;
};

void run(const lidlc_args& args) {
    auto importer = std::make_unique<lidl::path_resolver>();
    for (auto& path : args.import_paths) {
        importer->add_import_path(path);
    }

    lidl::load_context ctx;
    ctx.set_importer(std::move(importer));
    auto mod = ctx.do_import(*args.origin, "");

    if (!mod) {
        std::cerr << "Module parsing failed!\n";
        exit(1);
    }

    if (args.just_details) {
        std::cerr << "Symbols:\n";
        mod->parent->symbols().dump(std::cerr);
        std::cerr << "---\n";
        YAML::Node res;
        res["imports"] = {};
        for (auto& [path, _] : ctx.import_mapping) {
            res["imports"].push_back(path);
        }
        std::cout << res << '\n';
        return;
    }

    auto backend_maker = backends.find(args.backend);
    if (backend_maker == backends.end()) {
        std::cerr << fmt::format("Unknown backend: {}\n", args.backend);
        std::vector<std::string_view> names(backends.size());
        std::transform(backends.begin(), backends.end(), names.begin(), [](auto& be) {
            return be.first;
        });
        std::cerr << fmt::format("Possible backends: {}", fmt::join(names, ", "));
        exit(1);
    }

    auto backend = backend_maker->second();
    backend->generate(*mod, *args.output_stream);
    args.output_stream->flush();
    exit(0);
}
} // namespace lidl

int main(int argc, char** argv) {
    bool version         = false;
    bool help            = false;
    bool read_from_stdin = false;
    bool build_details   = false;
    std::string input_path;
    std::string out_path;
    std::string backend;
    std::vector<std::string> import_paths;
    auto cli =
        lyra::cli_parser() |
        lyra::opt(input_path, "input")["-f"]["--input-file"]("Input file to read from.")
            .optional() |
        lyra::opt(import_paths, "import_paths")["-I"]("Import prefixes") |
        lyra::opt(read_from_stdin)["-i"]["--stdin"](
            "Read the input from the standard input.")
            .optional() |
        lyra::opt(out_path,
                  "output file")["-o"]["--output-file"]("Output file to write to.")
            .optional() |
        lyra::opt(backend, "backend")["-g"]["--backend"]("Backend to use.") |
        lyra::opt(build_details, "build details")["-d"].optional() |
        lyra::opt(version)["--version"]("Print lidl version") | lyra::help(help);
    auto res = cli.parse({argc, argv});

    if (version) {
        std::cerr << "Lidl version: " << LIDL_VERSION_STRING << '\n';
        return 0;
    }

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
    args.input_stream  = &std::cin;
    args.output_stream = &std::cout;

    auto path = std::filesystem::canonical(std::filesystem::current_path() / input_path);
    if (!input_path.empty()) {
        args.input_stream = new std::ifstream{path.string()};
        if (!args.input_stream->good()) {
            throw std::runtime_error("File not found: " + path.string());
        }
        args.origin = path.string();
    }

    if (!out_path.empty()) {
        args.output_stream = new std::ofstream{out_path};
        if (!args.output_stream->good()) {
            throw std::runtime_error("File not found: " + out_path);
        }
    }

    args.backend = std::move(backend);

    args.import_paths = std::move(import_paths);
    args.just_details = build_details;

    lidl::run(args);

    args.output_stream->flush();

    if (!input_path.empty()) {
        delete args.input_stream;
    }
    if (!out_path.empty()) {
        delete args.output_stream;
    }
}
