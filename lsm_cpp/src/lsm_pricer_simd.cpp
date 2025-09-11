// lsm_cpp/src/lsm_pricer_simd.cpp

#include "lsm_pricer.h"
#include <vector>
#include <cmath>
#include <random>
#include <stdexcept>
#include "xsimd/xsimd.hpp"

namespace xs = xsimd;

double price_american_put_lsm_simd(
    double S0, double K, double T, double r, double sigma,
    int num_paths, int num_steps, int seed) {

    using batch_type = xs::batch<double>;
    const std::size_t batch_size = batch_type::size;
    if (num_paths % batch_size != 0) {
        throw std::runtime_error("Number of paths must be a multiple of SIMD batch size.");
    }

    double dt = T / static_cast<double>(num_steps);
    std::mt19937_64 rng(seed);
    std::normal_distribution<double> dist(0.0, 1.0);

    // --- 1. Path Simulation (Correct SoA Layout) ---
    // S_time_steps[t] holds a vector of all path prices at time t.
    // This layout ensures that memory access at each time step is contiguous.
    std::vector<std::vector<double>> S_time_steps(num_steps + 1, std::vector<double>(num_paths));
    
    // Initialize all paths at S0
    std::fill(S_time_steps[0].begin(), S_time_steps[0].end(), S0);

    batch_type r_dt_batch = (r - 0.5 * sigma * sigma) * dt;
    batch_type sigma_sqrt_dt_batch = sigma * std::sqrt(dt);
    std::vector<double> Z_buffer(num_paths);

    for (int j = 1; j <= num_steps; ++j) {
        // Generate all random numbers for the current time step
        for(int i = 0; i < num_paths; ++i) {
            Z_buffer[i] = dist(rng);
        }

        // Previous and current stock prices are now simple pointers to the start of the vectors.
        double* s_prev_ptr = S_time_steps[j - 1].data();
        double* s_curr_ptr = S_time_steps[j].data();

        for (int i = 0; i < num_paths; i += batch_size) {
            // Load a contiguous batch of prices from the previous time step.
            batch_type s_prev_batch = xs::load_unaligned(s_prev_ptr + i);
            // Load a contiguous batch of random numbers.
            batch_type z_batch = xs::load_unaligned(Z_buffer.data() + i);

            // Perform the calculation in SIMD registers.
            batch_type s_curr_batch = s_prev_batch * xs::exp(r_dt_batch + sigma_sqrt_dt_batch * z_batch);
            
            // Store the contiguous batch of results into the current time step's vector.
            xs::store_unaligned(s_curr_ptr + i, s_curr_batch);
        }
    }

    // --- Backward Induction (Scalar, using the SoA data) ---
    std::vector<std::vector<double>> cash_flows(num_steps + 1, std::vector<double>(num_paths, 0.0));

    for (int i = 0; i < num_paths; ++i) {
        cash_flows[num_steps][i] = std::max(0.0, K - S_time_steps[num_steps][i]);
    }
    
    for (int t = num_steps - 1; t > 0; --t) {
        std::vector<int> itm_paths_indices;
        std::vector<double> x_itm, y_itm;

        for (int i = 0; i < num_paths; ++i) {
            if (K - S_time_steps[t][i] > 0.0) {
                itm_paths_indices.push_back(i);
                x_itm.push_back(S_time_steps[t][i]);
                
                double future_cf = 0.0;
                for (int j = t + 1; j <= num_steps; ++j) {
                    if (cash_flows[j][i] > 0.0) {
                        future_cf = cash_flows[j][i] * std::exp(-r * (j - t) * dt);
                        break;
                    }
                }
                y_itm.push_back(future_cf);
            }
        }

        if (itm_paths_indices.empty()) continue;

        std::vector<double> coeffs = polyfit(x_itm, y_itm);
        
        for (size_t i = 0; i < itm_paths_indices.size(); ++i) {
            int path_idx = itm_paths_indices[i];
            double x_val = x_itm[i];
            double continuation_value = coeffs[0] * x_val * x_val + coeffs[1] * x_val + coeffs[2];
            double intrinsic_value = std::max(0.0, K - S_time_steps[t][path_idx]);

            if (intrinsic_value > continuation_value) {
                cash_flows[t][path_idx] = intrinsic_value;
                for (int j = t + 1; j <= num_steps; ++j) {
                    cash_flows[j][path_idx] = 0.0;
                }
            }
        }
    }

    // --- Pricing (Scalar) ---
    double total_payoff = 0.0;
    for (int i = 0; i < num_paths; ++i) {
        for (int j = 1; j <= num_steps; ++j) {
            if (cash_flows[j][i] > 0.0) {
                total_payoff += cash_flows[j][i] * std::exp(-r * j * dt);
                break;
            }
        }
    }

    return total_payoff / num_paths;
}