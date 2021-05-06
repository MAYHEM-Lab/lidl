#pragma once

#include "generator_base.hpp"
#include <lidl/generics.hpp>

namespace lidl::cpp {

struct generic_gen : codegen::generator_base<basic_generic_instantiation> {
    using generator_base::generator_base;

    codegen::sections generate() override {
        return do_generate();
    }

private:
    std::string full_name();
    std::string local_full_name();

    codegen::sections do_generate(const generic_structure& str);

    codegen::sections do_generate(const generic_union& u);

    codegen::sections do_generate();
};
}