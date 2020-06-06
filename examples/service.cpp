#include "lidl/builder.hpp"
#include "service_generated.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>

class calculator_impl : public scientific_calculator {
public:
    double add(const double& left, const double& right) override {
        return left + right;
    }

    double multiply(const double& left, const double& right) override {
        return left * right;
    }

    double log(const double& val) override {
        return ::log(val);
    }
};

class repeat_impl : public repeat {
public:
    std::string_view echo(std::string_view str,
                          lidl::message_builder& response) override {
        return str;
    }
};

std::vector<uint8_t> get_request() {
    std::vector<uint8_t> buf(64);
    lidl::message_builder builder(buf);
    lidl::create<calculator_call>(builder, calculator_multiply_params(3, 5));
    buf.resize(builder.size());
    return buf;
}
template<class... ParamsT>
struct get_result_type_impl;

template<auto... Procs, class... ParamsT, class... ResultsT>
struct get_result_type_impl<
    const std::tuple<lidl::procedure_descriptor<Procs, ParamsT, ResultsT>...>> {
    using params  = std::tuple<ParamsT...>;
    using results = std::tuple<ResultsT...>;
};

template<class T, class Tuple>
struct Index;

template<class T, class... Types>
struct Index<T, std::tuple<T, Types...>> {
    static const std::size_t value = 0;
};

template<class T, class U, class... Types>
struct Index<T, std::tuple<U, Types...>> {
    static const std::size_t value = 1 + Index<T, std::tuple<Types...>>::value;
};

template<class ServiceT, class ParamsT>
struct get_result_type {};

template<class ServiceT>
auto request_handler(std::common_type_t<ServiceT>& service) {
    using descriptor = lidl::service_descriptor<ServiceT>;

    using params_union  = typename descriptor::params_union;
    using results_union = typename descriptor::results_union;

    using all_params =
        typename get_result_type_impl<decltype(descriptor::procedures)>::params;
    using all_results =
        typename get_result_type_impl<decltype(descriptor::procedures)>::results;

    return [&](lidl::buffer buffer, lidl::message_builder& response) {
        visit(
            [&](const auto& call_params) -> decltype(auto) {
                constexpr auto proc = lidl::rpc_param_traits<std::remove_const_t<
                    std::remove_reference_t<decltype(call_params)>>>::params_for;
                using proc_traits   = lidl::procedure_traits<decltype(proc)>;
                constexpr auto idx  = Index<
                    std::remove_const_t<std::remove_reference_t<decltype(call_params)>>,
                    all_params>::value;
                using result_type = std::remove_const_t<std::remove_reference_t<decltype(
                    std::get<idx>(std::declval<all_results>()))>>;

                auto fn = [&](const auto&... args) -> void {
                    if constexpr (!proc_traits::takes_response_builder()) {
                        auto res = (service.*(proc))(args...);
                        lidl::create<results_union>(response, result_type(res));
                    } else {
                        auto& res = (service.*(proc))(args..., response);
                        auto& r   = lidl::create<result_type>(response, res);
                        lidl::create<results_union>(response, r);
                    }
                };
                lidl::apply(fn, call_params);
            },
            lidl::get_root<params_union>(buffer));
    };
}

static_assert(lidl::procedure_traits<decltype(&repeat::echo)>::takes_response_builder());
int main(int argc, char** argv) {
    // std::ifstream file(argv[1]);
    // std::vector<uint8_t> req(std::istream_iterator<uint8_t>(file),
    // std::istream_iterator<uint8_t>{});
    auto req = get_request();
    auto buf = lidl::buffer(req);
    //    std::cout.write((const char*)req.data(), req.size());
    std::cout << lidl::nameof(lidl::get_root<calculator_call>(buf).alternative()) << '\n';

    calculator_impl c;
    auto handler = request_handler<calculator>(c);

    std::array<uint8_t, 256> resp;
    lidl::message_builder resp_builder(resp);
    handler(buf, resp_builder);
    std::cout << resp_builder.size() << '\n';

    auto& res = lidl::get_root<calculator_return>(resp_builder.get_buffer()).multiply();
    std::cout << res.ret0() << '\n';
}