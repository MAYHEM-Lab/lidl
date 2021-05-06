#pragma once

#include <lidl/basic.hpp>
#include <lidl/scope.hpp>
#include <lidl/source_info.hpp>
#include <optional>

namespace lidl {
class base {
public:
    enum class categories
    {
        module,
        type,
        service,
        procedure,
        generic_type,
        member,
        queue,
        other,
    };

    explicit base(categories category,
                  base* parent                          = nullptr,
                  std::optional<source_info> p_src_info = {});
    base(const base&);
    base(base&&)  = default;
    base& operator=(base&&) = default;

    std::optional<source_info> src_info;

    virtual ~base() = default;

    base* parent() const {
        return m_parent_elem;
    }

    scope& get_scope() {
        return *m_scope;
    }

    scope& get_scope() const {
        return *m_scope;
    }

    void set_scope(std::unique_ptr<scope> s) {
        m_scope = std::move(s);
        m_scope->set_object(*this);
    }

    categories category() const {
        return m_category;
    }

    bool is_generic() const{
        return category() == categories::generic_type;
    }

private:
    categories m_category;
    std::unique_ptr<scope> m_scope;
    mutable base* m_parent_elem = nullptr;
};

template<base::categories Category>
class cbase : public base {
public:
    explicit cbase(base* parent = nullptr, std::optional<source_info> p_src_info = {})
        : base(Category, parent, std::move(p_src_info)) {
    }
};
} // namespace lidl