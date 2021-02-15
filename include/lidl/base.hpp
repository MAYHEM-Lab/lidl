#pragma once

#include <lidl/basic.hpp>
#include <lidl/scope.hpp>
#include <lidl/source_info.hpp>
#include <optional>

namespace lidl {
class base {
public:
    explicit base(base* parent = nullptr, std::optional<source_info> p_src_info = {});
    base(const base&);
    base(base&&) = default;
    base& operator=(base&&) = default;

    std::optional<source_info> src_info;

    virtual ~base() = default;

    void set_parent(base* parent) const {
        m_parent_elem = parent;
    }

    const base* parent() const {
        return m_parent_elem;
    }

    scope& get_scope() {
        return *m_scope;
    }

    const scope& get_scope() const {
        return *m_scope;
    }

    void set_scope(std::unique_ptr<scope> s) {
        m_scope = std::move(s);
        m_scope->set_object(*this);
    }

private:
    std::unique_ptr<scope> m_scope;
    mutable base* m_parent_elem = nullptr;
};
} // namespace lidl