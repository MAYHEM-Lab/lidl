#include "cppgen.hpp"
#include "lidl/module.hpp"

#include <doctest.h>

namespace lidl::cpp {
namespace {
TEST_CASE("name for identifier works for integral types") {
    auto basic_module = lidl::basic_module();
    auto& m           = *basic_module;

    REQUIRE_EQ("bool", get_identifier(m, name{m.symbols().name_lookup("bool").value()}));
    REQUIRE_EQ("int8_t", get_identifier(m, name{m.symbols().name_lookup("i8").value()}));
    REQUIRE_EQ("float", get_identifier(m, name{m.symbols().name_lookup("f32").value()}));
    REQUIRE_EQ("double", get_identifier(m, name{m.symbols().name_lookup("f64").value()}));
}

TEST_CASE("name for identifier works for slightly more complex types") {
    auto basic_module = lidl::basic_module();
    auto& m           = *basic_module;

    auto str_name = get_identifier(m, name{m.symbols().name_lookup("string").value()});
    REQUIRE_EQ("lidl::string", str_name);
}

TEST_CASE("name for identifier works for generic types") {
    auto basic_module = lidl::basic_module();
    auto& m           = *basic_module;

    auto vec_of_u8_name =
        get_identifier(m,
                       name{m.symbols().name_lookup("vector").value(),
                            {name{m.symbols().name_lookup("u8").value()}}});
    REQUIRE_EQ("lidl::vector<uint8_t>", vec_of_u8_name);
}

TEST_CASE("name for identifier works for nested generic types") {
    auto basic_module = lidl::basic_module();
    auto& m           = *basic_module;

    auto vec_of_u8_name =
        get_identifier(m,
                       name{m.symbols().name_lookup("vector").value(),
                            {name{m.symbols().name_lookup("vector").value(),
                                  {name{m.symbols().name_lookup("u8").value()}}}}});
    REQUIRE_EQ("lidl::vector<lidl::vector<uint8_t>>", vec_of_u8_name);
}
} // namespace
} // namespace lidl::cpp