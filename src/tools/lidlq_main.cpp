#include <filesystem>
#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/loader.hpp>
#include <lidl/module.hpp>
#include <lyra/lyra.hpp>

namespace {
enum class output_format
{
    yaml,
};

struct query_base {
    virtual void perform(const lidl::module& mod, output_format format) = 0;
    virtual ~query_base()                                               = default;
};

struct list_services : query_base {
    void perform(const lidl::module& mod, output_format format) override {
        for (auto& serv : mod.services) {
            auto sym =
                *recursive_definition_lookup(root_module(mod).get_scope(), serv.get());
            auto name = lidl::local_name(sym);
            std::cout << name << '\n';
        }
    }
};

struct lidlq_args {
    std::string input_path;
    std::vector<std::string> import_paths;
    std::unique_ptr<query_base> query;
    output_format format = output_format::yaml;
};

void run(const lidlq_args& args) {
    auto importer = std::make_shared<lidl::path_resolver>();
    for (auto& path : args.import_paths) {
        importer->add_import_path(path);
    }

    lidl::load_context ctx;
    ctx.set_importer(std::move(importer));
    auto mod = ctx.do_import(args.input_path, "");

    if (!mod) {
        std::cerr << "Module parsing failed!\n";
        exit(1);
    }

    std::cerr << "Parsing succeeded\n";

    args.query->perform(*mod, args.format);
}
} // namespace

int main(int argc, char** argv) {
    std::string verb = argv[1];

    bool help = false;
    std::string input_path;
    std::vector<std::string> import_paths;
    auto cli =
        lyra::cli_parser() |
        lyra::opt(input_path, "input")["-f"]["--input-file"]("Input file to read from.")
            .optional() |
        lyra::opt(import_paths, "import_paths")["-I"]("Import prefixes") |
        lyra::help(help);
    auto res = cli.parse({argc - 2, argv + 2});

    if (!res) {
        std::cerr << res.errorMessage() << '\n';
        return -1;
    }

    if (help) {
        std::cout << cli << '\n';
        return 0;
    }

    lidlq_args args;
    auto path = std::filesystem::canonical(std::filesystem::current_path() / input_path);

    args.input_path   = path.string();
    args.import_paths = import_paths;
    args.query        = std::make_unique<list_services>();

    run(args);
}