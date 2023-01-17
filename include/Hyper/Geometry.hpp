#pragma once

#include <string>
#include <vector>
#include <map>

#include <GL/glew.h>

#include <Hyper/Fundamentals.hpp>
#include <Hyper/Fuchsian.hpp>
#include <Hyper/Moebius.hpp>
#include <Hyper/Shader.hpp>
#include <Hyper/Sheet.hpp>

namespace Tesselation {
    using namespace Fundamentals;

    constexpr size_t N = 16;

    using Grid = Array²<Gyrovector<Real>, chunkSize + 1>;
    using Neighbours = std::array<Fuchsian<Integer>, N>;
    using Neighbours⁻¹ = std::array<Möbius<Real>, N>;

    extern const Fuchsian<Integer> I, U, L, D, R;

    extern const Neighbours   neighbours;
    extern const Neighbours⁻¹ neighbours⁻¹;
    extern const Grid         corners;
}

template<typename T> struct Parallelogram {
    Gyrovector<T> A, B, C, D;

    Parallelogram() {}
    Parallelogram(auto A, auto B, auto C, auto D) : A(A), B(B), C(C), D(D) {}

    const auto rev() const { return Parallelogram<T>(D, C, B, A); }
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

class Chunk {
private:
    Fuchsian<Integer> _isometry; Möbius<Real> relative; Real _awayness; // used for drawing
    Gaussian²<Integer> _pos; // used for indexing, should be equal to `isometry.origin()`
    Node data[Fundamentals::chunkSize][Fundamentals::worldHeight][Fundamentals::chunkSize];

    bool _needRefresh; Shader<VoxelShader>::VAO vao;

public:
    Chunk(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry);
    ~Chunk();

    void refresh(NodeRegistry &, const Fuchsian<Integer> &);
    void updateMatrix(const Fuchsian<Integer> &);
    void render(Shader<VoxelShader> *);

    bool walkable(Rank, Real, Rank);

    inline constexpr auto requestRefresh() { _needRefresh = true; }
    inline constexpr auto needRefresh() { return _needRefresh; }

    inline constexpr auto get(Rank i, Level j, Rank k) const { return data[i][j][k]; }

    inline constexpr auto awayness() const { return _awayness; }

    inline const auto isometry() const { return _isometry; }
    inline const auto pos()      const { return _pos;      }

    inline void set(size_t i, size_t j, size_t k, const Node & node)
    { data[i][j][k] = node; }

    template<typename T> static Parallelogram<T> parallelogram(Rank, Rank);

    static bool touch(const Gyrovector<Real> &, Rank, Rank);
    static std::pair<Rank, Rank> round(const Gyrovector<Real> &);

    static bool isInsideOfDomain(const Gyrovector<Real> &);

    inline static bool outside(Real L) { return L < 0 || L >= Fundamentals::worldHeight; }
};

using ChunkOperator = Chunk*(Chunk*);

class Atlas {
private:
    std::vector<Chunk*> container;

public:
    ChunkOperator * onLoad;

    Atlas();
    ~Atlas();

    Chunk * poll(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry);
    Chunk * lookup(const Gaussian²<Integer> &) const;

    void updateMatrix(const Fuchsian<Integer> &);
    void unload(const Gaussian²<Integer> &);

    inline const auto get() const { return container; }
};