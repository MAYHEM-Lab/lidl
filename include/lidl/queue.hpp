#pragma once

#include <lidl/base.hpp>

namespace lidl {
struct queue : cbase<base::categories::queue> {
    queue(name queue_of, base* parent, std::optional<source_info> loc = {})
        : cbase(parent, loc) {
        assert(is_type(queue_of));
    }

    name m_element_types;
};
} // namespace lidl