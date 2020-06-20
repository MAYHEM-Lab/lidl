#include "strings_generated.hpp"

#include <array>
#include <iostream>
#include <lidl/builder.hpp>
#include <lidl/string.hpp>

int main() {
    std::array<uint8_t, 64> x;
    lidl::message_builder builder(x);
    auto& p = lidl::create<module::person>(builder,
        lidl::create_string(builder, "foo"),
        lidl::create_string(builder, "bar"));

    std::cout << p.name().string_view() << '\n'
              << p.surname().string_view() << '\n';

    std::cout << "message took " << builder.size() << " bytes\n";
}