// lsm_cpp/src/lsm_pricer_mp.cpp

#include "lsm_pricer.h"
#include <vector>
#include <cmath>
#include <random>
#include <stdexcept>
#include <omp.h> // Include OpenMP header

// This is the multithreaded SCALAR pricer with Arena allocation
double price_american_put_lsm_mp(
    Arena& arena,
    double S0, double K, double T, double r, double sigma,
    int num_paths, int num_steps, int seed) {

    arena.reset();
    ArenaAllocator<double> alloc(arena);
    ArenaAllocator<int> int_alloc(arena);

    double dt = T / static_cast<double>(num_steps);
    
    // --- Thread-safe Random Number Generation ---
    int max_threads = omp_get_max_threads();
    std::vector<std::mt19937_64> generators;
    for(int i = 0; i < max_threads; ++i) {
        generators.emplace_back(seed + i);
    }
    std::vector<std::normal_distribution<double>> distributions(max_threads, std::normal_distribution<double>(0.0, 1.0));

    // --- Path Generation (Multithreaded SCALAR) ---
    std::vector<double, ArenaAllocator<double>> S_flat(num_paths * (num_steps + 1), alloc);
    auto S = [&](int path, int time) -> double& { return S_flat[path * (num_steps + 1) + time]; };

    // The pragma tells OpenMP to parallelize this loop across all cores.
    // Each thread will get a chunk of the 'i' loop.
    #pragma omp parallel for
    for (int i = 0; i < num_paths; ++i) {
        int thread_id = omp_get_thread_num();
        S(i, 0) = S0;
        for (int j = 1; j <= num_steps; ++j) {
            double Z = distributions[thread_id](generators[thread_id]);
            S(i, j) = S(i, j - 1) * std::exp((r - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt) * Z);
        }
    }

    // --- Backward Induction (Sequential) ---
    // (This logic is identical to cpp_arena)
    std::vector<double, ArenaAllocator<double>> cash_flows_flat(num_paths * (num_steps + 1), alloc);
    auto CF = [&](int path, int time) -> double& { return cash_flows_flat[path * (num_steps + 1) + time]; };
    
    for (int i = 0; i < num_paths; ++i) {
        CF(i, num_steps) = std::max(0.0, K - S(i, num_steps));
    }

    std::vector<int, ArenaAllocator<int>> itm_paths_indices(num_paths, int_alloc);
    std::vector<double, ArenaAllocator<double>> x_itm(num_paths, alloc);
    std::vector<double, ArenaAllocator<double>> y_itm(num_paths, alloc);

    for (int t = num_steps - 1; t > 0; --t) {
        size_t itm_count = 0;
        for (int i = 0; i < num_paths; ++i) {
            if (K - S(i, t) > 0.0) {
                itm_paths_indices[itm_count] = i;
                x_itm[itm_count] = S(i, t);
                double future_cf = 0.0;
                for (int j = t + 1; j <= num_steps; ++j) {
                    if (CF(i, j) > 0.0) {
                        future_cf = CF(i, j) * std::exp(-r * (j - t) * dt);
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
            double intrinsic_value = std::max(0.0, K - S(path_idx, t));

            if (intrinsic_value > continuation_value) {
                CF(path_idx, t) = intrinsic_value;
                for (int j = t + 1; j <= num_steps; ++j) {
                    CF(path_idx, j) = 0.0;
                }
            }
        }
    }

    // --- Pricing (Parallelized) ---
    double total_payoff = 0.0;
    #pragma omp parallel for reduction(+:total_payoff)
    for (int i = 0; i < num_paths; ++i) {
        for (int j = 1; j <= num_steps; ++j) {
            if (CF(i, j) > 0.0) {
                total_payoff += CF(i, j) * std::exp(-r * j * dt);
                break;
            }
        }
    }

    return total_payoff / num_paths;
}