#pragma once

#include <glm/vec3.hpp>

#include <Hyper/Tesselation.hpp>
#include <Hyper/Geometry.hpp>
#include <Hyper/Fuchsian.hpp>
#include <Hyper/Moebius.hpp>

class Position {
private:
    Möbius<Real>       _domain;
    Fuchsian<Integer>  _action;
    Gaussian²<Integer> _center;

public:
    Position() : _domain(Möbius<Real>::identity()), _action(Tesselation::I) { _center = _action.origin(); }

    Position(const auto & P, const auto & G, const auto & g) : _domain(P), _action(G), _center(g) {}
    Position(const auto & P, const auto & G) : _domain(P) { set(G); }

    inline constexpr const auto & domain() const { return _domain; }
    inline constexpr const auto & action() const { return _action; }
    inline constexpr const auto & center() const { return _center; }

    inline constexpr void set(const Position & P)
    { _domain = P.domain(); _action = P.action(); _center = P.center(); }

    inline constexpr void set(const Möbius<Real> & M, const Fuchsian<Integer> & G)
    { _domain = M; _action = G; _center = G.origin(); }

    inline constexpr void set(const Fuchsian<Integer> & G)
    { _action = G; _action.simpl(); _center = G.origin(); }

    // It doesn’t do anything if the speed is big enough to jump over ≥2 chunks.
    // (Of course, this can be easily fixed by iterating
    //  not only over neighbours, but it seems useless.)
    std::pair<Position, bool> move(const Gyrovector<Real> &, const Real dt) const;

    std::pair<Rank, Rank> round(const Chunk *) const;
};

struct Object {
    Position position;

    Real climb = 0, roc = 0;
    bool flying = false;

    Real yaw = 0, pitch = 0;

    void rotate(const Real, const Real);

    glm::vec3 direction() const;
    glm::vec3 right() const;
};

class Entity {
private:
    Rank _i, _j; Object _camera; Atlas * _atlas; Chunk * _chunk;

    bool jumped = false;

    bool stuck(Chunk *, Rank, Real, Rank);

    bool moveHorizontally(const Gyrovector<Real> & v, const Real dt);
    bool moveVertically(const Real dt);

public:
    Real eye, height, jumpSpeed;

    Real gravity = 9.8;

    Entity(Atlas * atlas) : _i(0), _j(0), _atlas(atlas), _chunk(nullptr) {}

    // Returns true iff chunk changes
    bool move(const Gyrovector<Real> & v, Real dt);
    void teleport(const Position &, const Real);

    constexpr void roc(const Real roc) { _camera.roc = roc; }
    constexpr void jump() { jumped = true; }

    inline constexpr const auto & camera() const { return _camera; }
    inline constexpr const auto & atlas()  const { return _atlas;  }
    inline constexpr const auto & chunk()  const { return _chunk;  }

    inline constexpr const auto & i() const { return _i; }
    inline constexpr const auto & j() const { return _j; }

    constexpr inline void jumpHeight(Real height)
    { jumpSpeed = sqrt(2 * gravity * height); }

    inline void rotate(const Real Δyaw, const Real Δpitch)
    { _camera.rotate(Δyaw, Δpitch); }
};