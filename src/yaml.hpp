#pragma once

#include <lidl/basic.hpp>
#include <lidl/module.hpp>
#include <string_view>

namespace lidl::yaml {
module&
load_module(module& root, std::istream& str, std::optional<std::string> origin = {});
}