#include <Hyper/Physics.hxx>

std::pair<Position, bool> Position::move(const Gyrovector<Real> & v) const {
    auto P = _domain * Aut𝔻<Real>(v); P.normalize();

    auto w = P.origin();

    if (Chunk::isInsideOfDomain(w))
        return std::pair(Position(P, _action, _center), false);

    if (auto k = Chunk::matchNeighbour(w)) {
        const auto & Δ   = Tesselation::neighbours[*k];
        const auto & Δ⁻¹ = Tesselation::neighbours⁻¹[*k];

        return std::pair(Position(Δ⁻¹ * P, _action * Δ), true);
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
    if (C == nullptr || !C->ready()) return false;

    for (Level L = std::floor(y); L <= std::floor(y + height); L++)
        if (!Chunk::outside(L) && !C->walkable(x, L, z))
            return true;

    return false;
}

bool Entity::stuck() { return stuck(_chunk, _i, _camera.climb, _j); }

bool Entity::moveHorizontally(const Gyrovector<Real> & v, const Real dt) {
    auto [P, chunkChanged] = _camera.position.move(v.scale(dt));
    auto C = chunkChanged ? atlas()->poll(_camera.position.action(), P.action()) : chunk();

    if (C != nullptr) {
        if (!C->ready()) return false;

        auto [i, j] = P.round(C);
        if (stuck(C, i, _camera.climb, j)) return false;
        _i = i; _j = j;
    }

    _chunk = C; _camera.position = P; return chunkChanged;
}

bool Entity::moveVertically(const Real dt) {
    if (!_chunk->ready()) return false;

    /*
        Lorentz factor: γ(v) = 1/√(1 − v²/c²).
        Kinetic energy: T = γ(v)mc².
        Potential energy for the (newtonian) uniform gravitational field: U = mgh.

        Assume that energy is locally conserved:
          T₁ + U₁ = T₂ + U₂
        ↔ γ(v₁)mc² + mgh₁ = γ(v₂)mc² + mgh₂
        ↔ [γ(v₁) − γ(v₂)]mc² = mg(h₂ − h₁)
        ↔ g(h₂ − h₁)/c² = γ(v₁) − γ(v₂).

        Let v₁ = v, v₂ = v + dt, h₂ − h₁ = dh = vdt.

        Then:
          −gvdt/c² = γ(v + dv) − γ(v) = γ′(v)dv
                   = (v/c²) × (1 − v²/c²)^(−3/2) × dv
        ↔ −gdt = (1 − v²/c²)^(−3/2) × dv
        ↔ dv/dt = −g(1 − v²/c²)^(3/2)

        In particular, (1 − v²/c²)^(3/2) = 1 − 3v²/2c² + o(x⁴),
        so if v/c ≈ 0, then dv/dt ≈ −g.
    */
    constexpr Real vmax = 32.0;

    auto γ⁻² = std::clamp<Real>(1 - Math::sqr(_camera.roc / vmax), 0, 1);
    auto roc = noclip ? _camera.roc : _camera.roc - dt * gravity * std::pow(γ⁻², 1.5);

    if (jumped) { roc += jumpSpeed; jumped = false; }

    auto L = _camera.climb + dt * roc;

    if (stuck(_chunk, _i, L, _j)) { _camera.roc = 0; _camera.flying = false; }
    else { _camera.climb = L; _camera.roc = roc; _camera.flying = true; }

    return false;
}

bool Entity::move(const Gyrovector<Real> & v, Real dt)
{ return moveHorizontally(v, dt) | moveVertically(dt); }

void Entity::teleport(const Position & P, const Real climb) {
    _camera.climb = climb; _camera.position = P;
    _chunk = _atlas->poll(P.action(), P.action());
}