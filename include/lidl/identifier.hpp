#pragma once

#include <string>
#include <vector>

namespace lidl {
struct identifier {
    std::string name;
    std::vector<identifier> parameters;

    explicit identifier(std::string just_name)
        : name{std::move(just_name)} {
    }

    explicit identifier(std::string base_name, std::vector<identifier> params)
        : name{std::move(base_name)}
        , parameters{std::move(params)} {
    }

    int8_t arity() const {
        return parameters.size();
    }
};

inline std::string to_string(const identifier& id) {
    std::string param_str;
    for (auto& param : id.parameters) {
        param_str += param.name + ",";
    }
    if (!param_str.empty()) {
        param_str = "<" + param_str;
        param_str.back() = '>';
    }
    return std::string(id.name) + param_str;
}

inline std::ostream& operator<<(std::ostream& ostr, const identifier& id) {
    return ostr << to_string(id);
}

inline bool operator<(const identifier& left, const identifier& right) {
    return left.name < right.name || left.parameters < right.parameters;
}

inline bool operator==(const identifier& left, const identifier& right) {
    return left.name == right.name && left.parameters == right.parameters;
}
} // namespace lidl