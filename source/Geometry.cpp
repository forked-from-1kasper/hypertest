#include <Hyper/Geometry.hpp>

namespace Array {
    template<std::size_t N, typename T, typename U>
    auto inverse(const std::array<Fuchsian<T>, N> & xs) {
        std::array<Möbius<U>, N> retval;

        for (size_t i = 0; i < N; i++)
            retval[i] = xs[i].inverse().template field<U>();

        return retval;
    }
}

namespace Tesselation {
    using ℤi = Gaussian<Integer>;

    // Chunk’s neighbours in tesselation
    const Fuchsian<Integer> I { ℤi(+1, +0), ℤi(+0, +0), ℤi(+0, +0), ℤi(+1, +0) };
    const Fuchsian<Integer> U { ℤi(+6, +0), ℤi(+6, +6), ℤi(+1, -1), ℤi(+6, +0) };
    const Fuchsian<Integer> L { ℤi(+6, +0), ℤi(+6, -6), ℤi(+1, +1), ℤi(+6, +0) };
    const Fuchsian<Integer> D { ℤi(+6, +0), ℤi(-6, -6), ℤi(-1, +1), ℤi(+6, +0) };
    const Fuchsian<Integer> R { ℤi(+6, +0), ℤi(-6, +6), ℤi(-1, -1), ℤi(+6, +0) };

    const Neighbours neighbours {
        U, L, D, R,
        // corners
        U * L, U * L * D, U * L * D * R,
        U * R, U * R * D, U * R * D * L,
        D * L, D * L * U, D * L * U * R,
        D * R, D * R * U, D * R * U * L
    };

    const Neighbours⁻¹ neighbours⁻¹ = Array::inverse<N, Integer, Real>(neighbours);

    // Generation of chunk’s grid
    const auto Φ(Real x, Real y) {
        using namespace Fundamentals;

        if (x == 0 && y == 0) return Gyrovector<Real>(0, 0);

        auto L = fabs(x) + fabs(y);

        Gyrovector<Real> u(Math::sign(x) * L, 0), v(0, Math::sign(y) * L);
        return u + fabs(y / L) * (-u + v);
    }

    const auto Ψ(Real x, Real y) {
        auto u = (x + y) / 2, v = (x - y) / 2;
        return Φ(u * Fundamentals::D, v * Fundamentals::D);
    }

    const auto yield(int i, int j) {
        using namespace Fundamentals;

        auto x = Real(2 * i - chunkSize) / Real(chunkSize);
        auto y = Real(2 * j - chunkSize) / Real(chunkSize);
        return Ψ(x, y);
    }

    const auto init() {
        using namespace Fundamentals;

        Array²<Gyrovector<Real>, chunkSize + 1> retval;

        for (int i = 0; i <= chunkSize; i++)
            for (int j = 0; j <= chunkSize; j++)
                retval[i][j] = yield(i, j);

        return retval;
    }

    const Grid corners = init();
}

NodeRegistry::NodeRegistry() { attach(0UL, {"Air", Texture()}); }

Chunk::Chunk(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry) : _isometry(isometry), data{} {
    // there should be better way to do this
    auto P = isometry.field<Real>();
    auto ω = P.det() / (P.d * P.d);

    if (ω.real() > 0 && ω.imag() >= 0) {}
    else if (ω.real() <= 0 && ω.imag() > 0) { _isometry.a.mulnegi(); _isometry.c.mulnegi(); }
    else if (ω.real() < 0 && ω.imag() <= 0) { _isometry.a.negate();  _isometry.c.negate();  }
    else if (ω.real() >= 0 && ω.imag() < 0) { _isometry.a.muli();    _isometry.c.muli();    }

    if (ω.real() == 0) { _isometry.a.mulnegi(); _isometry.c.mulnegi(); }

    _pos = isometry.origin();
    updateMatrix(origin);

    vao.initialize();
}

Chunk::~Chunk() { vao.free(); }

bool Chunk::walkable(Rank x, Real L, Rank z) {
    if (x >= Fundamentals::chunkSize || z >= Fundamentals::chunkSize) return true;
    return Chunk::outside(L) || (get(x, Level(L), z).id == 0);
}

