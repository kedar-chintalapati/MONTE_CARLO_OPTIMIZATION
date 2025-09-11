// lsm_cpp/src/lsm_pricer.h

#ifndef LSM_PRICER_H
#define LSM_PRICER_H

#include <vector> // We need to include <vector> here because it's used in the polyfit declaration

// --- Helper Functions ---
// Declaration for the polyfit function defined in lsm_pricer.cpp
std::vector<double> polyfit(const std::vector<double>& x, const std::vector<double>& y);


// --- Main Pricing Functions ---
// Declaration for the scalar pricer defined in lsm_pricer.cpp
double price_american_put_lsm_cpp(
    double S0, double K, double T, double r, double sigma,
    int num_paths, int num_steps, int seed);

// Declaration for the SIMD pricer defined in lsm_pricer_simd.cpp
double price_american_put_lsm_simd(
    double S0, double K, double T, double r, double sigma,
    int num_paths, int num_steps, int seed);

#endif // LSM_PRICER_H