#include "lidlrt/builder.hpp"
#include "service_generated.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <lidlrt/find_extent.hpp>
#include <unordered_map>

class calculator_impl : public lidl_example::scientific_calculator::sync_server {
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

class repeat_impl : public lidl_example::repeat::sync_server {
public:
    std::string_view echo(std::string_view str,
                          lidl::message_builder& response) override {
        std::cerr << "echo got " << str << '\n';
        return str.substr(0, 3);
    }

    tos::span<uint8_t> echo_bytes(tos::span<uint8_t> arg,
                                  lidl::message_builder& response_builder) override {
        return arg;
    }

    const lidl::vector<uint8_t>&
    return_vector(lidl::message_builder& response_builder) override {
        return lidl::create_vector_sized<uint8_t>(response_builder, 20);
    }
};

std::vector<uint8_t> get_request() {
    std::vector<uint8_t> buf(64);
    lidl::message_builder builder(buf);
    lidl::create<lidl_example::scientific_calculator::wire_types::call_union>(
        builder, lidl_example::calculator::wire_types::multiply_params(3, 5));
    buf.resize(builder.size());
    return buf;
}

std::vector<uint8_t> get_echo_req() {
    std::vector<uint8_t> buf(64);
    lidl::message_builder builder(buf);
    lidl::create<lidl_example::repeat::wire_types::call_union>(
        builder,
        lidl::create<lidl_example::repeat::wire_types::echo_params>(
            builder, lidl::create_string(builder, "foobar")));
    buf.resize(builder.size());
    return buf;
}

int main(int argc, char** argv) {
    std::array<uint8_t, 128> buff;
    lidl::message_builder mb(buff);
    auto& s =
        lidl::create<lidl_example::string_struct>(mb, lidl::create_string(mb, "hello"));
    auto sp = lidl::meta::detail::find_extent(s);
    std::cerr << sp.size() << '\n';
    auto all = mb.get_buffer();
    std::cerr << all.size() << '\n';

    auto& deep = lidl::create<lidl_example::deep_struct>(
        mb, lidl::create_vector_sized<uint8_t>(mb, 0x2), s);
    sp = lidl::meta::detail::find_extent(deep);
    std::cerr << sp.size() << '\n';
    all = mb.get_buffer();
    std::cerr << all.size() << '\n';

    // std::ifstream file(argv[1]);
    // std::vector<uint8_t> req(std::istream_iterator<uint8_t>(file),
    // std::istream_iterator<uint8_t>{});
    auto req = get_request();
    //    std::cout.write((const char*)req.data(), req.size());
    std::cout << lidl::nameof(
                     lidl::get_root<
                         lidl_example::scientific_calculator::wire_types::call_union>(req)
                         .alternative())
              << '\n';

    calculator_impl c;
    auto handler =
        lidl::make_procedure_runner<lidl_example::scientific_calculator::sync_server>();

    repeat_impl r;
    auto rep_handler = lidl::make_procedure_runner<lidl_example::repeat::sync_server>();

    std::array<uint8_t, 256> resp;
    auto resp_builder = lidl::message_builder(resp);
    handler(c, req, resp_builder);
    std::cout << resp_builder.size() << '\n';

    auto& res =
        lidl::get_root<lidl_example::scientific_calculator::wire_types::return_union>(
            resp_builder.get_buffer())
            .multiply();
    std::cout << res.ret0() << '\n';

    resp_builder = lidl::message_builder(resp);
    req          = get_echo_req();
    rep_handler(r, req, resp_builder);
    std::cout << resp_builder.size() << '\n';

    auto& rep_res = lidl::get_root<lidl_example::repeat::wire_types::return_union>(
                        resp_builder.get_buffer())
                        .echo();
    std::cout << rep_res.ret0().string_view() << '\n';
}