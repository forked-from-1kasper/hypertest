#include <Hyper/Physics.hpp>

std::pair<Position, bool> Position::move(const Gyrovector<Real> & v) const {
    auto P = _domain * Aut𝔻<Real>(v); P.normalize();

    if (Chunk::isInsideOfDomain(P.origin()))
        return std::pair(Position(P, _action, _center), false);

    for (size_t k = 0; k < Tesselation::neighbours.size(); k++) {
        const auto & Δ   = Tesselation::neighbours[k];
        const auto & Δ⁻¹ = Tesselation::neighbours⁻¹[k];

        auto Q = (Δ⁻¹ * P); Q.normalize();

        if (Chunk::isInsideOfDomain(Q.origin()))
            return std::pair(Position(Q, _action * Δ), true);
    }

    return std::pair(*this, false);
}

std::pair<Rank, Rank> Position::round(const Chunk * C) const {
    auto Q = (C->isometry().inverse() * _action).field<Real>() * Möbius<Real>(_domain);
    return Chunk::round(Q.origin());
}

void Object::rotate(const Real Δyaw, const Real Δpitch, const Real Δroll) {
    constexpr auto ε = 1e-6;

    yaw   = std::fmod(yaw + Δyaw, τ);
    pitch = std::clamp(pitch + Δpitch, -τ/4 + ε, τ/4 - ε);
    roll  = std::fmod(roll + Δroll, τ);
}

glm::vec3 Object::direction() const {
    return glm::vec3(
        cos(pitch) * sin(yaw),
        sin(pitch),
        cos(pitch) * cos(yaw)
    );
}

glm::vec3 Object::right() const {
    return glm::vec3(
        cos(roll) * sin(yaw - τ/4),
        sin(roll),
        cos(roll) * cos(yaw - τ/4)
    );
}

bool Entity::stuck(Chunk * C, Rank x, Real y, Rank z) {
    if (C == nullptr) return false;

    for (Level L = std::floor(y); L <= std::floor(y + height); L++)
        if (!Chunk::outside(L) && !C->walkable(x, L, z))
            return true;

    return false;
}

bool Entity::moveHorizontally(const Gyrovector<Real> & v, const Real dt) {
    auto [P, chunkChanged] = _camera.position.move(v.scale(dt));
    auto C = chunkChanged ? atlas()->poll(_camera.position.action(), P.action()) : chunk();

    if (C != nullptr) {
        auto [i, j] = P.round(C);
        if (stuck(C, i, _camera.climb, j)) return false;
        _i = i; _j = j;
    }

    _chunk = C; _camera.position = P; return chunkChanged;
}

bool Entity::moveVertically(const Real dt) {
    auto roc = _camera.roc - dt * gravity;
    if (jumped) { roc += jumpSpeed; jumped = false; }

    auto L = _camera.climb + dt * roc;

    if (stuck(_chunk, _i, L, _j)) { _camera.roc = 0; _camera.flying = false; }
    else { _camera.climb = L; _camera.roc = roc; _camera.flying = true; }

    return false;
}

bool Entity::move(const Gyrovector<Real> & v, Real dt)
{ return moveHorizontally(v, dt) | moveVertically(dt); }

void Entity::teleport(const Position & P, const Real climb) {
    _camera.climb = climb;
    _camera.position = P;
    _chunk = _atlas->lookup(P.center());
}