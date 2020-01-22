#include "strings_generated.hpp"

#include <array>
#include <iostream>
#include <lidl/builder.hpp>

int main() {
    std::array<uint8_t, 64> x;
    lidl::message_builder builder(x.data(), 64);
    auto& str = lidl::create_string(builder, "hello world");
    auto& str2 = lidl::create_string(builder, "yayy");
    std::cout << str.string_view() << '\n';
    std::cout << str2.string_view() << '\n';
    std::cout << "message took " << builder.size() << " bytes\n";
}