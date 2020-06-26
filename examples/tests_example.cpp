#include "tests_generated.hpp"

#include <array>
#include <iostream>
#include <lidl/builder.hpp>
#include <lidl/string.hpp>
#include <numeric>

int main() {
    std::array<uint8_t, 64> x;
    lidl::message_builder builder(x);
    auto& p = lidl::create<module::complex_vector>(
        builder,
        lidl::create_vector(builder,
                            lidl::create_string(builder, "foo"),
                            lidl::create_string(builder, "bar")));

    for (auto& str : p.vec()) {
        std::cout << str.string_view() << '\n';
    }

    std::cout.write(
        reinterpret_cast<const char*>(builder.get_buffer().get_buffer().data()),
        builder.get_buffer().get_buffer().size());
    std::cerr << "message took " << builder.size() << " bytes\n";
}