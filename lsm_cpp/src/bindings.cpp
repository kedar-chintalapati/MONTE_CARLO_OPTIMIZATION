// lsm_cpp/src/bindings.cpp

#include <pybind11/pybind11.h>

namespace py = pybind11;

// Forward declaration of the function we want to expose
double price_american_put_lsm_cpp(
    double S0, double K, double T, double r, double sigma,
    int num_paths, int num_steps, int seed);

// This is the macro that creates the Python module
PYBIND11_MODULE(lsm_cpp_backend, m) {
    m.doc() = "C++ backend for LSM American option pricing"; // optional module docstring
    
    m.def("price_american_put_lsm_cpp", &price_american_put_lsm_cpp, "A C++ LSM pricer",
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