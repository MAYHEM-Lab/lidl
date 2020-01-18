#pragma once

#include <lidl/lidl.hpp>

template<typename T>
class vec3 {
public:
    const T& x() const {
        return m_raw.x;
    }
    T& x() {
        return m_raw.x;
    }
    const T& y() const {
        return m_raw.y;
    }
    T& y() {
        return m_raw.y;
    }
    const T& z() const {
        return m_raw.z;
    }
    T& z() {
        return m_raw.z;
    }

private:
    struct vec3_raw {
        T x;
        T y;
        T z;
    };

    vec3_raw m_raw;
};

struct vec3f {
    float x;
    float y;
    float z;
};

struct vertex {
    vec3f normal;
    vec3f position;
};

class texture2d {
public:
    const ::lidl::string& path() const {
        return m_raw.path;
    }
    ::lidl::string& path() {
        return m_raw.path;
    }

private:
    struct texture2d_raw {
        ::lidl::string path;
    };

    texture2d_raw m_raw;
};

struct triangle {
    ::lidl::array<vertex, 3> vertices;
};
