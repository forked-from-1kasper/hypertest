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

    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);

    constexpr GLsizei stride = 5 * sizeof(float);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *) 0); // _texCoord
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *) (2 * sizeof(float))); // _pos
    glEnableVertexAttribArray(1);
}

Chunk::~Chunk() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

bool Chunk::walkable(Rank x, Real L, Rank z) {
    if (x >= Fundamentals::chunkSize || z >= Fundamentals::chunkSize) return true;
    return Chunk::outside(L) || (get(x, Level(L), z).id == 0);
}

inline void float2(VBO & vbo, GLfloat x, GLfloat y)
{ vbo.push_back(x); vbo.push_back(y); }

inline void float3(VBO & vbo, GLfloat x, GLfloat y, GLfloat z)
{ vbo.push_back(x); vbo.push_back(y); vbo.push_back(z); }

void drawParallelogram(VBO & vbo, Texture & T, const Parallelogram & P, Real h) {
    float2(vbo, T.left(),  T.down()); float3(vbo, P.A.x, h, P.A.y);
    float2(vbo, T.right(), T.down()); float3(vbo, P.B.x, h, P.B.y);
    float2(vbo, T.right(), T.up());   float3(vbo, P.C.x, h, P.C.y);
    float2(vbo, T.left(),  T.up());   float3(vbo, P.D.x, h, P.D.y);
}

void drawSide(VBO & vbo, Texture & T, const glm::vec2 & A, const glm::vec2 & B, Real h₁, Real h₂) {
    float2(vbo, T.right(), T.up());   float3(vbo, A.x, h₁, A.y);
    float2(vbo, T.right(), T.down()); float3(vbo, A.x, h₂, A.y);
    float2(vbo, T.left(),  T.down()); float3(vbo, B.x, h₂, B.y);
    float2(vbo, T.left(),  T.up());   float3(vbo, B.x, h₁, B.y);
}

void drawRightParallelogrammicPrism(VBO & vbo, Texture & T, Real h, Real Δh, const Parallelogram & P) {
    const auto h₁ = h, h₂ = h + Δh;

    drawParallelogram(vbo, T, P, h₂);       // Top
    drawParallelogram(vbo, T, P.rev(), h₁); // Bottom

    drawSide(vbo, T, P.B, P.A, h₁, h₂);
    drawSide(vbo, T, P.C, P.B, h₁, h₂);
    drawSide(vbo, T, P.D, P.C, h₁, h₂);
    drawSide(vbo, T, P.A, P.D, h₁, h₂);
}

void drawNode(VBO & vbo, Texture & T, Möbius<Real> M, Rank x, Level y, Rank z) {
    drawRightParallelogrammicPrism(vbo, T, Real(y), 1,
        { Projection::apply(M.apply(Grid::corners[x + 0][z + 0])),
          Projection::apply(M.apply(Grid::corners[x + 1][z + 0])),
          Projection::apply(M.apply(Grid::corners[x + 1][z + 1])),
          Projection::apply(M.apply(Grid::corners[x + 0][z + 1])) }
    );
}

void Chunk::render(NodeRegistry & nodeRegistry, Möbius<Real> & M, const Fuchsian<Integer> & G) {
    using namespace Fundamentals;

    auto N = M * (G.inverse() * isometry()).field<Real>();

    vertices.clear();

    NodeDef nodeDef;
    for (Level j = 0; true; j++) {
        for (Rank k = 0; k < chunkSize; k++) {
            for (Rank i = 0; i < chunkSize; i++) {
                auto id = get(i, j, k).id;

                if (id == 0) continue; // don’t draw air

                nodeDef = nodeRegistry.get(id);
                drawNode(vertices, nodeDef.texture, N, i, j, k);
            }
        }

        if (j == worldTop) break;
    }

    auto size = vertices.size();

    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_QUADS, 0, size);
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