#include "cppgen.hpp"
#include "lidl/module.hpp"

#include <doctest.h>


namespace lidl::cpp {
namespace {
TEST_CASE("name for identifier works for integral types") {
    module m;
    add_basic_types(m);

    REQUIRE_EQ("bool", get_identifier(m, name{*m.symbols->name_lookup("bool")}));
    REQUIRE_EQ("int8_t", get_identifier(m, name{*m.symbols->name_lookup("i8")}));
    REQUIRE_EQ("float", get_identifier(m, name{*m.symbols->name_lookup("f32")}));
    REQUIRE_EQ("double", get_identifier(m, name{*m.symbols->name_lookup("f64")}));
}

TEST_CASE("name for identifier works for slightly more complex types") {
    module m;
    add_basic_types(m);

    auto str_name = get_identifier(m, name{*m.symbols->name_lookup("string")});
    REQUIRE_EQ("::lidl::string", str_name);
}

TEST_CASE("name for identifier works for generic types") {
    module m;
    add_basic_types(m);

    auto vec_of_u8_name = get_identifier(
        m,
        name{*m.symbols->name_lookup("vector"), {name{*m.symbols->name_lookup("u8")}}});
    REQUIRE_EQ("::lidl::vector<uint8_t>", vec_of_u8_name);
}

TEST_CASE("name for identifier works for nested generic types") {
    module m;
    add_basic_types(m);

    auto vec_of_u8_name =
        get_identifier(m,
                       name{*m.symbols->name_lookup("vector"),
                            {name{*m.symbols->name_lookup("vector"),
                                  {name{*m.symbols->name_lookup("u8")}}}}});
    REQUIRE_EQ("::lidl::vector<::lidl::vector<uint8_t>>", vec_of_u8_name);
}
} // namespace
} // namespace lidl::cpp