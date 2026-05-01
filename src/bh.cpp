#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "particle_system.h"
#include "syncable.h"


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

    auto get_entities() -> py::dict
    {
        auto n = particles.size();
        auto x = py::array_t<double>(n);
        auto y = py::array_t<double>(n);
        auto m = py::array_t<double>(n);
        
        auto *px = x.mutable_data();
        auto *py_ = y.mutable_data();
        auto *pm = m.mutable_data();
        
        for (size_t i = 0; i < n; ++i) {
            px[i] = particles[i].x;
            py_[i] = particles[i].y;
            pm[i] = particles[i].m;
        }
        
        auto result = py::dict{};
        result["x"] = x;
        result["y"] = y;
        result["m"] = m;
        return result;
    }

    auto get_extents() -> py::dict
    {
        auto e = ParticleSystem::get_extents();
        auto n = e.size();
        auto x0 = py::array_t<double>(n);
        auto y0 = py::array_t<double>(n);
        auto x1 = py::array_t<double>(n);
        auto y1 = py::array_t<double>(n);

        auto *px0 = x0.mutable_data();
        auto *py0 = y0.mutable_data();
        auto *px1 = x1.mutable_data();
        auto *py1 = y1.mutable_data();

        for (size_t i = 0; i < n; ++i) {
            px0[i] = e[i][0];
            py0[i] = e[i][1];
            px1[i] = e[i][2];
            py1[i] = e[i][3];

        }
        
        auto result = py::dict{};
        result["x0"] = x0;
        result["y0"] = y0;
        result["x1"] = x1;
        result["y1"] = y1;
        return result;
    }

    auto request_stop() -> void
    {
        pool.request_stop();
    }

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
        .def("get_entities", &MultithreadedParticleSystem::get_entities)
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

