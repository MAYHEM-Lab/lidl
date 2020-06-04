#pragma once

#include "generator_base.hpp"
#include <lidl/enumeration.hpp>

namespace lidl::cpp {
struct enum_gen : generator_base<enumeration> {
    using generator_base::generator_base;

    sections generate() override;

private:
    sections do_generate();
    sections generate_traits();
};
} // namespace lidl::cpp