#include "union_generated.hpp"

#include <iostream>

int main() {
    std::array<uint8_t, 128> buf;
    lidl::message_builder builder(buf);
    auto& p = lidl::create<module::procedures>(builder, 42);
    auto& p_str =
        lidl::create<module::procedures>(builder, lidl::create_string(builder, "foo"));
    std::cout << lidl::nameof(p.alternative()) << '\n';
    std::cout << lidl::nameof(p_str.alternative()) << '\n';

    builder = lidl::message_builder(buf);
    auto& raw = lidl::create<module::raw>(builder, 42);
    std::cout << raw.x();
}
