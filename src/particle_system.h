#pragma once

#include "particle.h"
#include "quadtree.h"

#include <array>
#include <random>
#include <vector>

struct ParticleSystem {
    std::array<double, 2> ll {-1, -1};
    std::array<double, 2> ur {1, 1};
    QuadTree qt{};
    double theta{};
    std::vector<Particle> particles{};

    ParticleSystem(const int num_particles, const double bounds, const double default_theta, const std::uint64_t seed=1337):
        ll {-bounds, -bounds},
        ur {bounds, bounds},
        theta (default_theta),
        particles(num_particles)
    {
        auto generator = [eng = std::mt19937{seed}, dis = std::uniform_real_distribution<double>{-bounds, bounds}]() mutable -> Particle
        {
            return {dis(eng), dis(eng)};
        };
        std::ranges::generate_n(particles.begin(), num_particles-1, generator);
        particles.back() = Particle{.m=1e12};
    }

    auto build_tree() -> void
    {
        qt = {.theta=theta, .ll=ll, .ur=ur};
        for (auto &e : particles)
        {
            qt.add(e);
        }
        qt.get_cogs();
    }

    auto collect_forces(std::size_t start, std::size_t count) -> void
    {
        for (auto i = start; i < start + count; ++i) {
            qt.force(particles[i]);
        }
    }

    auto integrate(const double delta_time) -> void
    {
        auto bounds = 0.0;
        for (auto &e : particles)
        {
            e.integrate(delta_time);

            if (std::abs(e.x) > bounds)
            {
                bounds = std::abs(e.x);
            }
            if (std::abs(e.y) > bounds)
            {
                bounds = std::abs(e.y);
            }
        }
        ll = {-bounds, -bounds};
        ur = {bounds, bounds};
    }

    auto get_extents() -> std::vector<std::array<double, 4>>
    {
        auto extents = std::vector<std::array<double, 4>>{};
        qt.get_extents(extents);
        return extents;
    }
};
