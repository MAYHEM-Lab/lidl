#pragma once

#include "generator_base.hpp"
#include <lidl/generics.hpp>

namespace lidl::cpp {

struct generic_gen : generator_base<generic_instantiation> {
    using generator_base::generator_base;

    sections generate() override {
        return do_generate();
    }

private:
    std::string full_name();

    sections do_generate(const generic_structure& str);

    sections do_generate(const generic_union& u);

    sections do_generate();
};
}