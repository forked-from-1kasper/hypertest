#include <GL/glew.h>

#include "Grid.hpp"
#include "Geometry.hpp"

NodeRegistry::NodeRegistry() { attach(0UL, NodeDef("Air", Texture())); }

Chunk::Chunk(const Fuchsian<Integer> & isometry) : _isometry(isometry), data{} {
    // there should be better way to do this
    auto P = isometry.field<Real>();
    auto ω = P.det() / (P.d * P.d);

    if (ω.real() > 0 && ω.imag() >= 0) {}
    else if (ω.real() <= 0 && ω.imag() > 0) { _isometry.a.mulnegi(); _isometry.c.mulnegi(); }
    else if (ω.real() < 0 && ω.imag() <= 0) { _isometry.a.negate();  _isometry.c.negate();  }
    else if (ω.real() >= 0 && ω.imag() < 0) { _isometry.a.muli();    _isometry.c.muli();    }

    _pos = isometry.origin();
}

bool Chunk::walkable(Rank x, Real L, Rank z) {
    if (x >= Fundamentals::chunkSize || z >= Fundamentals::chunkSize) return true;
    return Chunk::outside(L) || (get(x, Level(L), z).id == 0);
}

void drawParallelogram(Texture & T, const Parallelogram<Real> & P, const Vector3<Real> n, Real h) {
    glNormal3d(n.x, n.y, n.z);

    glTexCoord2d(T.left(),  T.down()); glVertex3d(P.A.x, h, P.A.y);
    glTexCoord2d(T.right(), T.down()); glVertex3d(P.B.x, h, P.B.y);
    glTexCoord2d(T.right(), T.up());   glVertex3d(P.C.x, h, P.C.y);
    glTexCoord2d(T.left(),  T.up());   glVertex3d(P.D.x, h, P.D.y);
}

void drawSide(Texture & T, const Vector2<Real> & A, const Vector2<Real> & B, Real h₁, Real h₂) {
    Vector3<Real> v₁(0.0, h₂ - h₁, 0.0), v₂(B.x - A.x, 0.0, B.y - A.y);

    auto n = cross(v₁, v₂);
    glNormal3d(n.x, n.y, n.z);

    glTexCoord2d(T.right(), T.up());   glVertex3d(A.x, h₁, A.y);
    glTexCoord2d(T.right(), T.down()); glVertex3d(A.x, h₂, A.y);
    glTexCoord2d(T.left(),  T.down()); glVertex3d(B.x, h₂, B.y);
    glTexCoord2d(T.left(),  T.up());   glVertex3d(B.x, h₁, B.y);
}

void drawRightParallelogrammicPrism(Texture & T, Real h, Real Δh, const Parallelogram<Real> & P) {
    const auto h₁ = h, h₂ = h + Δh;

    glBegin(GL_QUADS);

    drawParallelogram(T, P, Vector3<Real>(0, +1, 0), h₂); // Top
    drawParallelogram(T, P.rev(), Vector3<Real>(0, -1, 0), h₁); // Bottom

    drawSide(T, P.B, P.A, h₁, h₂);
    drawSide(T, P.C, P.B, h₁, h₂);
    drawSide(T, P.D, P.C, h₁, h₂);
    drawSide(T, P.A, P.D, h₁, h₂);

    glEnd();
}

void drawNode(Texture & T, Möbius<Real> M, Rank x, Level y, Rank z) {
    drawRightParallelogrammicPrism(T, Real(y), 1,
        { Projection::apply(M.apply(Grid::corners[x + 0][z + 0])),
          Projection::apply(M.apply(Grid::corners[x + 1][z + 0])),
          Projection::apply(M.apply(Grid::corners[x + 1][z + 1])),
          Projection::apply(M.apply(Grid::corners[x + 0][z + 1])) }
    );
}

void Chunk::render(NodeRegistry & nodeRegistry, Möbius<Real> & M, const Fuchsian<Integer> & G) {
    using namespace Fundamentals;

    auto N = M * (G.inverse() * isometry()).field<Real>();

    NodeDef nodeDef;
    for (Level j = 0; true; j++) {
        for (Rank k = 0; k < chunkSize; k++) {
            for (Rank i = 0; i < chunkSize; i++) {
                auto id = get(i, j, k).id;

                if (id == 0) continue; // don’t draw air

                nodeDef = nodeRegistry.get(id);
                drawNode(nodeDef.texture, N, i, j, k);
            }
        }

        if (j == worldTop) break;
    }
}

bool Chunk::touch(const Gyrovector<Real> & w, Rank i, Rank j) {
    const auto A = Grid::corners[i + 0][j + 0];
    const auto B = Grid::corners[i + 1][j + 0];
    const auto C = Grid::corners[i + 1][j + 1];
    const auto D = Grid::corners[i + 0][j + 1];

    const auto α = (w.x() - A.x()) * (B.y() - A.y()) - (B.x() - A.x()) * (w.y() - A.y());
    const auto β = (w.x() - B.x()) * (C.y() - B.y()) - (C.x() - B.x()) * (w.y() - B.y());
    const auto γ = (w.x() - C.x()) * (D.y() - C.y()) - (D.x() - C.x()) * (w.y() - C.y());
    const auto δ = (w.x() - D.x()) * (A.y() - D.y()) - (A.x() - D.x()) * (w.y() - D.y());

    return (α < 0) == (β < 0) && (β < 0) == (γ < 0) && (γ < 0) == (δ < 0);
}

std::pair<Rank, Rank> Chunk::cell(const Gyrovector<Real> & w) {
    using namespace Fundamentals;

    for (Rank i = 0; i < chunkSize; i++)
        for (Rank j = 0; j < chunkSize; j++)
            if (Chunk::touch(w, i, j)) return std::pair(i, j);

    return std::pair(exterior, exterior);
}

Atlas::Atlas() {}

Atlas::~Atlas() {
    for (auto chunk : container)
        delete chunk;
}

Chunk * Atlas::lookup(const Gaussian²<Integer> & pos) {
    for (auto chunk : container)
        if (chunk->pos() == pos)
            return chunk;

    return nullptr;
}

Chunk * Atlas::poll(const Fuchsian<Integer> & isometry) {
    auto pos = isometry.origin();

    for (auto chunk : container)
        if (chunk->pos() == pos)
            return chunk;

    auto chunk = new Chunk(isometry);
    container.push_back(chunk);
    return chunk;
}

void Atlas::unload(const Gaussian²<Integer> & pos) {
    for (auto it = container.begin(); it != container.end();)
        if ((*it)->pos() == pos) { delete *it; it = container.erase(it); }
        else it++;
}