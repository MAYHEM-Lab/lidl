#include "service_generated.hpp"

#include <functional>
#include <iostream>
#include <unordered_map>

class calculator_impl : public calculator {
public:
    ::lidl::status<double> add(const double& left, const double& right) override {
        return left + right;
    }

    ::lidl::status<double> multiply(const double& left, const double& right) override {
        return left * right;
    }
};

template<auto Fn>
auto inspect_procedure(size_t order, const lidl::procedure_descriptor<Fn>& descriptor) {
    std::cout << '\t' << descriptor.name << '\n';
}

template<auto... x, size_t... Is>
auto inspect_procedures_impl(const std::tuple<lidl::procedure_descriptor<x>...>& procs,
                             std::index_sequence<Is...>) {
    (inspect_procedure(Is, std::get<Is>(procs)), ...);
}

template<auto... x>
auto inspect_procedures(const std::tuple<lidl::procedure_descriptor<x>...>& procs) {
    inspect_procedures_impl(procs, std::make_index_sequence<sizeof...(x)>{});
}

template<class Service>
auto inspect_service() {
    constexpr lidl::service_descriptor<Service> descriptor;
    std::cout << descriptor.name << '\n';
    inspect_procedures(descriptor.procedures);
}

template<class...>
struct member_fn_traits;
template<class Type, class RetType, class... ArgTypes>
struct member_fn_traits<RetType (Type::*)(ArgTypes...)> {
    using return_type = RetType;
    using argument_types = std::tuple<ArgTypes...>;
};

struct call_info_base {
    explicit call_info_base(::lidl::string& p_name)
        : name(p_name) {
    }
    ::lidl::ptr<::lidl::string> name;
};

template<auto Fn>
struct call_info : call_info_base {
    explicit call_info(::lidl::string& p_name, ::lidl::procedure_params_t<Fn>& p_params)
        : call_info_base(p_name)
        , params(p_params) {
    }
    ::lidl::ptr<::lidl::procedure_params_t<Fn>> params;
};

template<auto Fn>
auto& serialize_call(const lidl::procedure_descriptor<Fn>& desc,
                     const typename member_fn_traits<decltype(Fn)>::argument_types& args,
                     lidl::message_builder& builder) {
    auto& name = lidl::create_string(builder, desc.name);
    auto& args_placed = lidl::append_raw(
        builder, std::make_from_tuple<lidl::procedure_params_t<Fn>>(args));
    return lidl::emplace_raw<call_info<Fn>>(builder, name, args_placed);
}

template<class T, auto... x, size_t... Is>
auto make_lookup_impl(const std::tuple<lidl::procedure_descriptor<x>...>& procs,
                      std::index_sequence<Is...>) {
    std::unordered_map<std::string_view, std::function<void(T&, const call_info_base&)>>
        handlers;
    ((handlers[std::get<Is>(procs).name] =
          [](T& t, const call_info_base& info) {
              auto& params = static_cast<const call_info<x>&>(info).params.unsafe().get();
              auto& [a0, a1] = params;
              std::cout << *(t.*(x))(a0, a1);
          }),
     ...);
    return handlers;
}

template<class T, auto... x>
auto make_lookup(const std::tuple<lidl::procedure_descriptor<x>...>& procs) {
    return make_lookup_impl<T>(procs, std::make_index_sequence<sizeof...(x)>{});
}

template<class Service>
auto deserialize_call(Service& impl,
                      const lidl::buffer& call,
                      const call_info_base& msg) {
    constexpr lidl::service_descriptor<Service> desc;
    const auto handlers = make_lookup<Service>(desc.procedures);
    auto name = msg.name.unsafe()->unsafe_string_view();
    std::cout << name << '\n';

    if (auto it = handlers.find(name); it != handlers.end()) {
        it->second(impl, msg);
    } else {
        throw std::runtime_error("Non-existent procedure: " +
                                 std::string(name));
    }
}

int main() {
    std::array<uint8_t, 64> buf;
    lidl::message_builder builder(buf);

    auto& serialized_res = serialize_call<&calculator::multiply>(
        std::get<1>(lidl::service_descriptor<calculator>().procedures),
        std::tuple{10., 5.},
        builder);

    calculator_impl c;
    deserialize_call<calculator>(c, builder.get_buffer(), serialized_res);
}