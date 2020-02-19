#include "vec3f_generated.hpp"

#include <array>
#include <iostream>
#include <lidl/builder.hpp>

int main() {
    std::array<uint8_t, 64> x;
    lidl::message_builder builder(x);
    auto& vec = lidl::emplace_raw<vec3f>(builder, 1.f, 2.f, 3.f);
    std::cout << vec.x() << ", " << vec.y() << ", " << vec.z() << '\n';
    std::cout << "message took " << builder.size() << " bytes\n";
}