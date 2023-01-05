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
    Position(const auto & P, const auto & G) : _domain(P) { action(G); }

    inline constexpr auto domain() const { return _domain; }
    inline constexpr auto action() const { return _action; }
    inline constexpr auto center() const { return _center; }

    inline constexpr void action(const Fuchsian<Integer> & G)
    { _action = G; _action.simpl(); _center = G.origin(); }

    // If speed is too high to jump over ≥2 chunks, does nothing.
    // (Of course, this can be easily fixed by iterating
    //  not only over neighbours, but it seems useless.)
    std::pair<Position, bool> move(const Gyrovector<Real> &, const Real dt) const;
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

    // Needs to be replaced by more general bounding box
    bool isFree(Chunk * C, Rank x, Real L, Rank z);

    bool moveHorizontally(const Gyrovector<Real> & v, const Real dt);
    void moveVertically(const Real dt);

public:
    Real jumpSpeed = 0;
    Real height    = 1.8;
    Real gravity   = 9.8;

    Entity(Atlas * atlas) : _i(0), _j(0), _atlas(atlas), _chunk(nullptr) {}

    // Returns true iff chunk changes
    bool move(const Gyrovector<Real> & v, Real dt);
    void teleport(const Position &, const Real);

    const inline auto camera() const { return _camera; }
    constexpr void roc(const Real roc) { _camera.roc = roc; }
    constexpr void push(const Real speed) { _camera.roc += speed; }

    constexpr inline auto atlas() const { return _atlas; }
    constexpr inline auto chunk() const { return _chunk; }

    constexpr inline auto i() const { return _i; }
    constexpr inline auto j() const { return _j; }

    constexpr inline void jumpHeight(Real height)
    { jumpSpeed = sqrt(2 * gravity * height); }

    inline void rotate(const Real Δyaw, const Real Δpitch)
    { _camera.rotate(Δyaw, Δpitch); }
};