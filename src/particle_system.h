#pragma once

#include "particle.h"
#include "quadtree.h"

#include <array>
#include <numbers>
#include <random>
#include <vector>

using namespace std::numbers;

struct ParticleSystem {
    std::array<double, 2> ll {-1, -1};
    std::array<double, 2> ur {1, 1};
    QuadTree::QuadTreeArena arena;
    QuadTree qt;
    double theta{};
    std::vector<Particle> particles{};

    ParticleSystem(const int num_particles, const double bounds, const double default_theta, const std::uint64_t seed=1337):
        ll {-bounds, -bounds},
        ur {bounds, bounds},
        arena(num_particles),
        qt(arena),
        theta(default_theta),
        particles(num_particles)
    {

        auto bounds_dis = std::uniform_real_distribution<double>{-bounds,bounds};
        auto cluster_dis = std::uniform_real_distribution<double>{30.0, 75.0};
        auto theta_dis = std::uniform_real_distribution<double>{0.0, 2.0 * pi};
        auto eng = std::mt19937{seed};

        auto num_clusters = 4;
        auto num_per_cluster = num_particles / num_clusters;
        for (auto i = 0; i < num_clusters; ++i)
        {
            auto center_x = bounds_dis(eng);
            auto center_y = bounds_dis(eng);
            auto cd = std::sqrt(center_x * center_x + center_y * center_y);
            auto cvx = -center_y/cd;
            auto cvy = center_x/cd;
            auto generator = [&]() -> Particle
            {
                auto r = cluster_dis(eng);
                auto t = theta_dis(eng);
                auto dx = std::sqrt(r) * std::cos(t);
                auto dy = std::sqrt(r) * std::sin(t);
                auto p = Particle{.x=(center_x + dx), .y=(center_y + dy)};
                auto d = std::sqrt(dx * dx + dy * dy);
                p.vx = cvx + -2.0 * dy / d;
                p.vy = cvy + 2.0 * dx / d;
                return p;
            };
            std::ranges::generate_n(particles.begin() + i * num_per_cluster, num_per_cluster, generator);
            particles[(i+1)* num_per_cluster - 1] = Particle{.x=center_x, .y=center_y, .vx=cvx, .vy=cvy, .m=1e12};
        }
    }

    auto build_tree() -> void
    {
        arena.reset();
        qt.theta = theta;
        qt.ll = ll;
        qt.ur = ur;
        qt.particle = nullptr;
        qt.ne = qt.nw = qt.sw = qt.se = nullptr;
        qt.m = 0.0;
        qt.center = {0.0, 0.0};
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
