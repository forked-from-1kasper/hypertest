#pragma once

#include <optional>
#include <vector>
#include <cstdio>

#include <GL/glew.h>

#include <Math/Gyrovector.hxx>

#include <Meta/Literal.hxx>
#include <Meta/Basic.hxx>
#include <Meta/Tuple.hxx>
#include <Meta/List.hxx>

namespace GL {
    inline constexpr size_t size(GLenum index) {
        switch (index) {
            case GL_BYTE:           return sizeof(GLbyte);
            case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
            case GL_SHORT:          return sizeof(GLshort);
            case GL_UNSIGNED_SHORT: return sizeof(GLushort);
            case GL_INT:            return sizeof(GLint);
            case GL_UNSIGNED_INT:   return sizeof(GLuint);
            case GL_FIXED:          return sizeof(GLfixed);
            case GL_HALF_FLOAT:     return sizeof(GLhalf);
            case GL_FLOAT:          return sizeof(GLfloat);
            case GL_DOUBLE:         return sizeof(GLdouble);
            default:                return 0;
        }
    }

    template<typename T> constexpr GLenum EncodeM() {
        if constexpr(std::same_as<T, GLbyte>)
            return GL_BYTE;
        else if constexpr(std::same_as<T, GLubyte>)
            return GL_UNSIGNED_BYTE;
        else if constexpr(std::same_as<T, GLshort>)
            return GL_SHORT;
        else if constexpr(std::same_as<T, GLushort>)
            return GL_UNSIGNED_SHORT;
        else if constexpr(std::same_as<T, GLint>)
            return GL_INT;
        else if constexpr(std::same_as<T, GLuint>)
            return GL_UNSIGNED_INT;
        else if constexpr(std::same_as<T, GLfixed>)
            return GL_FIXED;
        else if constexpr(std::same_as<T, GLhalf>)
            return GL_HALF_FLOAT;
        else if constexpr(std::same_as<T, GLfloat>)
            return GL_FLOAT;
        else if constexpr(std::same_as<T, GLdouble>)
            return GL_DOUBLE;
        else
            static_assert(falsehood<T>, "Unexpected OpenGL type");
    }

    template<typename T> constexpr GLenum encode = EncodeM<T>();

    template<typename T> void uniform(GLuint, const char *, const T &);
}

template<Literal literal, typename T, GLenum t, GLint n>
struct Attrib {
    using value = T;

    constexpr static auto param = literal.unquote;
    constexpr static auto type  = t;
    constexpr static auto dim   = n;

    constexpr static size_t size = GL::size(t) * n;

    static_assert(std::is_standard_layout_v<T>);
    static_assert(sizeof(T) == size);
};

template<GLenum type> class AbstractShader {
public:
    GLuint ref;

    template<std::convertible_to<const char *>... Ts> inline AbstractShader(Ts... ts) {
        ref = glCreateShader(type);

        const char * bufarr[] = {ts...};

        glShaderSource(ref, sizeof...(Ts), bufarr, 0);

        glCompileShader(ref);

        GLint status; glGetShaderiv(ref, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            GLint loglen; glGetShaderiv(ref, GL_INFO_LOG_LENGTH, &loglen);

            std::vector<char> logbuf(loglen);

            glGetShaderInfoLog(ref, loglen, 0, logbuf.data());
            std::fprintf(stderr, "Shader compilation error:\n%s\n", logbuf.data());
        }
    }

    inline ~AbstractShader() { glDeleteShader(ref); }

    inline void attach(GLuint program) { glAttachShader(program, ref); }
    inline void compile() { glCompileShader(ref); }
};

using FragmentShader = AbstractShader<GL_FRAGMENT_SHADER>;
using VertexShader   = AbstractShader<GL_VERTEX_SHADER>;

template<typename T> struct AnyShaderM
{ static constexpr bool value = false; };

template<GLenum type> struct AnyShaderM<AbstractShader<type>>
{ static constexpr bool value = true; };

template<typename T> concept AnyShader = AnyShaderM<T>::value;

template<typename T> using Value = typename T::value;

namespace GVA {
    template<size_t stride, EmptyList T> inline void attrib(size_t, size_t) {}

    template<size_t stride, NonEmptyList T> inline void attrib(size_t index, size_t pointer) {
        glVertexAttribPointer(index, Head<T>::dim, Head<T>::type, GL_FALSE, stride, reinterpret_cast<void *>(pointer));
        glEnableVertexAttribArray(index);

        attrib<stride, Tail<T>>(index + 1, pointer + Head<T>::size);
    }

    template<size_t stride, AnyList T> inline void attrib()
    { attrib<stride, T>(0, 0); }

    template<size_t stride, EmptyList T> inline void bind(GLuint, size_t) {}

    template<size_t stride, NonEmptyList T> inline void bind(GLuint program, size_t index) {
        glBindAttribLocation(program, index, Head<T>::param);
        bind<stride, Tail<T>>(program, index + 1);
    };

    template<size_t stride, AnyList T> inline void bind(GLuint program)
    { bind<stride, T>(program, 0); }

