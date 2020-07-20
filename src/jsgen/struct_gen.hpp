#pragma once

#include "generator.hpp"

#include <lidl/structure.hpp>

namespace lidl::js {
class struct_gen : generator<structure> {
public:
    using generator::generator;

    sections generate() override;

private:

    std::string generate_member(std::string_view mem_name, const member& mem);

    std::string generate_metadata();
};
}