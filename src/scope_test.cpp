#include "lidl/layout.hpp"

#include <doctest.h>
#include <lidl/module.hpp>
#include <lidl/scope.hpp>
#include <lidl/union.hpp>

namespace lidl {
namespace {
TEST_CASE("scope declaration works") {
    auto s   = std::make_shared<base>();
    auto foo = s->get_scope().declare("foo");
    REQUIRE_FALSE(is_defined(foo));
}

struct mock_type : base {};

TEST_CASE("scope definition works") {
    auto s   = std::make_shared<base>();
    auto foo = s->get_scope().declare("foo");
    s->get_scope().define(foo, new mock_type);
    REQUIRE(is_defined(foo));
    auto res = s->get_scope().lookup(foo);
    REQUIRE(dynamic_cast<const base*>(res));
}

struct mock_type2 : base {
    mock_type2() {
        define(get_scope(), "foo", new mock_type);
    }
};

TEST_CASE("base scopes work") {
    auto s    = std::make_shared<base>();
    auto mock = new mock_type2;
    define(s->get_scope(), "bar", mock);
    auto handle = recursive_definition_lookup(s->get_scope(), mock);
    REQUIRE(handle);
}

TEST_CASE("union scoping works") {
    module m;
    m.add_child("", basic_module());
    m.unions.emplace_back(&m);
    auto& un = m.unions.back();
    auto& e  = un.get_enum(m);
    define(m.symbols(), "foo", &un);
    auto res = recursive_full_name_lookup(m.symbols(), "foo.alternatives");
    REQUIRE(res.has_value());
    auto def_res = recursive_definition_lookup(m.symbols(), &e);
    REQUIRE(def_res.has_value());
    REQUIRE_EQ("alternatives", local_name(*def_res));
}
} // namespace
} // namespace lidl