#pragma once

#include "generator.hpp"
#include "generator_base.hpp"

namespace lidl::cpp {
class remote_stub_generator : public generator_base<service> {
public:
    using generator_base::generator_base;

    sections generate() override;

private:
    std::string copy_proc_param(std::string_view param_name,
                                const lidl::name& param_type);
    std::string make_procedure_stub(std::string_view proc_name, const procedure& proc);
};
} // namespace lidl::cpp