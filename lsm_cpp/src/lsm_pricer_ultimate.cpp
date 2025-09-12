// lsm_cpp/src/lsm_pricer_ultimate.cpp

#include "lsm_pricer.h"
#include <vector>
#include <cmath>
#include <random>
#include <stdexcept>
#include <omp.h>
#include "xsimd/xsimd.hpp"

namespace xs = xsimd;

double price_american_put_lsm_ultimate(
    Arena& arena,
    double S0, double K, double T, double r, double sigma,
    int num_paths, int num_steps, int seed) {

    arena.reset();
    ArenaAllocator<double> alloc(arena);
    ArenaAllocator<int> int_alloc(arena);

    using batch_type = xs::batch<double>;
    const std::size_t batch_size = batch_type::size;
    if (num_paths % batch_size != 0) {
        throw std::runtime_error("Number of paths must be a multiple of SIMD batch size.");
    }

    double dt = T / static_cast<double>(num_steps);
    
    int max_threads = omp_get_max_threads();
    std::vector<std::mt19937_64> generators;
    for(int i = 0; i < max_threads; ++i) {
        generators.emplace_back(seed + i);
    }
    std::vector<std::normal_distribution<double>> distributions(max_threads, std::normal_distribution<double>(0.0, 1.0));

    // --- All allocations are from the Arena ---
    std::vector<std::vector<double, ArenaAllocator<double>>> S_time_steps(num_steps + 1, std::vector<double, ArenaAllocator<double>>(num_paths, alloc));
    std::fill(S_time_steps[0].begin(), S_time_steps[0].end(), S0);

    batch_type r_dt_batch = (r - 0.5 * sigma * sigma) * dt;
    batch_type sigma_sqrt_dt_batch = sigma * std::sqrt(dt);

    for (int j = 1; j <= num_steps; ++j) {
        double* s_prev_ptr = S_time_steps[j - 1].data();
        double* s_curr_ptr = S_time_steps[j].data();

        #pragma omp parallel for
        for (int i = 0; i < num_paths; i += batch_size) {
            int thread_id = omp_get_thread_num();
            
            double z_buffer[batch_size];
            for(size_t k = 0; k < batch_size; ++k) {
                z_buffer[k] = distributions[thread_id](generators[thread_id]);
            }

            batch_type s_prev_batch = xs::load_unaligned(s_prev_ptr + i);
            batch_type z_batch = xs::load_unaligned(z_buffer);
            batch_type s_curr_batch = s_prev_batch * xs::exp(r_dt_batch + sigma_sqrt_dt_batch * z_batch);
            xs::store_unaligned(s_curr_ptr + i, s_curr_batch);
        }
    }

    std::vector<std::vector<double, ArenaAllocator<double>>> cash_flows(num_steps + 1, std::vector<double, ArenaAllocator<double>>(num_paths, alloc));
    for (int i = 0; i < num_paths; ++i) {
        cash_flows[num_steps][i] = std::max(0.0, K - S_time_steps[num_steps][i]);
    }
    
    std::vector<int, ArenaAllocator<int>> itm_paths_indices(num_paths, int_alloc);
    std::vector<double, ArenaAllocator<double>> x_itm(num_paths, alloc);
    std::vector<double, ArenaAllocator<double>> y_itm(num_paths, alloc);

    for (int t = num_steps - 1; t > 0; --t) {
        size_t itm_count = 0;
        for (int i = 0; i < num_paths; ++i) {
            if (K - S_time_steps[t][i] > 0.0) {
                itm_paths_indices[itm_count] = i;
                x_itm[itm_count] = S_time_steps[t][i];
                double future_cf = 0.0;
                for (int j = t + 1; j <= num_steps; ++j) {
                    if (cash_flows[j][i] > 0.0) {
                        future_cf = cash_flows[j][i] * std::exp(-r * (j - t) * dt);
                        break;
                    }
                }
                y_itm[itm_count] = future_cf;
                itm_count++;
            }
        }

        if (itm_count == 0) continue;
        
        std::vector<double> x_itm_view(x_itm.begin(), x_itm.begin() + itm_count);
        std::vector<double> y_itm_view(y_itm.begin(), y_itm.begin() + itm_count);
        std::vector<double> coeffs = polyfit(x_itm_view, y_itm_view);
        
        for (size_t i = 0; i < itm_count; ++i) {
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

    double total_payoff = 0.0;
    #pragma omp parallel for reduction(+:total_payoff)
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