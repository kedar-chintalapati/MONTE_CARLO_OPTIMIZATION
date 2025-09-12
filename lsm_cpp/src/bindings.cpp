#include "lsm_pricer.h"
#include <pybind11/pybind11.h>
#include <stdexcept>
namespace py = pybind11;

PYBIND11_MODULE(lsm_cpp_backend, m) {
    m.doc() = "C++ backend for LSM American option pricing";
    m.def("price_american_put_lsm_cpp", &price_american_put_lsm_cpp, "C++ scalar", py::arg("S0"), py::arg("K"), py::arg("T"), py::arg("r"), py::arg("sigma"), py::arg("num_paths"), py::arg("num_steps"), py::arg("seed") = 42);
    m.def("price_american_put_lsm_arena", [](double S0, double K, double T, double r, double sigma, int num_paths, int num_steps, int seed) {
        size_t s_matrix_bytes = static_cast<size_t>(num_paths) * (num_steps + 1) * sizeof(double);
        size_t cf_matrix_bytes = static_cast<size_t>(num_paths) * (num_steps + 1) * sizeof(double);
        size_t itm_paths_bytes = static_cast<size_t>(num_paths) * sizeof(int);
        size_t x_itm_bytes = static_cast<size_t>(num_paths) * sizeof(double);
        size_t y_itm_bytes = static_cast<size_t>(num_paths) * sizeof(double);
        size_t total_bytes_needed = s_matrix_bytes + cf_matrix_bytes + itm_paths_bytes + x_itm_bytes + y_itm_bytes;
        size_t memory_estimate = static_cast<size_t>(total_bytes_needed * 1.1);
        Arena arena(memory_estimate);
        return price_american_put_lsm_arena(arena, S0, K, T, r, sigma, num_paths, num_steps, seed);
    }, "C++ scalar + Arena", py::arg("S0"), py::arg("K"), py::arg("T"), py::arg("r"), py::arg("sigma"), py::arg("num_paths"), py::arg("num_steps"), py::arg("seed") = 42);
    m.def("price_american_put_lsm_simd", [](double S0, double K, double T, double r, double sigma, int num_paths, int num_steps, int seed) {
        try { return price_american_put_lsm_simd(S0, K, T, r, sigma, num_paths, num_steps, seed); } catch (const std::runtime_error& e) { throw py::value_error(e.what()); }
    }, "C++ SIMD", py::arg("S0"), py::arg("K"), py::arg("T"), py::arg("r"), py::arg("sigma"), py::arg("num_paths"), py::arg("num_steps"), py::arg("seed") = 42);
    m.def("price_american_put_lsm_ultimate", [](double S0, double K, double T, double r, double sigma, int num_paths, int num_steps, int seed) {
        // Estimate for vector of RNGs + other small overheads
        size_t overhead = 1024 * 10; 
        // Estimate for the main data structures
        size_t main_data_bytes = static_cast<size_t>(num_paths) * ( (num_steps + 1) * sizeof(double) * 2 + sizeof(int) + sizeof(double) * 2 );
        size_t memory_estimate = static_cast<size_t>(main_data_bytes * 1.1) + overhead;
        Arena arena(memory_estimate);
        try { return price_american_put_lsm_ultimate(arena, S0, K, T, r, sigma, num_paths, num_steps, seed); } catch (const std::runtime_error& e) { throw py::value_error(e.what()); }
    }, "Ultimate: C++ MP + SIMD + Arena", py::arg("S0"), py::arg("K"), py::arg("T"), py::arg("r"), py::arg("sigma"), py::arg("num_paths"), py::arg("num_steps"), py::arg("seed") = 42);
}