#pragma once

#include <cassert>
#include <lidl/scope.hpp>
#include <optional>
#include <algorithm>

namespace lidl {
template<class T>
struct extendable {
    std::optional<name> extends;

    void set_base(name nm) {
        extends = nm;
    }

    const T* get_base() const {
        assert(extends->args.empty());

        auto base_sym = get_symbol(extends->base);
        return dynamic_cast<const T*>(base_sym);
    }

    std::vector<const T*> inheritance_list() const {
        std::vector<const T*> inheritance;

        for (const T* s = static_cast<const T*>(this); s;) {
            inheritance.emplace_back(s);

            if (s->extends) {
                assert(s->extends->args.empty());

                auto base_sym = get_symbol(s->extends->base);

                if (auto base_union = dynamic_cast<const T*>(base_sym); base_union) {
                    s = base_union;
                } else {
                    throw std::runtime_error("Base of an extendable is wrong!");
                }
            } else {
                s = nullptr;
            }
        }
        std::reverse(inheritance.begin(), inheritance.end());

        return inheritance;
    }
};
} // namespace lidl