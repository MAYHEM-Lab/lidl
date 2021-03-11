#pragma once

#include "generator.hpp"
#include "generator_base.hpp"

namespace lidl::cpp {
class service_generator : public codegen::generator_base<service> {
public:
    using generator_base::generator_base;

    codegen::sections generate() override;
};

class async_service_generator : public codegen::generator_base<service> {
public:
    using generator_base::generator_base;

    codegen::sections generate() override;
};

class zerocopy_stub_generator : public codegen::generator_base<service> {
public:
    using generator_base::generator_base;

    codegen::sections generate() override;

private:
    std::string make_procedure_stub(std::string_view proc_name, const procedure& proc);
};

class remote_stub_generator : public codegen::generator_base<service> {
public:
    using generator_base::generator_base;

    codegen::sections generate() override;

private:
    std::string copy_proc_param(const procedure& proc,
                                std::string_view param_name,
                                const lidl::parameter& param);

    std::string copy_and_return(const procedure& proc);

    std::string make_procedure_stub(std::string_view proc_name, const procedure& proc);
};
} // namespace lidl::cpp