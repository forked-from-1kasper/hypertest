#include <Hyper/Physics.hpp>

std::pair<Position, bool> Position::move(const Gyrovector<Real> & v, const Real dt) const {
    auto P = _domain * Möbius<Real>::translate(v.scale(dt)); P = P.normalize();

    if (Chunk::isInsideOfDomain(P.origin()))
        return std::pair(Position(P, _action, _center), false);

    for (size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        const auto Δ   = Tesselation::neighbours[k];
        const auto Δ⁻¹ = Tesselation::neighbours⁻¹[k];
        const auto Q   = (Δ⁻¹ * P).normalize();

        if (Chunk::isInsideOfDomain(Q.origin()))
            return std::pair(Position(Q, _action * Δ), true);
    }

    return std::pair(*this, false);
}

void Object::rotate(const Real Δyaw, const Real Δpitch) {
    constexpr auto ε = 1e-6;
    yaw += Δyaw; pitch += Δpitch;

    yaw   = std::fmod(yaw, τ);
    pitch = std::clamp(pitch, -τ/4 + ε, τ/4 - ε);
}

glm::vec3 Object::direction() const {
    auto dx = cos(pitch) * sin(yaw);
    auto dy = sin(pitch);
    auto dz = cos(pitch) * cos(yaw);

    return glm::vec3(dx, dy, dz);
}

glm::vec3 Object::right() const
{ return glm::vec3(sin(yaw - τ/4), 0.0, cos(yaw - τ/4)); }

bool Entity::isFree(Chunk * C, Rank x, Real L, Rank z)
{ return !C || (C->walkable(x, std::floor(L), z) && C->walkable(x, std::floor(L + height), z)); }

bool Entity::moveHorizontally(const Gyrovector<Real> & v, const Real dt) {
    Chunk * C; auto [P, chunkChanged] = camera().position.move(v, dt);
    C = chunkChanged ? atlas()->poll(camera().position.action(), P.action()) : chunk();

    auto Q = (C->isometry().inverse() * P.action()).field<Real>() * P.domain();
    auto [i, j] = Chunk::cell(Q.origin());

    if (!isFree(C, i, camera().climb, j)) return false;

    _chunk = C; _camera.position = P; _i = i; _j = j;
    return chunkChanged;
}

void Entity::moveVertically(const Real dt) {
    _camera.roc -= dt * gravity;

    auto L = camera().climb + dt * camera().roc;

    if (isFree(_chunk, _i, L, _j)) { _camera.climb = L; _camera.flying = true; }
    else _camera.roc = 0;

    if (!chunk()->walkable(_i, std::floor(L), _j)) _camera.flying = false;
}

bool Entity::move(const Gyrovector<Real> & v, Real dt)
{ bool chunkChanged = moveHorizontally(v, dt); moveVertically(dt); return chunkChanged; }

void Entity::teleport(const Position & P, const Real climb) {
    _camera.position = P; _camera.climb = climb;
    _chunk = atlas()->lookup(P.center());
}