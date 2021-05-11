#include "get_identifier.hpp"
#include "lidl/module.hpp"

#include <doctest.h>

namespace lidl::js {
namespace {
TEST_CASE("name for identifier works for integral types") {
    auto basic_module = lidl::basic_module();
    auto& m           = *basic_module;

    REQUIRE_EQ("new lidl.Uint8Class()",
               get_local_type_name(m, name{m.symbols().name_lookup("u8").value()}));
    REQUIRE_EQ("new lidl.Int8Class()",
               get_local_type_name(m, name{m.symbols().name_lookup("i8").value()}));
    REQUIRE_EQ("new lidl.FloatClass()",
               get_local_type_name(m, name{m.symbols().name_lookup("f32").value()}));
    REQUIRE_EQ("new lidl.DoubleClass()",
               get_local_type_name(m, name{m.symbols().name_lookup("f64").value()}));
}

TEST_CASE("Pointer names work") {
    auto basic_module = lidl::basic_module();
    auto& m           = *basic_module;

    auto vec_of_u8_name =
        get_local_type_name(m,
                            name{m.symbols().name_lookup("ptr").value(),
                                 {name{m.symbols().name_lookup("u8").value()}}});
    REQUIRE_EQ("new lidl.PtrClass(new lidl.Uint8Class())", vec_of_u8_name);
}
} // namespace
} // namespace lidl::js