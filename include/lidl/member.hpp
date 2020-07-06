#pragma once

#include "basic.hpp"
#include "source_info.hpp"

namespace lidl {
struct member {
    name type_;
    bool nullable = false;
    std::optional<source_info> src_info;

    bool is_nullable() const {
        return nullable;
    }
};
}