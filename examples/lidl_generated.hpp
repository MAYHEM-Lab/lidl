#pragma once

#include <lidl/lidl.hpp>

class identifier {
public:
    const ::lidl::string& name() const {
        return m_raw.name;
    }
    ::lidl::string& name() {
        return m_raw.name;
    }
    const ::lidl::vector<identifier>& parameters() const {
        return m_raw.parameters;
    }
    ::lidl::vector<identifier>& parameters() {
        return m_raw.parameters;
    }

private:
    struct identifier_raw {
        ::lidl::string name;
        ::lidl::vector<identifier> parameters;
    };

    identifier_raw m_raw;
};

class type {
public:
    const identifier& name() const {
        return m_raw.name;
    }
    identifier& name() {
        return m_raw.name;
    }

private:
    struct type_raw {
        identifier name;
    };

    type_raw m_raw;
};

class member {
public:
    const type& type() const {
        return m_raw.type;
    }
    type& type() {
        return m_raw.type;
    }

private:
    struct member_raw {
        type type;
    };

    member_raw m_raw;
};

class structure {
public:
    const ::lidl::vector<member>& members() const {
        return m_raw.members;
    }
    ::lidl::vector<member>& members() {
        return m_raw.members;
    }

private:
    struct structure_raw {
        ::lidl::vector<member> members;
    };

    structure_raw m_raw;
};
