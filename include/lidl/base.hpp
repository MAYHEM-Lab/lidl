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
        view,
        generic_view,
        queue,
        other,
        enum_member,
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

    bool is_generic() const {
        return category() == categories::generic_type ||
               category() == categories::generic_view;
    }

    bool is_view() const {
        return category() == categories::view || category() == categories::generic_view;
    }

    bool is_module() const {
        return category() == categories::module;
    }

    friend name get_wire_type_name(const module& mod, const name& n);

    virtual name get_wire_type_name_impl(const module& mod, const name& your_name) const {
        assert(false && "Not implemented");
        while (true)
            ;
    }

    void set_intrinsic() {
        m_intrinsic = true;
    }

    bool is_intrinsic() const {
        return m_intrinsic;
    }

private:
    categories m_category;
    std::unique_ptr<scope> m_scope;
    mutable base* m_parent_elem = nullptr;
    bool m_intrinsic            = false;
};

template<base::categories Category>
class cbase : public base {
public:
    explicit cbase(base* parent = nullptr, std::optional<source_info> p_src_info = {})
        : base(Category, parent, std::move(p_src_info)) {
    }
};

const module* find_parent_module(const base* obj);
} // namespace lidl