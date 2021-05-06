#include <doctest.h>
#include <gsl/span>
#include <lidl/module.hpp>

namespace lidl {
namespace {
class memory_writer : public ibinary_writer {
public:
    explicit memory_writer(gsl::span<uint8_t> data)
        : m_data{data} {
    }

    int tell() override {
        return m_loc;
    }
    void write_raw(gsl::span<const char> data) override {
        memcpy(m_data.data() + m_loc, data.data(), data.size());
        m_loc += data.size();
    }

    gsl::span<uint8_t> get_data() {
        return m_data.subspan(0, m_loc);
    }

    gsl::span<uint8_t> m_data;
    int m_loc = 0;
};

TEST_CASE("yaml2bin") {
    auto root_module = std::make_unique<lidl::module>();

    auto& root   = root_module->add_child("", lidl::basic_module());
    auto& module = root_module->get_child("test");

    auto bool_handle = recursive_full_name_lookup(module.symbols(), "bool").value();
    auto i8_handle   = recursive_full_name_lookup(module.symbols(), "i8").value();
    auto i32_handle  = recursive_full_name_lookup(module.symbols(), "i32").value();
    auto str_handle  = recursive_full_name_lookup(module.symbols(), "string").value();
    auto ptr_handle  = recursive_full_name_lookup(module.symbols(), "ptr").value();
    auto vec_handle  = recursive_full_name_lookup(module.symbols(), "vector").value();

    module.enums.emplace_back();
    auto& enumeration            = module.enums.back();
    enumeration->underlying_type = name{i8_handle};
    enumeration->add_member("foo");
    enumeration->add_member("bar");
    enumeration->add_member("baz");
    auto enum_handle = define(module.symbols(), "enum", enumeration.get());

    module.unions.emplace_back();
    auto& un = module.unions.back();
    un.members.emplace_back("foo", member{name{ptr_handle, {name{str_handle}}}});

    uint8_t buff[64];

    SUBCASE("ints") {
        auto i32 = std::get<const type*>(get_symbol(i32_handle));
        memory_writer writer(buff);

        i32->yaml2bin(module, YAML::Node(42), writer);
        auto res = writer.get_data();

        REQUIRE_EQ(std::vector<uint8_t>{42, 0, 0, 0},
                   std::vector<uint8_t>(res.begin(), res.end()));
    }

    SUBCASE("ints alignment") {
        auto i32 = std::get<const type*>(get_symbol(i32_handle));
        memory_writer writer(buff);

        writer.write<uint8_t>(0);
        i32->yaml2bin(module, YAML::Node(42), writer);
        auto res = writer.get_data();

        REQUIRE_EQ(std::vector<uint8_t>{0, 0, 0, 0, 42, 0, 0, 0},
                   std::vector<uint8_t>(res.begin(), res.end()));
    }

    SUBCASE("strings") {
        auto str = std::get<const type*>(get_symbol(str_handle));
        memory_writer writer(buff);

        str->yaml2bin(module, YAML::Node("hello world"), writer);
        auto res = writer.get_data();

        REQUIRE_EQ(
            std::vector<uint8_t>{
                11, 0, 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'},
            std::vector<uint8_t>(res.begin(), res.end()));
    }

    SUBCASE("vector of ints") {
        generic_instantiation vec_of_int(name{vec_handle, {name{i32_handle}}});
        memory_writer writer(buff);

        vec_of_int.yaml2bin(module, YAML::Node(std::vector<int64_t>{1, 2, 3}), writer);
        auto res = writer.get_data();

        REQUIRE_EQ(std::vector<uint8_t>{3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0},
                   std::vector<uint8_t>(res.begin(), res.end()));
    }

    SUBCASE("vector of pointers to strings") {
        generic_instantiation vec_of_int(
            name{vec_handle, {name{ptr_handle, {{name{str_handle}}}}}});
        memory_writer writer(buff);

        vec_of_int.yaml2bin(
            module, YAML::Node(std::vector<std::string>{"hello", "world"}), writer);
        auto res = writer.get_data();
        auto vec = std::vector<uint8_t>(res.begin(), res.end());

        REQUIRE_EQ(std::vector<uint8_t>{5,   0,   'h', 'e', 'l', 'l', 'o', 0,  5, 0,  'w',
                                        'o', 'r', 'l', 'd', 0,   2,   0,   18, 0, 12, 0},
                   vec);
    }
}
} // namespace
} // namespace lidl