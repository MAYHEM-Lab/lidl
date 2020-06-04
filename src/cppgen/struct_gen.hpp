#pragma once

#include "generator_base.hpp"
#include <lidl/structure.hpp>

namespace lidl::cpp {
struct struct_gen : generator_base<structure> {
    using generator_base::generator_base;

    sections generate() override {
        return do_generate();
    }

private:
    sections do_generate();

    sections generate_traits();
};
}