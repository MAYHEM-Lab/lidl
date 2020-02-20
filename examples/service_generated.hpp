#pragma once

#include <lidl/lidl.hpp>
#include<lidl/service.hpp>

class calculator {
public:
virtual ::lidl::status<double> add(const double& left, const double& right) = 0;
virtual ::lidl::status<double> multiply(const double& left, const double& right) = 0;
virtual ~calculator() = default;
};

namespace lidl {
template <> class service_descriptor<calculator> {
public:
std::tuple<::lidl::procedure_descriptor<&calculator::add>, ::lidl::procedure_descriptor<&calculator::multiply>> procedures{{"add"}, {"multiply"}};
std::string_view name = "calculator";
};
}
namespace lidl {
template <> struct procedure_params_t<&calculator::add> {
        
    public:
        procedure_params_t(const double& p_left, const double& p_right) : raw{p_left, p_right} {}
        const double& left() const { return raw.left; }
double& left() { return raw.left; }
const double& right() const { return raw.right; }
double& right() { return raw.right; }

    private:
        struct raw_t {
        raw_t(const double& p_left, const double& p_right) : left(p_left), right(p_right) {}
        double left;
double right;

    };

        raw_t raw;
    

    };

static_assert(sizeof(procedure_params_t<&calculator::add>) == 16, "Size of procedure_params_t<&calculator::add> does not match the wire size!");
static_assert(alignof(procedure_params_t<&calculator::add>) == 8, "Alignment of procedure_params_t<&calculator::add> does not match the wire alignment!");

}
namespace lidl {
template <> struct procedure_params_t<&calculator::multiply> {
        
    public:
        procedure_params_t(const double& p_left, const double& p_right) : raw{p_left, p_right} {}
        const double& left() const { return raw.left; }
double& left() { return raw.left; }
const double& right() const { return raw.right; }
double& right() { return raw.right; }

    private:
        struct raw_t {
        raw_t(const double& p_left, const double& p_right) : left(p_left), right(p_right) {}
        double left;
double right;

    };

        raw_t raw;
    

    };

static_assert(sizeof(procedure_params_t<&calculator::multiply>) == 16, "Size of procedure_params_t<&calculator::multiply> does not match the wire size!");
static_assert(alignof(procedure_params_t<&calculator::multiply>) == 8, "Alignment of procedure_params_t<&calculator::multiply> does not match the wire alignment!");

}
