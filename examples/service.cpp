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

/*template<auto Fn>
void serialize_call(const typename procedure_traits<decltype(Fn)>::param_types& args,
                    lidl::message_builder& builder) {
    auto desc = std::get<lidl::procedure_descriptor<Fn>>(
        lidl::service_descriptor<calculator>().procedures);
    auto& name = lidl::create_string(builder, desc.name);
    auto& args_placed = lidl::append_raw(
        builder, std::make_from_tuple<lidl::procedure_params_t<Fn>>(args));
    auto& info = lidl::emplace_raw<call_info<Fn>>(builder, name, args_placed);
    lidl::emplace_raw<lidl::ptr<call_info_base>>(builder, info);
}

template<class T, auto... Fn, size_t... Is>
auto make_lookup_impl(const std::tuple<lidl::procedure_descriptor<Fn>...>& procs,
                      std::index_sequence<Is...>) {
    std::unordered_map<std::string_view,
                       std::function<const response_info_base&(
                           T&, const call_info_base&, lidl::message_builder& response)>>
        handlers;
    ((handlers[std::get<Is>(procs).name] =
          [](T& t,
             const call_info_base& info,
             lidl::message_builder& response) -> const response_info_base& {
         auto& params = static_cast<const call_info<Fn>&>(info).params.unsafe().get();
         auto& [a0, a1] = params;
         if constexpr (procedure_traits<decltype(Fn)>::takes_response_builder()) {
             auto res = (t.*(Fn))(a0, a1, response);
             if (!res) {
                 return lidl::emplace_raw<response_info<Fn>>(response);
             }
             return lidl::emplace_raw<response_info<Fn>>(response, *res);
         }
         auto res = (t.*(Fn))(a0, a1);
         if (!res) {
             return lidl::emplace_raw<response_info<Fn>>(response);
         }
         auto& ress = lidl::append_raw(response, *res);
         return lidl::emplace_raw<response_info<Fn>>(response, ress);
     }),
     ...);
    return handlers;
}

template<class T, auto... x>
auto make_lookup(const std::tuple<lidl::procedure_descriptor<x>...>& procs) {
    return make_lookup_impl<T>(procs, std::make_index_sequence<sizeof...(x)>{});
}

template<class Service>
void deserialize_call_and_execute(Service& impl,
                                  const lidl::buffer& call,
                                  lidl::message_builder& response) {
    auto& root = lidl::get_root<call_info_base>(call);
    constexpr lidl::service_descriptor<Service> desc;
    const auto handlers = make_lookup<Service>(desc.procedures);
    auto name = root.name.unsafe()->unsafe_string_view();
    std::cout << name << '\n';

    if (auto it = handlers.find(name); it != handlers.end()) {
        auto& res = it->second(impl, root, response);
        lidl::finish(response, res);
    } else {
        throw std::runtime_error("Non-existent procedure: " + std::string(name));
    }
}*/

std::vector<uint8_t> get_request() {
    std::vector<uint8_t> buf(64);
    lidl::message_builder builder(buf);
    lidl::create<calculator_call>(builder, multiply_params(3, 5));
    buf.resize(builder.size());
    return buf;
}

namespace detail {
template<class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
    // This implementation is valid since C++20 (via P1065R2)
    // In C++17, a constexpr counterpart of std::invoke is actually needed here
    return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
}
} // namespace detail

template<class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t) {
    return detail::apply_impl(
        std::forward<F>(f),
        std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

int main() {
    calculator_impl c;

    auto req = get_request();
    auto buf = lidl::buffer(req);
    std::cout << req.size() << '\n'
              << lidl::nameof(lidl::get_root<calculator_call>(buf).alternative()) << '\n';

    visit(
        [&](const auto& x) {
            auto fn = [&](auto&&... args) { return (c.*(x.params_for))(args...); };
            std::cout << *::apply(fn, x);
            // auto [l, r] = x;
        },
        lidl::get_root<calculator_call>(buf));

    /*std::array<uint8_t, 64> resp;
    lidl::message_builder resp_builder(resp);
    deserialize_call_and_execute<calculator>(c, lidl::buffer(req), resp_builder);

    auto& response =
        lidl::get_root<response_info<&calculator::multiply>>(resp_builder.get_buffer());

    std::cout << response.res.unsafe().get() << '\n';*/
}