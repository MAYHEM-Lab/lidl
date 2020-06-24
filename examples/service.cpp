#include "lidl/builder.hpp"
#include "service_generated.hpp"
#include <lidl/find_extent.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>

class calculator_impl : public lidl_example::scientific_calculator {
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

class repeat_impl : public lidl_example::repeat {
public:
    std::string_view echo(std::string_view str,
                          lidl::message_builder& response) override {
        std::cerr << "echo got " << str << '\n';
        return str.substr(0, 3);
    }
};

std::vector<uint8_t> get_request() {
    std::vector<uint8_t> buf(64);
    lidl::message_builder builder(buf);
    lidl::create<lidl_example::calculator_call>(builder, lidl_example::calculator_multiply_params(3, 5));
    buf.resize(builder.size());
    return buf;
}

std::vector<uint8_t> get_echo_req() {
    std::vector<uint8_t> buf(64);
    lidl::message_builder builder(buf);
    lidl::create<lidl_example::repeat_call>(builder,
                              lidl::create<lidl_example::repeat_echo_params>(
                                  builder, lidl::create_string(builder, "foobar")));
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

template<class T>
using remove_cref = std::remove_const_t<std::remove_reference_t<T>>;

template<class ServiceT>
auto make_request_handler() {
    /**
     * Don't panic!
     *
     * Yes, this is full of fearsome metaprogramming magic, but I'll walk you through.
     */

    // The service descriptor stores the list of procedures in a service. We'll use this
    // information to decode lidl messages into actual calls to services.
    using descriptor = lidl::service_descriptor<ServiceT>;

    using params_union  = typename ServiceT::call_union;
    using results_union = typename ServiceT::return_union;

    using all_params =
        typename get_result_type_impl<decltype(descriptor::procedures)>::params;
    using all_results =
        typename get_result_type_impl<decltype(descriptor::procedures)>::results;

    return [](ServiceT& service, lidl::buffer buffer, lidl::message_builder& response) {
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

                /**
                 * This ugly thing is where the final magic happens.
                 *
                 * The apply call will pass each member of the parameters of the call to
                 * this function.
                 *
                 * Inside, we have a bunch of cases:
                 *
                 * 1. Does the procedure take a message builder or not?
                 *
                 *    Procedures that do not return a _reference type_ (types that contain
                 *    pointers) do not need a message builder since their result will be
                 *    self contained.
                 *
                 * 2. Is the return value a view type?
                 *
                 *    Procedures that return a view type need special care. The special
                 *    care is basically that we copy whatever it returns to the response
                 *    buffer.
                 *
                 *    If not, we return whatever the procedure returned directly.
                 *
                 */
                auto make_service_call = [&service,
                                          &response](const auto&... args) -> void {
                    if constexpr (!proc_traits::takes_response_builder()) {
                        auto res = (service.*(proc))(args...);
                        lidl::create<results_union>(response, result_type(res));
                    } else {
                        const auto& res = (service.*(proc))(args..., response);
                        if constexpr (std::is_same_v<remove_cref<decltype(res)>,
                                                     std::string_view>) {
                            /**
                             * The procedure returned a view.
                             *
                             * We need to see if the returned view is already in the
                             * response buffer. If it is not, we will copy it.
                             *
                             * Issue #6.
                             */

                            auto& str     = lidl::create_string(response, res);
                            const auto& r = lidl::create<result_type>(response, str);
                            lidl::create<results_union>(response, r);
                        } else {
                            const auto& r = lidl::create<result_type>(response, res);
                            lidl::create<results_union>(response, r);
                        }
                    }
                };

                lidl::apply(make_service_call, call_params);
            },
            lidl::get_root<params_union>(buffer));
    };
}

static_assert(lidl::procedure_traits<decltype(&lidl_example::repeat::echo)>::takes_response_builder());
int main(int argc, char** argv) {
    std::array<uint8_t, 128> buff;
    lidl::message_builder mb(buff);
    auto& s = lidl::create<lidl_example::string_struct>(mb, lidl::create_string(mb, "hello"));
    auto sp = lidl::meta::detail::find_extent(s);
    std::cerr << sp.size() << '\n';
    auto all = mb.get_buffer().get_buffer();
    std::cerr << all.size() << '\n';

    auto& deep = lidl::create<lidl_example::deep_struct>(mb, lidl::create_vector_sized<uint8_t>(mb, 0x2), s);
    sp = lidl::meta::detail::find_extent(deep);
    std::cerr << sp.size() << '\n';
    all = mb.get_buffer().get_buffer();
    std::cerr << all.size() << '\n';

    // std::ifstream file(argv[1]);
    // std::vector<uint8_t> req(std::istream_iterator<uint8_t>(file),
    // std::istream_iterator<uint8_t>{});
    auto req = get_request();
    auto buf = lidl::buffer(req);
    //    std::cout.write((const char*)req.data(), req.size());
    std::cout << lidl::nameof(lidl::get_root<lidl_example::calculator_call>(buf).alternative()) << '\n';

    calculator_impl c;
    auto handler = make_request_handler<lidl_example::calculator>();

    repeat_impl r;
    auto rep_handler = make_request_handler<lidl_example::repeat>();

    std::array<uint8_t, 256> resp;
    auto resp_builder = lidl::message_builder(resp);
    handler(c, buf, resp_builder);
    std::cout << resp_builder.size() << '\n';

    auto& res = lidl::get_root<lidl_example::calculator_return>(resp_builder.get_buffer()).multiply();
    std::cout << res.ret0() << '\n';

    resp_builder = lidl::message_builder(resp);
    req          = get_echo_req();
    rep_handler(r, lidl::buffer(req), resp_builder);
    std::cout << resp_builder.size() << '\n';

    auto& rep_res = lidl::get_root<lidl_example::repeat_return>(resp_builder.get_buffer()).echo();
    std::cout << rep_res.ret0().string_view() << '\n';
}