using VBO = Shader<VoxelShader>::VBO;
using EBO = Shader<VoxelShader>::EBO;

void drawParallelogram(VBO & vbo, EBO & ebo, Texture & T, const Parallelogram<GLfloat> & P, GLfloat h) {
    auto index = vbo.size();
    emit(vbo, Tuple(T.left(),  T.down()), P.A, h); // 1
    emit(vbo, Tuple(T.right(), T.down()), P.B, h); // 2
    emit(vbo, Tuple(T.right(), T.up()),   P.C, h); // 3
    emit(vbo, Tuple(T.left(),  T.up()),   P.D, h); // 4

    ebo.push_back(index); ebo.push_back(index + 1); ebo.push_back(index + 2);
    ebo.push_back(index); ebo.push_back(index + 2); ebo.push_back(index + 3);
}

void drawSide(VBO & vbo, EBO & ebo, Texture & T, const Gyrovector<GLfloat> & A, const Gyrovector<GLfloat> & B, GLfloat h₁, GLfloat h₂) {
    auto index = vbo.size();
    emit(vbo, Tuple(T.right(), T.up()),   A, h₁); // 1
    emit(vbo, Tuple(T.right(), T.down()), A, h₂); // 2
    emit(vbo, Tuple(T.left(),  T.down()), B, h₂); // 3
    emit(vbo, Tuple(T.left(),  T.up()),   B, h₁); // 4

    ebo.push_back(index); ebo.push_back(index + 1); ebo.push_back(index + 2);
    ebo.push_back(index); ebo.push_back(index + 2); ebo.push_back(index + 3);
}

struct Mask { bool top : 1, bottom : 1, back : 1, forward : 1, left : 1, right : 1; };

void drawRightParallelogrammicPrism(VBO & vbo, EBO & ebo, Texture & T, Mask m, Real h, Real Δh, const Parallelogram<GLfloat> & P) {
    const auto h₁ = h, h₂ = h + Δh;

    if (m.top)     drawParallelogram(vbo, ebo, T, P, h₂);
    if (m.bottom)  drawParallelogram(vbo, ebo, T, P.rev(), h₁);

    if (m.back)    drawSide(vbo, ebo, T, P.B, P.A, h₁, h₂);
    if (m.right)   drawSide(vbo, ebo, T, P.C, P.B, h₁, h₂);
    if (m.forward) drawSide(vbo, ebo, T, P.D, P.C, h₁, h₂);
    if (m.left)    drawSide(vbo, ebo, T, P.A, P.D, h₁, h₂);
}

template<typename T> Parallelogram<T> Chunk::parallelogram(Rank i, Rank j) {
    return {
        Tesselation::corners[i + 0][j + 0], Tesselation::corners[i + 1][j + 0],
        Tesselation::corners[i + 1][j + 1], Tesselation::corners[i + 0][j + 1]
    };
}

template Parallelogram<GLfloat> Chunk::parallelogram(Rank, Rank);
template Parallelogram<Real>    Chunk::parallelogram(Rank, Rank);

void drawNode(VBO & vbo, EBO & ebo, Texture & T, Mask m, Rank x, Level y, Rank z) {
    auto P = Chunk::parallelogram<GLfloat>(x, z);
    drawRightParallelogrammicPrism(vbo, ebo, T, m, Real(y), 1, P);
}

