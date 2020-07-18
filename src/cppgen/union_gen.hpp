#pragma once

#include "generator_base.hpp"
#include <lidl/union.hpp>

namespace lidl::cpp {
struct union_gen : generator_base<union_type> {
    using generator_base::generator_base;

    sections generate() override;

private:

    sections generate_traits();

    std::string
    generate_getter(std::string_view member_name, const member& mem, bool is_const);
};
}