#pragma once

#include "particle.h"

#include <array>
#include <memory>

struct QuadTree
{
    double theta = 0.5;

    std::array<double, 2> ll {-1.0, -1.0};
    std::array<double, 2> ur {1.0, 1.0};

    Particle *particle {nullptr};

    std::unique_ptr<QuadTree> ne {nullptr};
    std::unique_ptr<QuadTree> nw {nullptr};
    std::unique_ptr<QuadTree> sw {nullptr};
    std::unique_ptr<QuadTree> se {nullptr};

    std::array<double, 2> center {0.0, 0.0};
    double m {0.0};

    auto _get_quadrant(Particle &e) -> std::unique_ptr<QuadTree>&
    {
        auto dxh = 0.5 * (ur[0] + ll[0]);
        auto dyh = 0.5 * (ur[1] + ll[1]);
        if (e.x > dxh && e.y >= dyh)
        {
            if (!ne)
            {
                ne.reset(new QuadTree {theta, {dxh, dyh}, ur});
            }
            return ne;
        }
        else if (e.x <= dxh && e.y > dyh)
        {
            if (!nw)
            {
                nw.reset(new QuadTree {theta, {ll[0], dyh}, {dxh, ur[1]}});
            }
            return nw;
        }
        else if (e.x < dxh && e.y <= dyh)
        {
            if (!sw)
            {
                sw.reset(new QuadTree {theta, ll, {dxh, dyh}});
            }
            return sw;
        }
        else
        {
            if (!se)
            {
                se.reset(new QuadTree {theta, {dxh, ll[1]}, {ur[0], dyh}});
            }
            return se;
        }
    }

    auto _subdivide(Particle &e) -> void
    {
        auto &existing_particle_quadrant = _get_quadrant(*particle);
        auto _particle = particle;
        particle = nullptr;
        existing_particle_quadrant->add(*_particle);

        auto &new_particle_quadrant = _get_quadrant(e);
        new_particle_quadrant->add(e);
    }

    auto add(Particle &e) -> void
    {
        if (ne || nw || sw || se)
        {
            auto &particle_quadrant = _get_quadrant(e);
            particle_quadrant->add(e);
        }
        else if (particle)
        {
            _subdivide(e);
        }
        else
        {
            particle = &e;
        }
    }

    auto get_cogs() -> void
    {
        if (particle)
        {
            m = particle->m;
            center = {particle->x, particle->y};
        }
        else
        {
            m = 0.0;
            center = {0.0, 0.0};
            if (ne)
            {
                ne->get_cogs();
                center[0] += ne->center[0] * ne->m;
                center[1] += ne->center[1] * ne->m;
                m += ne->m;
            }
            if (nw)
            {
                nw->get_cogs();
                center[0] += nw->center[0] * nw->m;
                center[1] += nw->center[1] * nw->m;
                m += nw->m;
            }
            if (sw)
            {
                sw->get_cogs();
                center[0] += sw->center[0] * sw->m;
                center[1] += sw->center[1] * sw->m;
                m += sw->m;
            }
            if (se)
            {
                se->get_cogs();
                center[0] += se->center[0] * se->m;
                center[1] += se->center[1] * se->m;
                m += se->m;
            }
            center[0] /= m;
            center[1] /= m;
        }
    }

   auto force(Particle &e) -> void
   {
        if (particle)
        {
            if (particle != &e)
            {
               e.force(*particle);
            }
        }
        else
        {
            auto dx = center[0] - e.x;
            auto dy = center[1] - e.y;
            auto d = std::sqrt(dx * dx + dy * dy);

            if ((ur[0] - ll[0]) / d < theta)
            {
                e.force(dx, dy, m);
            }
            else
            {
                if (ne)
                {
                    ne->force(e);
                }
                if (nw)
                {
                    nw->force(e);
                }
                if (sw)
                {
                    sw->force(e);
                }
                if (se)
                {
                    se->force(e);
                }
            }
        }
    }

    auto get_extents(std::vector<std::array<double, 4>> &extents) -> void
    {
        if (particle)
        {
            extents.push_back({ll[0], ll[1], ur[0], ur[1]});
        }
        if (ne)
        {
            ne->get_extents(extents);
        }
        if (nw)
        {
            nw->get_extents(extents);
        }
        if (sw)
        {
            sw->get_extents(extents);
        }
        if (se)
        {
            se->get_extents(extents);
        }
    }
};
