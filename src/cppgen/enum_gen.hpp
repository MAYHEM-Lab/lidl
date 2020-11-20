#pragma once

#include "generator_base.hpp"
#include <lidl/enumeration.hpp>

namespace lidl::cpp {
struct enum_gen : codegen::generator_base<enumeration> {
    using generator_base::generator_base;

    codegen::sections generate() override;

private:
    codegen::sections do_generate();
    codegen::sections generate_traits();
};
} // namespace lidl::cpp