    template<typename... Ts> struct SizeM
    { static inline constexpr size_t value = (Ts::size + ...); };

    template<AnyList T> inline constexpr auto size = Apply<SizeM, T>::value;
}

template<typename T> concept ShaderSpec =
requires() { typename T::Index; typename T::Params; };

template<ShaderSpec Spec> class ShaderProgram {
private:
    GLuint ref;

public:
    using Index  = typename Spec::Index;
    using Params = typename Spec::Params;

    using Data = Apply<Tuple, Map<Value, Params>>;

    static_assert(std::is_standard_layout_v<Data>);
    static_assert(sizeof(Data) == GVA::size<Params>);

    constexpr static GLenum indexType = GL::encode<Index>;
    constexpr static size_t stride = sizeof(Data);

    using VBO = std::vector<Data>;
    using EBO = std::vector<Index>;

    template<AnyShader... Ts> inline ShaderProgram(Ts & ... shaders) {
        ref = glCreateProgram();

        (shaders.attach(ref), ...);

        GVA::bind<stride, Params>(ref);
        glLinkProgram(ref);

        GLint status; glGetProgramiv(ref, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            GLint loglen; glGetProgramiv(ref, GL_INFO_LOG_LENGTH, &loglen);

            std::vector<char> logbuf(loglen);

            glGetProgramInfoLog(ref, loglen, 0, logbuf.data());

            std::fprintf(stderr, "Shader program linking failure:\n%s\n", logbuf.data());
        }
    }

    inline ~ShaderProgram() { glDeleteProgram(ref); }

    inline constexpr auto index() const { return ref; }

    inline static void attrib() { GVA::attrib<stride, Params>(); }

    template<typename T> void uniform(const char * loc, const T & value)
    { GL::uniform<T>(ref, loc, value); };

    inline void activate() { glUseProgram(ref); }

    struct VAO {
        GLuint vao, vbo, ebo;
        GLsizei count = 0;
        VBO vertices;
        EBO indices;

        void initialize() {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            attrib();
        }

        void upload(const GLenum usage) {
            glBindVertexArray(vao);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * stride, vertices.data(), usage);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(Index), indices.data(), usage);

            glBindVertexArray(0);

            count = indices.size();
        }

        void draw(const GLenum type) {
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glDrawElements(type, count, indexType, nullptr);
        }

        inline Index index() { return vertices.size(); }

        inline void push() { indices.push_back(vertices.size()); }
        inline void push(const Index index) { indices.push_back(index); }

        template<typename... Ts> inline void emit(const Ts & ... ts)
        { vertices.push_back(Tuple(ts...)); }

        void clear() {
            vertices.clear();
            indices.clear();
        }

        void free() {
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
            glDeleteVertexArrays(1, &vao);
        }
    };
};

enum class Status { Inactive, Issued, Working };

template<typename T, typename Msg> class PBO {
    using Result = std::optional<std::pair<T, Msg>>;
    using enum Status;

private:
    GLenum format; GLsizei width, height;
    Status status = Inactive; Msg message;
    GLuint buffer; GLsync sync;

public:
    PBO(GLenum format, GLsizei width, GLsizei height) : format(format), width(width), height(height) {}

    void initialize() {
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
        glBufferData(GL_PIXEL_PACK_BUFFER, sizeof(T) * width * height, nullptr, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    void issue(Msg value) {
        if (status == Inactive) {
            message = value;
            status  = Issued;
        }
    }

    Result read(GLint x, GLint y) {
        switch (status) {
            case Issued: {
                glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);

                glReadPixels(x, y, width, height, format, GL::encode<T>, nullptr);
                sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

                glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

                status = Working;
                return std::nullopt;
            }

            case Working: {
                Result retval = std::nullopt;

                if (glClientWaitSync(sync, 0, 0) == GL_ALREADY_SIGNALED) {
                    status = Inactive;

                    glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
                    auto value = static_cast<T *>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

                    if (value != nullptr) {
                        retval = std::optional(std::pair(*value, message));
                        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                    }

                    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                    glDeleteSync(sync);
                }

                return retval;
            }

            default: return std::nullopt;
        }
    }

    void free() {
        glDeleteBuffers(1, &buffer);
    }
};

struct VoxelShaderSpec {
    using Index = GLuint;

    using Params =
    List<Attrib<"_texCoord", vec2, GL_FLOAT, 2>,
         Attrib<"_vertex",   vec3, GL_FLOAT, 3>>;
};

using VoxelShader = ShaderProgram<VoxelShaderSpec>;

struct DummyShaderSpec {
    using Index = GLuint;

    using Params =
    List<Attrib<"_vertex",    vec3,    GL_FLOAT, 3>,
         Attrib<"_color",     vec4,    GL_FLOAT, 4>,
         Attrib<"_texCoord",  vec2,    GL_FLOAT, 2>,
         Attrib<"_mixFactor", GLfloat, GL_FLOAT, 1>>;
};

using DummyShader = ShaderProgram<DummyShaderSpec>;
