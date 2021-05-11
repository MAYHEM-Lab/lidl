#include "lexer.hpp"
#include "parser2.hpp"

#include <iostream>

int main(int argc, char** argv) {
    constexpr auto in = R"__(
namespace lidl::foo;

import "./std";

/*
    a block comment
*/
union vec3f {
    x: vec<vec<f32>>;
    // some member
    y: f32;
    z: f32;
}

enum vec3f : base {
    foo,
    bar = 5,
    yolo,
}

// some vector
struct vec3f : base {
    x: vec<vec<f32>>;
    y: f32;
    z: f32;
})__";

    for (auto tok : lidl::frontend::tokenize(in)) {
        std::cerr << tok.content << '\n';
    }

    auto mod = lidl::frontend::parse_module(in);
    std::cerr << mod->elements.size() << '\n';
}