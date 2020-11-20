#pragma once

#include "generator_base.hpp"
#include <lidl/union.hpp>

namespace lidl::cpp {
struct union_gen : codegen::generator_base<union_type> {
    using generator_base::generator_base;

    codegen::sections generate() override;

private:

    codegen::sections generate_traits();

    std::string
    generate_getter(std::string_view member_name, const member& mem, bool is_const);
};
}