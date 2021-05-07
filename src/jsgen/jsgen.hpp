#pragma once

#include <fmt/format.h>
#include <lidl/module.hpp>
#include <lidl/types.hpp>
#include <string>

namespace lidl::js {
inline std::string generate_layout_getter(const module& mod, const wire_type& t) {
    auto layout = t.wire_layout(mod);

    static constexpr auto format = R"__(
    layout(): lidl.Layout {{
        return {{
            size: {size},
            alignment: {align}
        }};
    }}
)__";

    return fmt::format(
        format, fmt::arg("size", layout.size()), fmt::arg("align", layout.alignment()));
}
} // namespace lidl::js