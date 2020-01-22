#pragma once

#include <lidl/lidl.hpp>

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

class triangle {
public:
    const ::lidl::array<texture2d, 3>& textures() const {
        return m_raw.textures;
    }
    ::lidl::array<texture2d, 3>& textures() {
        return m_raw.textures;
    }
    const ::lidl::array<vertex, 3>& vertices() const {
        return m_raw.vertices;
    }
    ::lidl::array<vertex, 3>& vertices() {
        return m_raw.vertices;
    }

private:
    struct triangle_raw {
        ::lidl::array<texture2d, 3> textures;
        ::lidl::array<vertex, 3> vertices;
    };

    triangle_raw m_raw;
};
