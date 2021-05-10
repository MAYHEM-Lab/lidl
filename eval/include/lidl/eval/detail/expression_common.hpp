#pragma once

#include <lidl/module.hpp>
#include <lidl/source_info.hpp>
#include <optional>

namespace lidl::eval::detail {
template<class T>
concept MemberEvaluatable = requires(const T& t) {
    t.evaluate(std::declval<const lidl::module&>());
};

template<class T>
concept FreeEvaluatable = requires(const T& t) {
    evaluate(std::declval<const lidl::module&>(), t);
};
} // namespace lidl::eval::detail