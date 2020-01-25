#include "tests_generated.hpp"

#include <array>
#include <iostream>
#include <lidl/builder.hpp>
#include <lidl/string.hpp>
#include <numeric>

int main() {
    std::array<uint8_t, 64> x;
    lidl::message_builder builder(x);
    auto& p = lidl::create<basic_vector>(builder, builder,
        lidl::create_vector<float>(builder, 5));

    std::iota(p.vec.unsafe()->span().begin(), p.vec.unsafe()->span().end(), 1);
    std::cout << "Size: " << p.vec.unsafe()->span().size() << '\n';

    for (float f : p.vec.unsafe()->span()) {
        std::cout << f << '\n';
    }

    std::cout << "message took " << builder.size() << " bytes\n";
}