#pragma once

#include <map>
#include <vector>
#include <string>

#include "Sheet.hpp"
#include "Fuchsian.hpp"
#include "Gyrovector.hpp"
#include "Fundamentals.hpp"

template<typename T> struct Parallelogram {
    Vector2<T> A, B, C, D;
    constexpr auto rev() const { return Parallelogram<T>(D, C, B, A); }
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
    Fuchsian<Integer> _isometry; // used only for drawing
    Gaussian²<Integer> _pos; // used for indexing, should be equal to `isometry.origin()`
    Node data[Fundamentals::chunkSize][Fundamentals::worldHeight][Fundamentals::chunkSize];

    std::vector<Vector3<Real>> vbo;

public:
    Chunk(const Fuchsian<Integer> & isometry);

    void render(NodeRegistry &, Möbius<Real> &, const Fuchsian<Integer> &);
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

    Chunk * poll(const Fuchsian<Integer> &);
    Chunk * lookup(const Gaussian²<Integer> &);
    void unload(const Gaussian²<Integer> &);

    inline const auto get() const { return container; }
};