#include <doctest.h>
#include <lidl/layout.hpp>

namespace lidl {
namespace {
TEST_CASE("layout with 2 identical members") {
    aggregate_layout_computer computer;
    computer.add({1, 1});
    computer.add({1, 1});
    REQUIRE_EQ(raw_layout{2, 1}, computer.get());
}
TEST_CASE("layout with 2 different members") {
    aggregate_layout_computer computer;
    computer.add({1, 1});
    computer.add({4, 1});
    REQUIRE_EQ(raw_layout{5, 1}, computer.get());
}
TEST_CASE("layout big alignment") {
    aggregate_layout_computer computer;
    computer.add({4, 4});
    computer.add({1, 1});
    REQUIRE_EQ(raw_layout{8, 4}, computer.get());
}
TEST_CASE("layout small to big alignment") {
    aggregate_layout_computer computer;
    computer.add({1, 1});
    computer.add({4, 4});
    REQUIRE_EQ(raw_layout{8, 4}, computer.get());
}
TEST_CASE("weird layout") {
    aggregate_layout_computer computer;
    computer.add({9, 1});
    REQUIRE_EQ(raw_layout{9, 1}, computer.get());
    computer.add({8, 8});
    REQUIRE_EQ(raw_layout{24, 8}, computer.get());
}
} // namespace
} // namespace lidl