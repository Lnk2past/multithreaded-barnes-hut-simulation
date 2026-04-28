#pragma once

#include <cmath>

static constexpr auto G = 6.67408e-11;

struct Particle
{
    double x = 0.0;
    double y = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double ax = 0.0;
    double ay = 0.0;
    double m = 5.0e6;

    auto force(const Particle &o) -> void
    {
        auto dx = o.x - x;
        auto dy = o.y - y;
        force(dx, dy, o.m);
    }

    auto force(const double dx, const double dy, const double omass) -> void
    {
        auto d2 = dx * dx + dy * dy;
        auto d = std::sqrt(d2);
        auto f = G * omass / d2;
        ax += f * dx / d;
        ay += f * dy / d;
    }

    auto integrate(const double dt) -> void
    {
        vx += ax * dt;
        vy += ay * dt;
        x += vx * dt;
        y += vy * dt;
        ax = 0.0;
        ay = 0.0;
    }
};