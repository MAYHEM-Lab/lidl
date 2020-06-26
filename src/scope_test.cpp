#include "lidl/layout.hpp"

#include <doctest.h>
#include <lidl/scope.hpp>


namespace lidl {
namespace {
TEST_CASE("scope declaration works") {
    auto s   = std::make_shared<scope>();
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

    YAML::Node bin2yaml(const module& module,
                                           ibinary_reader& span) const override {
        return YAML::Node(0);
    }
    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        return 0;
    }
};

TEST_CASE("scope definition works") {
    auto s   = std::make_shared<scope>();
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

    YAML::Node bin2yaml(const module& module,
                                           ibinary_reader& span) const override {
        return YAML::Node();
    }
    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        return 0;
    }
};

TEST_CASE("scope symbol redefinition works") {
    auto s   = std::make_shared<scope>();
    auto foo = s->declare("foo");
    s->define(foo, new mock_type);
    REQUIRE(is_defined(foo));
    s->redefine(foo, new mock_type2);
    auto res = s->lookup(foo);
    auto t   = std::get<const type*>(res);
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