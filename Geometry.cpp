#include "Grid.hpp"
#include "Geometry.hpp"

NodeRegistry::NodeRegistry() { attach(0UL, NodeDef("Air", Texture())); }

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

    glGenBuffers(1, &ebo);

    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);

    constexpr GLsizei stride = 5 * sizeof(float);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *) 0); // _texCoord
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *) (2 * sizeof(float))); // _gyrovector
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void *) (4 * sizeof(float))); // _height
    glEnableVertexAttribArray(2);
}

Chunk::~Chunk() {
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
}

bool Chunk::walkable(Rank x, Real L, Rank z) {
    if (x >= Fundamentals::chunkSize || z >= Fundamentals::chunkSize) return true;
    return Chunk::outside(L) || (get(x, Level(L), z).id == 0);
}

template<typename... Ts> inline void emit(VBO & vbo, Ts... ts) { vbo.push_back(ShaderData(ts...)); }

void drawParallelogram(VBO & vbo, EBO & ebo, Texture & T, const Parallelogram<Real> & P, Real h) {
    auto index = vbo.size();
    emit(vbo, T.left(),  T.down(), P.A, h); // 1
    emit(vbo, T.right(), T.down(), P.B, h); // 2
    emit(vbo, T.right(), T.up(),   P.C, h); // 3
    emit(vbo, T.left(),  T.up(),   P.D, h); // 4

    ebo.push_back(index); ebo.push_back(index + 1); ebo.push_back(index + 2);
    ebo.push_back(index); ebo.push_back(index + 2); ebo.push_back(index + 3);
}

void drawSide(VBO & vbo, EBO & ebo, Texture & T, const Gyrovector<Real> & A, const Gyrovector<Real> & B, Real h₁, Real h₂) {
    auto index = vbo.size();
    emit(vbo, T.right(), T.up(),   A, h₁); // 1
    emit(vbo, T.right(), T.down(), A, h₂); // 2
    emit(vbo, T.left(),  T.down(), B, h₂); // 3
    emit(vbo, T.left(),  T.up(),   B, h₁); // 4

    ebo.push_back(index); ebo.push_back(index + 1); ebo.push_back(index + 2);
    ebo.push_back(index); ebo.push_back(index + 2); ebo.push_back(index + 3);
}

struct Mask { bool top : 1, bottom : 1, back : 1, forward : 1, left : 1, right : 1; };

void drawRightParallelogrammicPrism(VBO & vbo, EBO & ebo, Texture & T, Mask m, Real h, Real Δh, const Parallelogram<Real> & P) {
    const auto h₁ = h, h₂ = h + Δh;

    if (m.top)     drawParallelogram(vbo, ebo, T, P, h₂);
    if (m.bottom)  drawParallelogram(vbo, ebo, T, P.rev(), h₁);

    if (m.back)    drawSide(vbo, ebo, T, P.B, P.A, h₁, h₂);
    if (m.right)   drawSide(vbo, ebo, T, P.C, P.B, h₁, h₂);
    if (m.forward) drawSide(vbo, ebo, T, P.D, P.C, h₁, h₂);
    if (m.left)    drawSide(vbo, ebo, T, P.A, P.D, h₁, h₂);
}

void drawNode(VBO & vbo, EBO & ebo, Texture & T, Mask m, Rank x, Level y, Rank z) {
    drawRightParallelogrammicPrism(vbo, ebo, T, m, Real(y), 1,
        { Grid::corners[x + 0][z + 0],
          Grid::corners[x + 1][z + 0],
          Grid::corners[x + 1][z + 1],
          Grid::corners[x + 0][z + 1] }
    );
}

void Chunk::refresh(NodeRegistry & nodeRegistry, const Fuchsian<Integer> & G) {
    using namespace Fundamentals;

    vertices.clear(); indices.clear();

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
                drawNode(vertices, indices, nodeDef.texture, m, i, j, k);
            }
        }

        if (j == worldTop) break;
    }

    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ShaderData), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(ShaderIndex), indices.data(), GL_DYNAMIC_DRAW);
}

void Chunk::updateMatrix(const Fuchsian<Integer> & origin) {
    relative = (origin.inverse() * _isometry).field<Real>();
    relative = relative.normalize();
}

void Chunk::render(Shader * shader) {
    shader->uniform("relative.a", relative.a);
    shader->uniform("relative.b", relative.b);
    shader->uniform("relative.c", relative.c);
    shader->uniform("relative.d", relative.d);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glDrawElements(GL_TRIANGLES, indices.size(), shaderIndexType, nullptr);
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

Chunk * Atlas::poll(const Fuchsian<Integer> & origin, const Fuchsian<Integer> & isometry) {
    auto pos = isometry.origin();

    for (auto chunk : container)
        if (chunk->pos() == pos)
            return chunk;

    auto chunk = new Chunk(origin, isometry);
    container.push_back(chunk);
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