void Chunk::refresh(NodeRegistry & nodeRegistry, const Fuchsian<Integer> & G) {
    using namespace Fundamentals;

    vao.clear();

    NodeDef nodeDef; Mask m;
    for (Level j = 0; true; j++) {
        for (Rank k = 0; k < chunkSize; k++) {
            for (Rank i = 0; i < chunkSize; i++) {
                auto id = get(i, j, k).id;

                if (id == 0) continue; // don’t draw air

                m.top     = (j == worldTop)      || (get(i + 0, j + 1, k + 0).id == 0);
                m.bottom  = (j == 0)             || (get(i + 0, j - 1, k + 0).id == 0);
                m.back    = (k == 0)             || (get(i + 0, j + 0, k - 1).id == 0);
                m.forward = (k == chunkSize - 1) || (get(i + 0, j + 0, k + 1).id == 0);
                m.left    = (i == 0)             || (get(i - 1, j + 0, k + 0).id == 0);
                m.right   = (i == chunkSize - 1) || (get(i + 1, j + 0, k + 0).id == 0);

                nodeDef = nodeRegistry.get(id);
                drawNode(vao.vertices, vao.indices, nodeDef.texture, m, i, j, k);
            }
        }

        if (j == worldTop) break;
    }

    vao.upload(GL_DYNAMIC_DRAW);
    _needRefresh = false;
}

void Chunk::updateMatrix(const Fuchsian<Integer> & origin) {
    relative = (origin.inverse() * _isometry).field<Real>();
    relative = relative.normalize();
}

void Chunk::render(Shader<VoxelShader> * shader) {
    shader->uniform("relative.a", relative.a);
    shader->uniform("relative.b", relative.b);
    shader->uniform("relative.c", relative.c);
    shader->uniform("relative.d", relative.d);

    vao.draw(GL_TRIANGLES);
}

bool Chunk::touch(const Gyrovector<Real> & w, Rank i, Rank j) {
    const auto A = Tesselation::corners[i + 0][j + 0];
    const auto B = Tesselation::corners[i + 1][j + 0];
    const auto C = Tesselation::corners[i + 1][j + 1];
    const auto D = Tesselation::corners[i + 0][j + 1];

    const auto α = w.sub(A).cross(B.sub(A));
    const auto β = w.sub(B).cross(C.sub(B));
    const auto γ = w.sub(C).cross(D.sub(C));
    const auto δ = w.sub(D).cross(A.sub(D));

    return (α < 0) == (β < 0) && (β < 0) == (γ < 0) && (γ < 0) == (δ < 0);
}

std::pair<Rank, Rank> Chunk::round(const Gyrovector<Real> & w) {
    using namespace Fundamentals;

    for (Rank i = 0; i < chunkSize; i++)
        for (Rank j = 0; j < chunkSize; j++)
            if (Chunk::touch(w, i, j))
                return std::pair(i, j);

    return std::pair(exterior, exterior);
}

bool Chunk::isInsideOfDomain(const Gyrovector<Real> & w₀) {
    using namespace Fundamentals;

    // We are using symmetry of grid along axes here
    Gyrovector<Real> w(fabs(w₀.x()), fabs(w₀.y()));

    for (Rank i = 0; i < chunkSize; i++) {
        const auto A = Tesselation::corners[chunkSize][i + 0];
        const auto B = Tesselation::corners[chunkSize][i + 1];

        const auto α = w.sub(A).cross(B.sub(A));
        const auto β = w.sub(B).cross(-B);
        const auto γ = w.cross(A);

        if ((α < 0) == (β < 0) && (β < 0) == (γ < 0)) return true;
    }

    return false;
}

Atlas::Atlas() : onLoad(nullptr) {}

Atlas::~Atlas() {
    for (auto chunk : container)
        delete chunk;
}

Chunk * Atlas::lookup(const Gaussian²<Integer> & pos) const {
    for (auto chunk : container)
        if (chunk->pos() == pos)
            return chunk;

    return nullptr;
}

Chunk * Atlas::poll(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry) {
    auto pos = isometry.origin();

    for (auto chunk : container)
        if (chunk->pos() == pos)
            return chunk;

    auto chunk = new Chunk(origin, isometry);
    container.push_back(chunk);

    if (onLoad) (*onLoad)(chunk);
    return chunk;
}

void Atlas::unload(const Gaussian²<Integer> & pos) {
    for (auto it = container.begin(); it != container.end();)
        if ((*it)->pos() == pos) { delete *it; it = container.erase(it); }
        else it++;
}

void Atlas::updateMatrix(const Fuchsian<Integer> & origin) {
    for (auto & chunk : container)
        chunk->updateMatrix(origin);
}