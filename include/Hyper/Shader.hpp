#pragma once

#include <GL/glew.h>
#include <string>

#include <Hyper/Gyrovector.hpp>
#include <Literal.hpp>
#include <Tuple.hpp>
#include <List.hpp>

template<Literal lit, typename T, GLenum t, GLint n>
struct Field {
    constexpr static auto param = lit.unquote;
    constexpr static auto type  = t;
    constexpr static auto dim   = n;
    using value = T;
};

template<typename T> using Value = T::value;

namespace GVA {
    template<size_t stride, Empty U> inline void attrib(size_t, size_t) {}

    template<size_t stride, Inhabited U> inline void attrib(size_t index, size_t pointer) {
        glVertexAttribPointer(index, Head<U>::dim, Head<U>::type, GL_FALSE, stride, (void *) pointer);
        glEnableVertexAttribArray(index);

        attrib<stride, Tail<U>>(index + 1, pointer + sizeof(Value<Head<U>>));
    }

    template<size_t stride, typename U> inline void attrib()
    { attrib<stride, U>(0, 0); }

    template<size_t stride, Empty U> inline void bind(GLuint, size_t) {}

    template<size_t stride, Inhabited U> inline void bind(GLuint program, size_t index) {
        glBindAttribLocation(program, index, Head<U>::param);
        bind<stride, Tail<U>>(program, index + 1);
    };

    template<size_t stride, typename U> inline void bind(GLuint program)
    { bind<stride, U>(program, 0); }
}

class Shader {
private:
    GLuint _index;

public:
    Shader(const char *, const char * vertex, const char * fragment);
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

using ShaderData = Apply<Tuple, Map<Value, ShaderParams>>;
constexpr size_t shaderStride = sizeof(ShaderData);

using VBO = std::vector<ShaderData>;
using EBO = std::vector<ShaderIndex>;

template<typename... Ts> inline void emit(VBO & vbo, const Ts&... ts)
{ vbo.push_back(Tuple(ts...)); }