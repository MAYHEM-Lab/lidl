#pragma once

#include "basic.hpp"

namespace lidl {
struct member {
    name type_;
    bool nullable = false;

    bool is_nullable() const {
        return nullable;
    }
};
}