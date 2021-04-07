#pragma once

#include "generator.hpp"
#include "generator_base.hpp"

namespace lidl::cpp {
class better_service_generator : public codegen::generator_base<service> {
public:
    using generator_base::generator_base;

    codegen::sections generate() override;

private:
    codegen::sections generate_sync_server();
    codegen::sections generate_async_server();
    codegen::sections generate_traits();
    codegen::sections generate_wire_types();
    codegen::sections generate_stub();
    codegen::sections generate_async_stub();
    codegen::sections generate_zerocopy_stub();
    codegen::sections generate_async_zerocopy_stub();

    std::string copy_proc_param(const procedure& proc,
                                std::string_view param_name,
                                const lidl::parameter& param);

    std::string copy_and_return(const procedure& proc, bool async);

    std::string make_procedure_stub(std::string_view proc_name,
                                             const procedure& proc,
                                             bool async);
    std::string make_zerocopy_procedure_stub(std::string_view proc_name,
                                             const procedure& proc,
                                             bool async);
};

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