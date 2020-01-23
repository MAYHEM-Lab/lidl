#pragma once

#include <lidl/lidl.hpp>

struct vec3f {
    float x;
    float y;
    float z;
};

static_assert(sizeof(vec3f) == 12, "Size of vec3f does not match the wire size!");
static_assert(alignof(vec3f) == 4,
              "Alignment of vec3f does not match the wire alignment!");
struct vertex {
    vec3f normal;
    vec3f position;
};

static_assert(sizeof(vertex) == 24, "Size of vertex does not match the wire size!");
static_assert(alignof(vertex) == 4,
              "Alignment of vertex does not match the wire alignment!");
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

static_assert(sizeof(texture2d) == 4, "Size of texture2d does not match the wire size!");
static_assert(alignof(texture2d) == 2,
              "Alignment of texture2d does not match the wire alignment!");
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

static_assert(sizeof(triangle) == 84, "Size of triangle does not match the wire size!");
static_assert(alignof(triangle) == 4,
              "Alignment of triangle does not match the wire alignment!");
