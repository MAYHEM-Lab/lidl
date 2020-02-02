#include "tests_generated.hpp"

#include <array>
#include <iostream>
#include <lidl/builder.hpp>
#include <lidl/string.hpp>
#include <numeric>

int main() {
    std::array<uint8_t, 64> x;
    lidl::message_builder builder(x);
    auto& p = lidl::create<string_and_vector>(builder, builder,
        lidl::create_string(builder, "hello"),
        lidl::create_vector<uint16_t>(builder, 3));
    p.numbers.unsafe()->span()[0] = 42;
    p.numbers.unsafe()->span()[1] = 84;
    p.numbers.unsafe()->span()[2] = 168;

    std::cout.write(reinterpret_cast<const char*>(builder.get_buffer().get_buffer().data()),
        builder.get_buffer().get_buffer().size());
    std::cerr << "message took " << builder.size() << " bytes\n";
}