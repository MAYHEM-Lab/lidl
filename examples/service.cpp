#include "lidl/builder.hpp"
#include "service_generated.hpp"

#include <iostream>
#include <unordered_map>


class calculator_impl : public calculator {
public:
    double add(const double& left, const double& right) override {
        return left + right;
    }

    double multiply(const double& left, const double& right) override {
        return left * right;
    }
};

class repeat_impl : public repeat {
public:
    const lidl::string& echo(const lidl::string& str,
                             lidl::message_builder& response) override {
        return lidl::create_string(response, str.unsafe_string_view());
    }
};

std::vector<uint8_t> get_request() {
    std::vector<uint8_t> buf(64);
    lidl::message_builder builder(buf);
    lidl::create<calculator_call>(builder, multiply_params(3, 5));
    buf.resize(builder.size());
    return buf;
}

template <class...> class print;
template<class ServiceT>
auto request_handler(std::common_type_t<ServiceT>& service) {
    using descriptor = lidl::service_descriptor<ServiceT>;
    using params_union = typename descriptor::params_union;
    using results_union = typename descriptor::results_union;
    return [&](lidl::buffer buffer, lidl::message_builder& response) -> decltype(auto) {
        return visit(
            [&](const auto& call_params) -> decltype(auto) {
                auto fn = [&](const auto&... args) -> decltype(auto) {
                    if constexpr (!lidl::procedure_traits<decltype(
                                      call_params
                                          .params_for)>::takes_response_builder()) {
                        auto res = (service.*(call_params.params_for))(args...);
                        return lidl::append_raw(response, res);
                    } else {
                        return (service.*(call_params.params_for))(args..., response);
                    }
                };
                return lidl::apply(fn, call_params);
            },
            lidl::get_root<params_union>(buffer));
    };
}

static_assert(lidl::procedure_traits<decltype(&repeat::echo)>::takes_response_builder());
int main() {
    auto req = get_request();
    auto buf = lidl::buffer(req);
    std::cout << lidl::nameof(lidl::get_root<calculator_call>(buf).alternative()) << '\n';

    calculator_impl c;
    auto handler = request_handler<calculator>(c);

    std::array<uint8_t, 256> resp;
    lidl::message_builder resp_builder(resp);
    std::cout << handler(buf, resp_builder) << '\n';
    std::cout << resp_builder.size() << '\n';

    repeat_impl i;
    auto repeat_handler = request_handler<repeat>(i);
}