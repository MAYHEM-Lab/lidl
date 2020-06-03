#include <lidl/layout.hpp>

namespace lidl {
namespace {
size_t pad_to(size_t current, size_t needed_alignment) {
    int padding = 0;
    while ((current + padding) % needed_alignment) {
        padding++;
    }
    return padding;
}
} // namespace

compound_layout& compound_layout::add_member(std::string_view member_name,
                                             const raw_layout& member_layout) {
    if (offset_of(member_name)) {
        throw std::runtime_error("Member already exists!");
    }

    // The overall alignment of the compound type.
    auto align_to =
        std::lcm(std::max<int>(1, m_current.alignment()), member_layout.alignment());

    // Align the beginning address of the new member to the correct alignment
    auto padding = pad_to(m_current.size() - m_padding, member_layout.alignment());

    auto member_offset = m_current.size() - m_padding + padding;

    // Align the entire object to the correct alignment
    auto new_padding = pad_to(member_offset + member_layout.size(), align_to);

    m_current =
        raw_layout{member_offset + member_layout.size() + new_padding, size_t(align_to)};

    m_padding = new_padding;

    m_offsets.emplace(std::string(member_name), member_offset);

    return *this;
}

std::optional<size_t> compound_layout::offset_of(std::string_view member_name) const {
    auto it = m_offsets.find(std::string(member_name));
    if (it == m_offsets.end()) {
        return std::nullopt;
    }
    return it->second;
}
} // namespace lidl