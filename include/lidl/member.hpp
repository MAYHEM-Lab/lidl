#pragma once

#include "basic.hpp"
#include "source_info.hpp"

#include <lidl/base.hpp>

namespace lidl {
struct member : public cbase<base::categories::member> {
    using cbase::cbase;
    explicit member(name type,
                    base* parent                        = nullptr,
                    std::optional<source_info> src_info = {})
        : cbase(parent, std::move(src_info))
        , type_{std::move(type)} {
    }

    void finalize(const module& mod) {
        type_.finalize(mod);
    }

    name type_;
    bool nullable = false;

    bool is_nullable() const {
        return nullable;
    }
};
} // namespace lidl