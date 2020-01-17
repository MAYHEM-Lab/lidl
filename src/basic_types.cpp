#include <iostream>
#include <proto/basic.hpp>
#include <proto/types.hpp>
#include <string_view>
#include <unordered_map>

namespace proto {
void add_basic_types(type_db& db) {
    db.define(std::make_unique<integral_type>("i8", 8, false));
    db.define(std::make_unique<integral_type>("i16", 16, false));
    db.define(std::make_unique<integral_type>("i32", 32, false));
    db.define(std::make_unique<integral_type>("i64", 64, false));

    db.define(std::make_unique<integral_type>("u8", 8, true));
    db.define(std::make_unique<integral_type>("u16", 16, true));
    db.define(std::make_unique<integral_type>("u32", 32, true));
    db.define(std::make_unique<integral_type>("u64", 64, true));

    db.define(std::make_unique<half_type>());
    db.define(std::make_unique<float_type>());
    db.define(std::make_unique<double_type>());

    db.define(std::make_unique<string_type>());
}
} // namespace proto
