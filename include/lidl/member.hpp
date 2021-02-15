#pragma once

#include "basic.hpp"
#include "source_info.hpp"

#include <lidl/base.hpp>

namespace lidl {
struct member : public base {
    using base::base;
    explicit member(name type,
                    base* parent                        = nullptr,
                    std::optional<source_info> src_info = {})
        : base(parent, std::move(src_info))
        , type_{std::move(type)} {
    }

    name type_;
    bool nullable = false;

    bool is_nullable() const {
        return nullable;
    }
};
} // namespace lidl