#include <doctest.h>
#include <lidl/module.hpp>

namespace lidl {
void reference_type_pass(module&);
namespace {
TEST_CASE("reference type pass works with a string") {
    module m;
    add_basic_types(m);

    auto ptr_sym = *m.symbols->name_lookup("ptr");
    auto str_sym = *m.symbols->name_lookup("string");

    structure s;
    s.members.emplace_back("foo", member{str_sym});
    m.structs.emplace_back(std::move(s));

    reference_type_pass(m);

    auto& mem = std::get<member>(m.structs[0].members[0]);

    REQUIRE_EQ(ptr_sym, mem.type_.base);
    REQUIRE_EQ(1, mem.type_.args.size());
    auto name_arg = std::get_if<name>(&mem.type_.args[0]);
    REQUIRE_NE(nullptr, name_arg);
    REQUIRE_EQ(str_sym, name_arg->base);
    REQUIRE(name_arg->args.empty());
}

TEST_CASE("reference type pass works with a vector of strings") {
    module m;
    add_basic_types(m);

    auto ptr_sym = *m.symbols->name_lookup("ptr");
    auto str_sym = *m.symbols->name_lookup("string");
    auto vec_sym = *m.symbols->name_lookup("vector");

    auto vec_of_str_name = name{vec_sym, std::vector<generic_argument>{name{str_sym}}};

    structure s;
    s.members.emplace_back("foo", member{vec_of_str_name});
    m.structs.emplace_back(std::move(s));

    reference_type_pass(m);

    auto& mem = std::get<member>(m.structs[0].members[0]);

    REQUIRE_EQ(ptr_sym, mem.type_.base);
    REQUIRE_EQ(1, mem.type_.args.size());

    auto name_arg = std::get_if<name>(&mem.type_.args[0]);
    REQUIRE_NE(nullptr, name_arg);
    REQUIRE_EQ(vec_sym, name_arg->base);
    REQUIRE_EQ(1, name_arg->args.size());

    name_arg = std::get_if<name>(&name_arg->args[0]);
    REQUIRE_NE(nullptr, name_arg);
    REQUIRE_EQ(ptr_sym, name_arg->base);
    REQUIRE_EQ(1, name_arg->args.size());

    name_arg = std::get_if<name>(&name_arg->args[0]);
    REQUIRE_NE(nullptr, name_arg);
    REQUIRE_EQ(str_sym, name_arg->base);
    REQUIRE(name_arg->args.empty());
}
} // namespace
} // namespace lidl