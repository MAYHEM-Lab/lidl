//
// Created by fatih on 1/31/20.
//
#include <iostream>
#include <lidl_generated.hpp>

int main() {
    std::array<uint8_t, 64> x;
    lidl::message_builder builder(x);

    auto& id = lidl::create<identifier>(
        builder, builder,
        lidl::create_string(builder, "foo"),
        lidl::create_vector<lidl::ptr<identifier>>(builder, 0));

    auto& p = lidl::create<type>(
        builder,
        builder,
        id);

    std::cout.write(
        reinterpret_cast<const char*>(builder.get_buffer().get_buffer().data()),
        builder.get_buffer().get_buffer().size());
    std::cerr << "message took " << builder.size() << " bytes\n";
}