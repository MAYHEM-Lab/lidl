#include "lidl/layout.hpp"

#include <doctest.h>
#include <lidl/scope.hpp>


namespace lidl {
namespace {
TEST_CASE("scope declaration works") {
    auto s = std::make_shared<scope>();
    auto foo = s->declare("foo");
    REQUIRE_FALSE(is_defined(foo));
}

struct mock_type : type {
    bool is_reference_type(const module&) const override {
        return false;
    }

    raw_layout wire_layout(const module&) const override {
        return {1, 1};
    }
};

TEST_CASE("scope definition works") {
    auto s = std::make_shared<scope>();
    auto foo = s->declare("foo");
    s->define(foo, new mock_type);
    REQUIRE(is_defined(foo));
    auto res = s->lookup(foo);
    REQUIRE(std::get<const type*>(res));
}

struct mock_type2 : type {
    bool is_reference_type(const module&) const override {
        return false;
    }

    raw_layout wire_layout(const module&) const override {
        return {2, 2};
    }
};

TEST_CASE("scope symbol redefinition works") {
    auto s = std::make_shared<scope>();
    auto foo = s->declare("foo");
    s->define(foo, new mock_type);
    REQUIRE(is_defined(foo));
    s->redefine(foo, new mock_type2);
    auto res = s->lookup(foo);
    auto t = std::get<const type*>(res);
    REQUIRE(t);
    REQUIRE(dynamic_cast<const mock_type2*>(t));
}

TEST_CASE("scope trees work") {
    auto s = std::make_shared<scope>();
    auto foo = s->add_child_scope();
    auto handle = foo->declare("bar");
}
} // namespace
} // namespace lidl