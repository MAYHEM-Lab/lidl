#pragma once

#include <lidl/basic.hpp>
#include <lidl/source_info.hpp>
#include <optional>

namespace lidl {
class base {
public:
    explicit base(std::optional<source_info> p_src_info = {})
        : src_info(std::move(p_src_info)) {
    }

    std::optional<source_info> src_info;

    [[nodiscard]] virtual const scope* get_scope() const {
        return nullptr;
    }

    virtual ~base() = default;
};
} // namespace lidl