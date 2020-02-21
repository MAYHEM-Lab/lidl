#pragma once

#include <lidl/meta.hpp>

namespace lidl {
template<class T>
struct union_base {
protected:
    using discriminator_type = typename T::discriminator_type;
    discriminator_type discriminator;

    [[nodiscard]] int index() const noexcept {
        return static_cast<int>(discriminator);
    }
};

template<class UnionT>
struct union_traits;
} // namespace lidl