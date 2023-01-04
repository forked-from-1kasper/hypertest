#pragma once

#include <string>
#include <vector>
#include <map>

#include <GL/glew.h>

#include <Hyper/Fundamentals.hpp>
#include <Hyper/Fuchsian.hpp>
#include <Hyper/Möbius.hpp>
#include <Hyper/Shader.hpp>
#include <Hyper/Sheet.hpp>

template<typename T>
struct Parallelogram {
    Gyrovector<T> A, B, C, D;
    const auto rev() const { return Parallelogram(D, C, B, A); }
};

struct NodeDef { std::string name; Texture texture; };

struct Node { NodeId id; };

class NodeRegistry {
private:
    NodeDef air;
    std::map<NodeId, NodeDef> table;

public:
    NodeRegistry();

    inline void attach(NodeId id, const NodeDef & def) { table.insert({id, def}); }
    inline auto get(NodeId id) { return table[id]; }
};

using ShaderIndex = unsigned int;
constexpr GLenum shaderIndexType = GL_UNSIGNED_INT;

struct ShaderData { GLfloat tx, ty; Gyrovector<GLfloat> v; GLfloat h; };

using VBO = std::vector<ShaderData>;
using EBO = std::vector<ShaderIndex>;

class Chunk {
private:
    Fuchsian<Integer> _isometry; Möbius<Real> relative; // used for drawing
    Gaussian²<Integer> _pos; // used for indexing, should be equal to `isometry.origin()`
    Node data[Fundamentals::chunkSize][Fundamentals::worldHeight][Fundamentals::chunkSize];

    GLuint vao, vbo, ebo; VBO vertices; EBO indices;

public:
    Chunk(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry);
    ~Chunk();

    void refresh(NodeRegistry &, const Fuchsian<Integer> &);
    void updateMatrix(const Fuchsian<Integer> &);
    void render(Shader *);

    bool walkable(Rank, Real, Rank);

    inline constexpr auto get(Rank i, Level j, Rank k) const { return data[i][j][k]; }

    inline constexpr auto isometry() const { return _isometry; }
    inline constexpr auto pos()      const { return _pos;      }

    inline void set(size_t i, size_t j, size_t k, const Node & node)
    { data[i][j][k] = node; }

    static bool touch(const Gyrovector<Real> &, Rank, Rank);
    static std::pair<Rank, Rank> cell(const Gyrovector<Real> &);

    inline static bool outside(Real L) { return L < 0 || L >= Fundamentals::worldHeight; }
};

class Atlas {
private:
    std::vector<Chunk*> container;

public:
    Atlas();
    ~Atlas();

    Chunk * poll(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry);
    Chunk * lookup(const Gaussian²<Integer> &);

    void updateMatrix(const Fuchsian<Integer> &);
    void unload(const Gaussian²<Integer> &);

    inline const auto get() const { return container; }
};