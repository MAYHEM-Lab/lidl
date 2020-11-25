#pragma once

#include "generator_base.hpp"

#include <lidl/enumeration.hpp>

namespace lidl::js {
class enum_gen : public codegen::generator_base<enumeration> {
public:
    using generator_base::generator_base;

    codegen::sections generate() override;
};
} // namespace lidl::js
