#pragma once

#include "generator_base.hpp"
#include <lidl/structure.hpp>

namespace lidl::cpp {
struct struct_gen : generator_base<structure> {
    using generator_base::generator_base;

    codegen::sections generate() override {
        return do_generate();
    }

private:
    codegen::sections do_generate();

    codegen::sections generate_traits();
};
}