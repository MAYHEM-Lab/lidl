//
// Created by fatih on 1/31/20.
//

#include "passes.hpp"

#include <fstream>
#include <lidl/buffer.hpp>
#include <lidl/module.hpp>
#include <yaml-cpp/yaml.h>
#include <yaml.hpp>

namespace lidl {
class memory_reader : public ibinary_reader {
public:
    explicit memory_reader(gsl::span<uint8_t> data)
        : m_data{data}
        , m_loc(m_data.size()) {
    }

    using ibinary_reader::seek;
    void seek(int bytes, std::ios::seekdir dir) override {
        if (dir == std::ios::cur) {
            m_loc += bytes;
        } else if (dir == std::ios::beg) {
            m_loc = bytes;
        } else {
            m_loc = m_data.size() - bytes;
        }
    }

    int tell() override {
        return m_loc;
    }

    gsl::span<uint8_t> read(gsl::span<uint8_t> span) override {
        memcpy(span.data(), m_data.data() + m_loc, span.size());
        return span;
    }

    gsl::span<const uint8_t> m_data;
    int m_loc;
};
} // namespace lidl

int main(int argc, char** argv) {
    using namespace lidl;
    auto root_mod = std::make_unique<module>();
    root_mod->add_child("", basic_module());
    std::ifstream schema(argv[1]);
    auto& mod = yaml::load_module(*root_mod, schema);
    run_passes_until_stable(mod);
    auto root = std::get<const type*>(
        get_symbol(recursive_name_lookup(*mod.symbols, argv[2]).value()));

    std::ifstream datafile(argv[3], std::ios::binary);
    std::vector<uint8_t> data(std::istreambuf_iterator<char>(datafile),
                              std::istreambuf_iterator<char>{});

    lidl::memory_reader reader(data);

    reader.seek(-root->wire_layout(mod).size());

    auto yaml = root->bin2yaml(mod, reader);
    std::cout << yaml << '\n';
}