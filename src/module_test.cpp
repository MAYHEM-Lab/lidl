#include <doctest.h>
#include <lidl/module.hpp>

namespace lidl {
namespace {
TEST_CASE("basic types work") {
    module m;
    add_basic_types(m);

    auto str_sym = m.symbols->name_lookup("string");
    REQUIRE(str_sym.has_value());
}

TEST_CASE("creating a simple struct in a module works") {
    module m;
    add_basic_types(m);

    structure s;
    s.members.emplace_back("foo", member{*m.symbols->name_lookup("string")});
    m.structs.emplace_back(std::move(s));
}
} // namespace
} // namespace lidl