#pragma once

#include "generator_base.hpp"

namespace lidl::js {
class union_gen : public codegen::generator_base<union_type> {
public:
    using generator_base::generator_base;

    codegen::sections generate() override;

private:

    std::string generate_member(std::string_view mem_name, const member& mem);

    std::string generate_ctor() const;

    std::string generate_members() const;
};
} // namespace lidl::js
