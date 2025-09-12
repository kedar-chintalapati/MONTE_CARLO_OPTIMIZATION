// lsm_cpp/src/lsm_pricer_arena.cpp

#include "lsm_pricer.h"
#include "arena_allocator.h"
#include <numeric>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>

double price_american_put_lsm_arena(
    Arena& arena,
    double S0, double K, double T, double r, double sigma,
    int num_paths, int num_steps, int seed) {
    
    arena.reset();
    ArenaAllocator<double> alloc(arena);
    ArenaAllocator<int> int_alloc(arena);
    
    double dt = T / static_cast<double>(num_steps);

    std::mt19937_64 rng(seed);
    std::normal_distribution<double> dist(0.0, 1.0);

    // --- Pre-allocate ALL memory from the arena upfront ---
    std::vector<double, ArenaAllocator<double>> S_flat(num_paths * (num_steps + 1), alloc);
    std::vector<double, ArenaAllocator<double>> cash_flows_flat(num_paths * (num_steps + 1), alloc);
    
    // Pre-allocate temporary vectors to their maximum possible size
    std::vector<int, ArenaAllocator<int>> in_the_money_paths(num_paths, int_alloc);
    std::vector<double, ArenaAllocator<double>> x_itm(num_paths, alloc);
    std::vector<double, ArenaAllocator<double>> y_itm(num_paths, alloc);

    auto S = [&](int path, int time) -> double& { return S_flat[path * (num_steps + 1) + time]; };
    auto CF = [&](int path, int time) -> double& { return cash_flows_flat[path * (num_steps + 1) + time]; };
    
    for (int i = 0; i < num_paths; ++i) {
        S(i, 0) = S0;
        for (int j = 1; j <= num_steps; ++j) {
            S(i, j) = S(i, j - 1) * std::exp((r - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt) * dist(rng));
        }
    }

    for (int i = 0; i < num_paths; ++i) {
        CF(i, num_steps) = std::max(0.0, K - S(i, num_steps));
    }

    for (int t = num_steps - 1; t > 0; --t) {
        // Use counters to manage the "size" of our pre-allocated vectors, avoiding re-allocation.
        size_t itm_count = 0;

        for (int i = 0; i < num_paths; ++i) {
            if (K - S(i, t) > 0.0) {
                in_the_money_paths[itm_count] = i;
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

        // Create temporary "views" into our pre-allocated vectors for polyfit
        // This avoids copying and uses the existing arena memory.
        std::vector<double> x_itm_view(x_itm.begin(), x_itm.begin() + itm_count);
        std::vector<double> y_itm_view(y_itm.begin(), y_itm.begin() + itm_count);
        std::vector<double> coeffs = polyfit(x_itm_view, y_itm_view); 
        
        for (size_t i = 0; i < itm_count; ++i) {
            int path_idx = in_the_money_paths[i];
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

    double total_payoff = 0.0;
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