#include "parser.hpp"

#include <iostream>

int main(int argc, char** argv) {
    constexpr auto in = R"__(enum vec3f : base {
    foo,
    bar = 5,
    yolo,
}


union vec3f : base {
    x: vec<vec<f32>>;
    // some member
    y: f32;
    z: f32;
}

// some vector
struct vec3f : base {
    x: vec<vec<f32>>;
    y: f32;
    z: f32;
})__";

    auto mod = lidl::frontend::parse_module(in);
    if (!mod) {
        return 1;
    }

    std::cerr << mod->elements.size() << '\n';
}