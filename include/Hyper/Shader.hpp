#pragma once

#include <GL/glew.h>
#include <string>

#include <Hyper/Gyrovector.hpp>
#include <Literal.hpp>
#include <Tuple.hpp>
#include <List.hpp>

template<size_t, typename> struct GVA;

template<size_t stride> struct GVA<stride, List<>> {
    inline static void attrib() {}
    inline static void attrib(size_t, size_t) {}

    inline static void bind(GLuint) {}
    inline static void bind(GLuint, size_t) {}
};

template<size_t stride, Literal param, typename T, GLenum type, GLint dim, typename... Ts>
struct GVA<stride, List<Field<param, T, type, dim>, Ts...>> {
    inline static void attrib(size_t index, size_t pointer) {
        glVertexAttribPointer(index, dim, type, GL_FALSE, stride, (void *) pointer);
        glEnableVertexAttribArray(index);

        GVA<stride, List<Ts...>>::attrib(index + 1, pointer + sizeof(T));
    }

    inline static void bind(GLuint program, size_t index) {
        glBindAttribLocation(program, index, param.unquote);

        GVA<stride, List<Ts...>>::bind(program, index + 1);
    }

    inline static void attrib() { attrib(0, 0); }
    inline static void bind(GLuint program) { bind(program, 0); }
};

class Shader {
private:
    GLuint _index;

public:
    Shader(const char * vertex, const char * fragment);
    ~Shader();

    template<typename T> void uniform(const char *, const T &) const;
    void activate();

    inline constexpr auto index() const { return _index; }
};

using ShaderIndex = unsigned int;
constexpr GLenum shaderIndexType = GL_UNSIGNED_INT;

using ShaderParams =
List<Field<"_texCoord",   Tuple<GLfloat, GLfloat>, GL_FLOAT, 2>,
     Field<"_gyrovector", Gyrovector<GLfloat>,     GL_FLOAT, 2>,
     Field<"_height",     GLfloat,                 GL_FLOAT, 1>>;

using ShaderData = Apply<Tuple, Map<FieldType, ShaderParams>>;
constexpr size_t shaderStride = sizeof(ShaderData);

using VBO = std::vector<ShaderData>;
using EBO = std::vector<ShaderIndex>;

template<typename... Ts> inline void emit(VBO & vbo, const Ts&... ts)
{ vbo.push_back(Tuple(ts...)); }