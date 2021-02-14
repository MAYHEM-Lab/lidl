#include "lidl/layout.hpp"

#include <doctest.h>
#include <lidl/scope.hpp>
#include <lidl/union.hpp>
#include <lidl/module.hpp>

namespace lidl {
namespace {
TEST_CASE("scope declaration works") {
    auto s   = std::make_shared<scope>();
    auto foo = s->declare("foo");
    REQUIRE_FALSE(is_defined(foo));
}

struct mock_type : base {
};

TEST_CASE("scope definition works") {
    auto s   = std::make_shared<scope>();
    auto foo = s->declare("foo");
    s->define(foo, new mock_type);
    REQUIRE(is_defined(foo));
    auto res = s->lookup(foo);
    REQUIRE(dynamic_cast<const base*>(res));
}

struct mock_type2 : base {
    const scope* get_scope() const override {
        return m_scope.get();
    }

    mock_type2() {
        define(*m_scope, "foo", new mock_type);
    }

    std::shared_ptr<scope> m_scope = std::make_shared<scope>();
};

TEST_CASE("base scopes work") {
    auto s = std::make_shared<scope>();
    auto mock = new mock_type2;
    define(*s, "bar", mock);
    auto handle = recursive_definition_lookup(*s, mock);
    REQUIRE(handle);
}

TEST_CASE("union scoping works") {
    module m;
    m.add_child("", basic_module());
    m.unions.emplace_back();
    auto& un = m.unions.back();
    auto& e = un.get_enum(m);
    define(m.symbols(), "foo", &un);
    auto res = recursive_full_name_lookup(m.symbols(), "foo.alternatives");
    REQUIRE(res.has_value());
    auto def_res = recursive_definition_lookup(m.symbols(), &e);
    REQUIRE(def_res.has_value());
    REQUIRE_EQ("alternatives", local_name(*def_res));
}

TEST_CASE("scope symbol redefinition works") {
    auto s   = std::make_shared<scope>();
    auto foo = s->declare("foo");
    s->define(foo, new mock_type);
    REQUIRE(is_defined(foo));
    s->redefine(foo, new mock_type2);
    auto t = s->lookup(foo);
    REQUIRE(t);
    REQUIRE(dynamic_cast<const mock_type2*>(t));
}

TEST_CASE("scope trees work") {
    auto s = std::make_shared<scope>();

    auto foo = s->add_child_scope("foo");
    auto bar = foo->declare("bar");

    auto baz = s->add_child_scope("baz");
    auto yo  = baz->declare("yo");

    auto ret = recursive_full_name_lookup(*s, "foo.bar");
    REQUIRE_EQ(bar, ret.value());

    ret = recursive_full_name_lookup(*foo, "foo.bar");
    REQUIRE_EQ(bar, ret.value());

    ret = recursive_full_name_lookup(*s, "baz.yo");
    REQUIRE_EQ(yo, ret.value());

    ret = recursive_full_name_lookup(*foo, "baz.yo");
    REQUIRE_EQ(yo, ret.value());
}

TEST_CASE("sibling scopes work") {
    auto s = std::make_shared<scope>();

    auto foo = s->add_child_scope("foo");
    auto bar = foo->declare("bar");

    auto baz = s->add_child_scope("");
    auto yo  = baz->declare("yo");

    auto ret = recursive_full_name_lookup(*s, "foo.bar");
    REQUIRE_EQ(bar, ret.value());

    ret = recursive_full_name_lookup(*foo, "foo.bar");
    REQUIRE_EQ(bar, ret.value());

    ret = recursive_full_name_lookup(*s, "yo");
    REQUIRE_EQ(yo, ret.value());

    ret = recursive_full_name_lookup(*foo, "yo");
    REQUIRE_EQ(yo, ret.value());
}

TEST_CASE("scope absolute naming works") {
    auto s = std::make_shared<scope>();

    auto foo = s->add_child_scope("foo");
    auto bar = foo->declare("bar");

    auto name = absolute_name(bar);
    REQUIRE_EQ(std::vector<std::string_view>{"foo", "bar"}, name);
}
} // namespace
} // namespace lidl