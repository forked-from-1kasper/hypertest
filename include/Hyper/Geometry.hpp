#pragma once

#include <optional>
#include <string>
#include <vector>

#include <thread>

#include <GL/glew.h>
#include <sqlite3.h>

#include <Hyper/Fundamentals.hpp>
#include <Hyper/Fuchsian.hpp>
#include <Hyper/Shader.hpp>
#include <Hyper/Sheet.hpp>
#include <Hyper/AutD.hpp>
#include <Meta/List.hpp>

namespace Tesselation {
    using namespace Fundamentals;

    enum class Direction { Identity, Up, Down, Left, Right };
    using enum Direction;

    template<typename T> T interpret(Direction);

    template<Direction... ds> struct Compose {
        template<typename T> static T eval()
        { return (interpret<T>(ds) * ...); }
    };

    template<typename, typename> struct Eval;

    template<typename T> struct Eval<T, List<>>
    { static inline void insert(size_t, auto &) {} };

    template<typename T, typename U, typename... Us> struct Eval<T, List<U, Us...>> {
        static inline void insert(size_t i, auto & ret) {
            ret[i] = U::template eval<T>(); ret[i].normalize();
            Eval<T, List<Us...>>::insert(i + 1, ret);
        }
    };

    template<typename T, typename Us> std::array<T, Length<Us>> eval()
    { std::array<T, Length<Us>> retval; Eval<T, Us>::insert(0, retval); return retval; }

    using Grid = Array²<Gyrovector<Real>, chunkSize + 1>;

    using Neighbours = List<
        Compose<Up>, Compose<Left>, Compose<Down>, Compose<Right>,
        Compose<Up,   Left>,  Compose<Up,   Left,  Down>, Compose<Up,   Left,  Down, Right>,
        Compose<Up,   Right>, Compose<Up,   Right, Down>, Compose<Up,   Right, Down, Left>,
        Compose<Down, Left>,  Compose<Down, Left,  Up>,   Compose<Down, Left,  Up,   Right>,
        Compose<Down, Right>, Compose<Down, Right, Up>,   Compose<Down, Right, Up,   Left>
    >;

    constexpr size_t amount = Length<Neighbours>;

    template<typename T> using Array = std::array<T, amount>;

    extern const Fuchsian<Integer>        I, U, L, D, R;
    extern const Array<Fuchsian<Integer>> neighbours;
    extern const Array<Aut𝔻<Real>>        neighbours⁻¹;
    extern const Grid                     corners;
    extern const Real                     meter;
}

template<typename T> struct Parallelogram {
    Gyrovector<T> A, B, C, D;

    Parallelogram() {}
    Parallelogram(auto A, auto B, auto C, auto D) : A(A), B(B), C(C), D(D) {}

    const auto rev() const { return Parallelogram<T>(D, C, B, A); }
};

struct Cube { Texture top, bottom, left, right, front, back; };
struct NodeDef { std::string name; Cube cube; };

struct Node { NodeId id; };

class NodeRegistry {
private:
    NodeDef air; std::vector<NodeDef> table;

public:
    NodeRegistry();

    inline NodeId attach(const NodeDef & def)
    { table.push_back(def); return table.size() - 1; }

    inline NodeDef get(NodeId id) { return table[id]; }
    inline bool has(NodeId id) { return id < table.size(); }
};

struct Blob {
    Node data[Fundamentals::chunkSize][Fundamentals::worldHeight][Fundamentals::chunkSize];

    Blob() : data{} {}
};

class Chunk {
private:
    Fuchsian<Integer> _isometry; Möbius<Real> _relative; Real _awayness; // used for drawing
    Gaussian²<Integer> _pos; // used for indexing, should be equal to `isometry.origin()`

    bool _dirty = false, _needRefresh = false, _needUnload = false;
    Blob blob; Shader<VoxelShader>::VAO vao;

    bool working = false; std::thread worker;

public:
    Chunk(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry);
    ~Chunk();

    void updateMatrix(const Fuchsian<Integer> &);
    void refresh(NodeRegistry &);

    void render(Shader<VoxelShader> *);

    bool walkable(Rank, Real, Rank);

    void serialize(sqlite3_stmt *, int, int, int, int, int);
    void dump(sqlite3 *);
    bool load(sqlite3 *);
    void join();

    inline constexpr bool dirty()       { return _dirty;       }
    inline constexpr bool needRefresh() { return _needRefresh; }
    inline constexpr bool needUnload()  { return _needUnload;  }

    inline constexpr void unload()         { _needUnload = true;  }
    inline constexpr auto requestRefresh() { _needRefresh = true; }

    inline constexpr auto get(Rank i, Level j, Rank k) const { return blob.data[i][j][k]; }

    inline constexpr auto awayness() const { return _awayness; }

    inline const auto isometry() const { return _isometry; }
    inline const auto relative() const { return _relative; }
    inline const auto pos()      const { return _pos;      }

    inline void set(size_t i, size_t j, size_t k, const Node & node)
    { _dirty = true; blob.data[i][j][k] = node; }

    template<typename T> static Parallelogram<T> parallelogram(Rank, Rank);

    static bool touch(const Gyrovector<Real> &, Rank, Rank);
    static std::pair<Rank, Rank> round(const Gyrovector<Real> &);

    static bool isInsideOfDomain(const Gyrovector<Real> &);
    static std::optional<size_t> matchNeighbour(const Gyrovector<Real> &);

    inline static bool outside(Real L) { return L < 0 || L >= Fundamentals::worldHeight; }
};

using ChunkOperator = Chunk*(Chunk*);

class Atlas {
private:
    sqlite3 * engine;

public:
    std::vector<Chunk*> pool;
    ChunkOperator * generator = nullptr;

    Atlas();
    ~Atlas();

    void connect(std::string &);
    void disconnect();

    void dump();

    Chunk * poll(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry);
    Chunk * lookup(const Gaussian²<Integer> &) const;

    void updateMatrix(const Fuchsian<Integer> &);
};

template<typename T> struct Bitfield {
    T value;

    Bitfield(T value) : value(value) {}
    inline operator T() { return value; }

    inline void set(size_t n, bool bit) { value = (value | (1 << n)) & ~(T(!bit) << n); }
    inline bool get(size_t n) const { return (value >> n) & 1; }
};
