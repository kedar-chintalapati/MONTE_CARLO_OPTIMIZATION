// lsm_cpp/src/bindings.cpp

#include <pybind11/pybind11.h>
#include <stdexcept> // Required for catching C++ exceptions
#include "lsm_pricer.h"

namespace py = pybind11;

// --- Forward Declarations ---
// We tell the compiler that these functions exist in other files.

// 1. The original scalar function from lsm_pricer.cpp
//double price_american_put_lsm_cpp(
    //double S0, double K, double T, double r, double sigma,
    //int num_paths, int num_steps, int seed);

// 2. The new SIMD function from lsm_pricer_simd.cpp
//double price_american_put_lsm_simd(
    //double S0, double K, double T, double r, double sigma,
    //int num_paths, int num_steps, int seed);


// --- Python Module Definition ---
// This is the macro that creates the Python module.
// The first argument, lsm_cpp_backend, MUST match the target name in CMakeLists.txt.
PYBIND11_MODULE(lsm_cpp_backend, m) {
    m.doc() = "C++ backend for LSM American option pricing"; // Optional module docstring
    
    // --- Binding for the SCALAR function ---
    // This is a direct mapping of the C++ function to a Python function.
    m.def("price_american_put_lsm_cpp", &price_american_put_lsm_cpp, "A C++ scalar LSM pricer",
          py::arg("S0"),
          py::arg("K"),
          py::arg("T"),
          py::arg("r"),
          py::arg("sigma"),
          py::arg("num_paths"),
          py::arg("num_steps"),
          py::arg("seed") = 42
    );

    // --- Binding for the SIMD function ---
    // We use a C++ lambda function here to safely handle potential C++ exceptions
    // and convert them into Python exceptions.
    m.def("price_american_put_lsm_simd", 
        [](double S0, double K, double T, double r, double sigma, int num_paths, int num_steps, int seed) {
            try {
                return price_american_put_lsm_simd(S0, K, T, r, sigma, num_paths, num_steps, seed);
            } catch (const std::runtime_error& e) {
                // This converts the C++ std::runtime_error into a Python ValueError,
                // which is much cleaner and prevents the interpreter from crashing.
                throw py::value_error(e.what());
            }
        }, 
        "A C++ SIMD-accelerated LSM pricer",
          py::arg("S0"),
          py::arg("K"),
          py::arg("T"),
          py::arg("r"),
          py::arg("sigma"),
          py::arg("num_paths"),
          py::arg("num_steps"),
          py::arg("seed") = 42
    );
}