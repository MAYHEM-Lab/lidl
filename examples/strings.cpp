#include "strings_generated.hpp"

#include <array>
#include <iostream>
#include <lidl/builder.hpp>
#include <lidl/string.hpp>

int main() {
    std::array<uint8_t, 64> x;
    lidl::message_builder builder(x);
    auto& p = lidl::create<person>(builder,
        lidl::create_string(builder, "foo"),
        lidl::create_string(builder, "bar"));

    std::cout << p.name().unsafe_string_view() << '\n'
              << p.surname().unsafe_string_view() << '\n';

    std::cout << "message took " << builder.size() << " bytes\n";
}