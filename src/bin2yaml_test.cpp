#include "passes.hpp"

#include <doctest.h>
#include <lidl/module.hpp>
#include <lidl/types.hpp>

namespace lidl {
namespace {
class memory_reader : public ibinary_reader {
public:
    explicit memory_reader(gsl::span<uint8_t> data)
        : m_data{data}
        , m_loc(m_data.size()) {
    }

    using ibinary_reader::seek;
    void seek(int bytes, std::ios::seekdir dir) override {
        if (dir == std::ios::cur) {
            m_loc += bytes;
        } else if (dir == std::ios::beg) {
            m_loc = bytes;
        } else {
            m_loc = m_data.size() - bytes;
        }
    }

    int tell() override {
        return m_loc;
    }

    gsl::span<uint8_t> read(gsl::span<uint8_t> span) override {
        memcpy(span.data(), m_data.data() + m_loc, span.size());
        return span;
    }

    gsl::span<const uint8_t> m_data;
    int m_loc;
};

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

TEST_CASE("bin2yaml") {
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
    auto& enumeration           = module.enums.back();
    enumeration.underlying_type = name{i8_handle};
    enumeration.members.emplace_back("foo", enum_member{0});
    enumeration.members.emplace_back("bar", enum_member{1});
    enumeration.members.emplace_back("baz", enum_member{2});
    auto enum_handle = define(module.symbols(), "enum", &enumeration);

    module.unions.emplace_back();
    auto& un = module.unions.back();
    un.add_member("foo", member{name{ptr_handle, {name{str_handle}}}});

    union_enum_pass(module);

    uint8_t buff[64];

    SUBCASE("bools") {
        auto b = std::get<const type*>(get_symbol(bool_handle));

        memory_writer writer(buff);
        writer.write(true);

        memory_reader reader(writer.get_data());
        reader.seek(0, std::ios::beg);
        auto res = b->bin2yaml(module, reader);

        REQUIRE_EQ(true, res.as<bool>());
    }

    SUBCASE("ints") {
        auto i32 = std::get<const type*>(get_symbol(i32_handle));

        int32_t i = 0xDEADBEEF;
        uint8_t buf[4];
        memcpy(buf, &i, 4);

        memory_reader reader(buf);
        reader.seek(-i32->wire_layout(module).size());
        auto res = i32->bin2yaml(module, reader);

        REQUIRE_EQ(0xDEADBEEF, res.as<uint64_t>());
    }

    SUBCASE("strings") {
        /**
         * Normally, having just a string in a lidl message is not well defined, as you
         * cannot find technically find the beginning of a string if you're just given a
         * buffer.
         *
         * The well-defined use is to put a pointer to the string. The reference type pass
         * in the lidl compiler actually converts every mention of string into a pointer
         * to a string. We do not do that here and we know where the string begins so it's
         * okay.
         *
         * Same thing for vectors!
         */
        auto str = std::get<const type*>(get_symbol(str_handle));

        memory_writer writer(buff);

        writer.write<int16_t>(11);
        writer.write_raw_string("hello world");

        memory_reader reader(writer.get_data());
        reader.seek(0, std::ios::beg);
        auto res = str->bin2yaml(module, reader);

        using namespace std::string_literals;
        REQUIRE_EQ("hello world"s, res.as<std::string>());
    }

    SUBCASE("Pointer to int") {
        generic_instantiation ptr_to_int(name{ptr_handle, {name{i32_handle}}});

        int32_t i = 0xDEADBEEF;
        uint8_t buf[6];
        memcpy(buf, &i, 4);

        int16_t ptrdiff = 4;
        memcpy(buf + 4, &ptrdiff, sizeof ptrdiff);

        memory_reader reader(buf);
        reader.seek(-ptr_to_int.wire_layout(module).size());
        auto res = ptr_to_int.bin2yaml(module, reader);

        REQUIRE_EQ(0xDEADBEEF, res.as<uint64_t>());
    }

    SUBCASE("Pointer to strings") {
        generic_instantiation ptr_to_str(name{ptr_handle, {name{str_handle}}});

        memory_writer writer(buff);

        writer.write<int16_t>(11);
        writer.write_raw_string("hello world");
        writer.align(2);
        writer.write<int16_t>(writer.tell());

        memory_reader reader(writer.get_data());
        reader.seek(-2);
        auto res = ptr_to_str.bin2yaml(module, reader);

        using namespace std::string_literals;
        REQUIRE_EQ("hello world"s, res.as<std::string>());
    }

    SUBCASE("vector of ints") {
        generic_instantiation vec_of_int(name{vec_handle, {name{i32_handle}}});

        memory_writer writer(buff);

        writer.write<int16_t>(2);
        writer.align(4);
        writer.write<int32_t>(42);
        writer.write<int32_t>(33);

        memory_reader reader(writer.get_data());
        reader.seek(0, std::ios::beg);

        auto res = vec_of_int.bin2yaml(module, reader);

        REQUIRE_EQ(std::vector<uint64_t>{42, 33}, res.as<std::vector<uint64_t>>());
    }

    SUBCASE("Pointer to vector of ints") {
        generic_instantiation ptr_to_vec_of_int(
            name{ptr_handle, {name{vec_handle, {name{i32_handle}}}}});

        memory_writer writer(buff);

        writer.write<int16_t>(2); // size
        writer.align(4);
        writer.write<int32_t>(42); // elements
        writer.write<int32_t>(33);
        writer.align(2);
        writer.write<int16_t>(writer.tell()); // pointer offset

        memory_reader reader(writer.get_data());
        reader.seek(-2);
        auto res = ptr_to_vec_of_int.bin2yaml(module, reader);

        REQUIRE_EQ(std::vector<uint64_t>{42, 33}, res.as<std::vector<uint64_t>>());
    }

    SUBCASE("enums") {
        generic_instantiation vec_of_enums(name{vec_handle, {name{enum_handle}}});

        memory_writer writer(buff);

        writer.write<int16_t>(3);
        writer.align(1);
        writer.write<int8_t>(0); // foo
        writer.write<int8_t>(1); // bar
        writer.write<int8_t>(2); // baz

        memory_reader reader(writer.get_data());
        reader.seek(0, std::ios::beg);
        auto res = vec_of_enums.bin2yaml(module, reader);

        REQUIRE_EQ(std::vector<std::string>{"foo", "bar", "baz"},
                   res.as<std::vector<std::string>>());
    }

    SUBCASE("unions") {
        memory_writer writer(buff);

        writer.write<int16_t>(11);
        writer.write_raw_string("hello world");

        writer.write<int16_t>(3);
        writer.align(2);
        writer.write<int16_t>(writer.tell()); // pointer to string

        memory_reader reader(writer.get_data());
        reader.seek(-un.wire_layout(module).size());
        auto res = un.bin2yaml(module, reader);

        REQUIRE_EQ("hello world", res["foo"].as<std::string>());
    }
}
} // namespace
} // namespace lidl