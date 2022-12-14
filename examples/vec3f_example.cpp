#include "vec3f_generated.hpp"

#include <array>
#include <iostream>
#include <lidlrt/builder.hpp>

constexpr gfx::vec3f v(1, 2, 3);

int main() {
    std::array<uint8_t, 64> buf;
    lidl::message_builder builder(buf);
    auto& vec = lidl::emplace_raw<gfx::vec3f>(builder, 1.f, 2.f, 3.f);
    const auto& [x, y, z] = vec;
    std::cout << vec.x() << ", " << vec.y() << ", " << vec.z() << '\n';
    std::cout << x << ", " << y << ", " << z << '\n';
    std::cout << "message took " << builder.size() << " bytes\n";
}