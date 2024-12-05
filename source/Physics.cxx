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

vec3 Object::direction() const {
    return vec3(
        cos(pitch) * sin(yaw),
        sin(pitch),
        cos(pitch) * cos(yaw)
    );
}

vec3 Object::right() const {
    return vec3(
        cos(roll) * sin(yaw - τ/4),
        sin(roll),
        cos(roll) * cos(yaw - τ/4)
    );
}

bool Entity::stuck(Chunk * C, Rank x, Real y, Rank z) {
    if (flymode && noclip) return false;

    if (C == nullptr || !C->ready()) return false;

    auto y₁ = std::floor(y), y₂ = std::floor(y + height);

    for (int L = y₁; L <= y₂; L++)
        if (!C->walkable(x, L, z))
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
        Relativistic kinetic energy: T = γ(v)mc².
        Potential energy for the (newtonian) uniform gravitational field: U = mgh.

        Then:
          dT = γ′(v)mc²dv,
          δU = mgdh = mgvdt,
          δE = dT + δU,
        where
          γ′(v) = (v/c²) × (1 − v²/c²)^(−3/2).

        Assume that energy is locally conserved:
          δE = 0
        ↔ dT = −δU
        ↔ γ′(v)mc²dv = −mgvdt
        ↔ γ′(v)dv = −(v/c²)gdt
        ↔ dv = −g(1 − v²/c²)^(3/2) × dt.

        In particular, (1 − v²/c²)^(3/2) = 1 − 3v²/2c² + o(v⁴/c⁴),
        so if v/c ≈ 0, then dv/dt ≈ −g.

        For the non-trivial topology δE may be not exact, so that it
        makes no sense to define potential energy globally.
        Therefore energy will not be conserved.

        In particular, “dh” is known to be a generator
        of de Rham cohomology group H¹(S¹) ≅ ℝ.
    */
    constexpr Real vmax = 32.0;

    auto γ⁻² = std::clamp<Real>(1 - Math::sqr(_camera.roc / vmax), 0, 1);
    auto roc = flymode ? _camera.roc : _camera.roc - dt * gravity * std::pow(γ⁻², 1.5);

    if (jumped) { roc += jumpSpeed; jumped = false; }

    auto L = Chunk::clamp(_camera.climb + dt * roc);

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