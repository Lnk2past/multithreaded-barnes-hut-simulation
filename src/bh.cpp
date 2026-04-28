#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include "particle_system.h"
#include "syncable.h"

#include <print>

struct MultithreadedParticleSystem : ParticleSystem {
    MultithreadedParticleSystem(
        const int num_particles,
        const double bounds,
        const double theta,
        const uint64_t seed,
        const double dt,
        const std::size_t num_threads
    ):
        ParticleSystem(num_particles, bounds, theta, seed),
        delta_time(dt),
        pool(num_threads)
    {
        auto slice_size = (num_particles + num_threads - 1) / num_threads;
        for (auto i = size_t{}; i < num_threads; ++i)
        {
            pool.threads.emplace_back([this, start=i * slice_size, count=slice_size](std::stop_token st)
            {
                while (!st.stop_requested())
                {
                    pool.sync_point_1.arrive_and_wait();
                    if (st.stop_requested())
                    {
                        break;
                    }
                    this->collect_forces(start, count);
                    pool.sync_point_2.arrive_and_wait();
                }
            });
        }
    }

    auto update() -> void
    {
        build_tree();
        pool.trigger();
        integrate(delta_time);
        simulation_time += delta_time;
    }

    auto request_stop() -> void
    {
        pool.request_stop();
    }

    std::vector<std::function<void(void)>> callables{};
    double simulation_time = 0.0;
    double delta_time = 1.0;
    Syncable pool;
};

PYBIND11_MODULE(PyModel, m) {
    py::class_<MultithreadedParticleSystem>(m, "MultithreadedParticleSystem")
        .def(py::init<const int, const double, const double, const uint64_t, const double, const std::size_t>())
        .def("update", &MultithreadedParticleSystem::update)
        .def("request_stop", &MultithreadedParticleSystem::request_stop)
        .def("get_extents", &MultithreadedParticleSystem::get_extents)
        .def_readwrite("ll", &MultithreadedParticleSystem::ll)
        .def_readwrite("ur", &MultithreadedParticleSystem::ur)
        .def_readwrite("simulation_time", &MultithreadedParticleSystem::simulation_time)
        .def_readwrite("particles", &MultithreadedParticleSystem::particles);

    py::class_<Particle>(m, "Particle")
        .def_readwrite("x", &Particle::x)
        .def_readwrite("y", &Particle::y)
        .def_readwrite("vx", &Particle::vx)
        .def_readwrite("vy", &Particle::vy)
        .def_readwrite("m", &Particle::m);
}

