#include <iostream>
#include <lidl/basic.hpp>
#include <lidl/types.hpp>
#include <string_view>
#include <unordered_map>

namespace lidl {
void add_basic_types(type_db& db) {
    db.define(std::make_unique<integral_type>(identifier("i8"), 8, false));
    db.define(std::make_unique<integral_type>(identifier("i16"), 16, false));
    db.define(std::make_unique<integral_type>(identifier("i32"), 32, false));
    db.define(std::make_unique<integral_type>(identifier("i64"), 64, false));

    db.define(std::make_unique<integral_type>(identifier("u8"), 8, true));
    db.define(std::make_unique<integral_type>(identifier("u16"), 16, true));
    db.define(std::make_unique<integral_type>(identifier("u32"), 32, true));
    db.define(std::make_unique<integral_type>(identifier("u64"), 64, true));

    db.define(std::make_unique<half_type>());
    db.define(std::make_unique<float_type>());
    db.define(std::make_unique<double_type>());

    db.define(std::make_unique<string_type>());
}

struct vector_type : type {
    vector_type()
        : type(identifier("vector", std::vector<identifier>{identifier("T")})) {
    }
};

struct pointer_type : type {
    pointer_type()
        : type(identifier("pointer", std::vector<identifier>{identifier("T")})) {
    }
};

struct array_type : type {
    array_type()
        : type(identifier("array",
                          std::vector<identifier>{identifier("T"), identifier("Size")})) {
    }

    virtual bool is_raw() const override {
        return true;
    }
};

void add_basic_types(generics_table& db) {
    db.define(std::make_unique<pointer_type>());
    db.define(std::make_unique<vector_type>());
    db.define(std::make_unique<array_type>());
}
} // namespace lidl
