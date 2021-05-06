#include <lidl/base.hpp>

namespace lidl {
base::base(categories category, base* parent, std::optional<source_info> p_src_info)
    : src_info(std::move(p_src_info))
    , m_category(category)
    , m_parent_elem{parent}
    , m_scope(std::make_unique<scope>(*this)) {
}

base::base(const base& rhs)
    : src_info(rhs.src_info)
    , m_parent_elem(rhs.m_parent_elem)
    , m_scope(std::make_unique<scope>(*rhs.m_scope)) {
    m_scope->set_object(*this);
}
} // namespace lidl