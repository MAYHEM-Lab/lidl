#pragma once

#include "generator_base.hpp"

#include <lidl/structure.hpp>

namespace lidl::js {
class struct_gen : codegen::generator_base<structure> {
public:
    using generator_base::generator_base;

    codegen::sections generate() override;

private:

    std::string generate_member(std::string_view mem_name, const member& mem);

    std::string generate_ctor() const;

    std::string generate_layout() const;
    std::string generate_members() const;
};
}