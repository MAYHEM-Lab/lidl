#include "generics_generated.hpp"

#include <iostream>

int main() {
    std::array<uint8_t, 128> buf;
    lidl::message_builder builder(buf);

    auto& f = lidl::create<lidl::example::foo<lidl::string>>(
        builder, lidl::create_string(builder, "hello"));
    std::cout << f.t().string_view() << '\n';

    module::maybe<uint16_t> mmmmm(module::none{});
    visit(
        [](auto value) {
            if constexpr (std::is_same_v<module::none, decltype(value)>) {
                std::cout << "none\n";
            } else {
                std::cout << "yes: " << value << '\n';
            }
        },
        mmmmm);
}