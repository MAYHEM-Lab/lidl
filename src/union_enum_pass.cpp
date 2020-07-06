#include <lidl/enumeration.hpp>
#include <lidl/module.hpp>
#include <lidl/union.hpp>

namespace lidl {
enumeration enum_for_union(const union_type& u, const module& m) {
    enumeration e;
    e.underlying_type = name{recursive_name_lookup(*m.symbols, "i8").value()};
    for (auto& [name, _] : u.members) {
        e.members.emplace_back(name, enum_member{static_cast<int>(e.members.size())});
    }
    return e;
}

void union_enum_pass(module& m) {
    for (auto& u : m.unions) {
        m.enums.emplace_back(enum_for_union(u, m));
        u.alternatives_enum = &m.enums.back();
    }
}
} // namespace lidl