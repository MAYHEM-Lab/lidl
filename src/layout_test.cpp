#include <doctest.h>
#include <lidl/layout.hpp>

namespace lidl {
namespace {
TEST_CASE("layout with 2 identical members") {
    compound_layout computer;
    computer.add_member("foo", {1, 1});
    computer.add_member("bar", {1, 1});
    REQUIRE_EQ(raw_layout{2, 1}, computer.get());
}
TEST_CASE("layout with 2 different members") {
    compound_layout computer;
    computer.add_member("foo", {1, 1});
    computer.add_member("bar", {4, 1});
    REQUIRE_EQ(raw_layout{5, 1}, computer.get());
}
TEST_CASE("layout big alignment") {
    compound_layout computer;
    computer.add_member("foo", {4, 4});
    computer.add_member("bar", {1, 1});
    REQUIRE_EQ(raw_layout{8, 4}, computer.get());
}
TEST_CASE("layout small to big alignment") {
    compound_layout computer;
    computer.add_member("foo", {1, 1});
    computer.add_member("bar", {4, 4});
    REQUIRE_EQ(raw_layout{8, 4}, computer.get());
}
TEST_CASE("weird layout") {
    compound_layout computer;
    computer.add_member("foo", {9, 1});
    REQUIRE_EQ(raw_layout{9, 1}, computer.get());
    computer.add_member("bar", {8, 8});
    REQUIRE_EQ(raw_layout{24, 8}, computer.get());
}
TEST_CASE("three members") {
    compound_layout computer;
    computer.add_member("foo", {12, 4});
    computer.add_member("bar", {2, 2});
    REQUIRE_EQ(raw_layout{16, 4}, computer.get());
    computer.add_member("baz", {2, 2});
    REQUIRE_EQ(raw_layout{16, 4}, computer.get());
}
} // namespace
} // namespace lidl