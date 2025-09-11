// lsm_cpp/src/lsm_pricer.cpp

#include <vector>
#include <cmath>
#include <numeric>
#include <random>
#include <algorithm>
#include <iostream>
#include "lsm_pricer.h"

// Simple least-squares polynomial fit (degree 2)
std::vector<double> polyfit(const std::vector<double>& x, const std::vector<double>& y) {
    size_t n = x.size();
    double S_x = 0, S_y = 0, S_xx = 0, S_xy = 0, S_xxx = 0, S_xxy = 0, S_xxxx = 0;

    for (int i = 0; i < n; ++i) {
        double xi = x[i];
        double yi = y[i];
        double xi_2 = xi * xi;
        S_x += xi;
        S_y += yi;
        S_xx += xi_2;
        S_xy += xi * yi;
        S_xxx += xi * xi_2;
        S_xxy += xi_2 * yi;
        S_xxxx += xi_2 * xi_2;
    }

    double A[3][3] = {
        {static_cast<double>(n), S_x, S_xx},
        {S_x, S_xx, S_xxx},
        {S_xx, S_xxx, S_xxxx}
    };

    double b[3] = {S_y, S_xy, S_xxy};
    
    // Simple Gaussian elimination for 3x3 system
    for (int i = 0; i < 3; ++i) {
        int pivot = i;
        for (int j = i + 1; j < 3; ++j) {
            if (std::abs(A[j][i]) > std::abs(A[pivot][i])) {
                pivot = j;
            }
        }
        std::swap(A[i], A[pivot]);
        std::swap(b[i], b[pivot]);
        for (int j = i + 1; j < 3; ++j) {
            double factor = A[j][i] / A[i][i];
            for (int k = i; k < 3; ++k) {
                A[j][k] -= factor * A[i][k];
            }
            b[j] -= factor * b[i];
        }
    }

    std::vector<double> coeffs(3);
    for (int i = 2; i >= 0; --i) {
        double sum = 0;
        for (int j = i + 1; j < 3; ++j) {
            sum += A[i][j] * coeffs[j];
        }
        coeffs[i] = (b[i] - sum) / A[i][i];
    }
    // Return in numpy order (highest power first)
    return {coeffs[2], coeffs[1], coeffs[0]};
}


double price_american_put_lsm_cpp(
    double S0, double K, double T, double r, double sigma,
    int num_paths, int num_steps, int seed) {
    
    double dt = T / static_cast<double>(num_steps);

    std::mt19937_64 rng(seed);
    std::normal_distribution<double> dist(0.0, 1.0);

    std::vector<std::vector<double>> S(num_paths, std::vector<double>(num_steps + 1));
    for (int i = 0; i < num_paths; ++i) {
        S[i][0] = S0;
        for (int j = 1; j <= num_steps; ++j) {
            double Z = dist(rng);
            S[i][j] = S[i][j - 1] * std::exp((r - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt) * Z);
        }
    }

    std::vector<std::vector<double>> cash_flows(num_paths, std::vector<double>(num_steps + 1, 0.0));
    for (int i = 0; i < num_paths; ++i) {
        cash_flows[i][num_steps] = std::max(0.0, K - S[i][num_steps]);
    }

    for (int t = num_steps - 1; t > 0; --t) {
        std::vector<int> in_the_money_paths;
        std::vector<double> x_itm, y_itm;

        for (int i = 0; i < num_paths; ++i) {
            if (K - S[i][t] > 0.0) {
                in_the_money_paths.push_back(i);
                x_itm.push_back(S[i][t]);
                
                double future_cf = 0.0;
                for (int j = t + 1; j <= num_steps; ++j) {
                    if (cash_flows[i][j] > 0.0) {
                        future_cf = cash_flows[i][j] * std::exp(-r * (j - t) * dt);
                        break;
                    }
                }
                y_itm.push_back(future_cf);
            }
        }

        if (x_itm.empty()) continue;

        std::vector<double> coeffs = polyfit(x_itm, y_itm);
        
        for (size_t i = 0; i < in_the_money_paths.size(); ++i) {
            int path_idx = in_the_money_paths[i];
            double x_val = x_itm[i];
            double continuation_value = coeffs[0] * x_val * x_val + coeffs[1] * x_val + coeffs[2];
            double intrinsic_value = std::max(0.0, K - S[path_idx][t]);

            if (intrinsic_value > continuation_value) {
                cash_flows[path_idx][t] = intrinsic_value;
                for (int j = t + 1; j <= num_steps; ++j) {
                    cash_flows[path_idx][j] = 0.0;
                }
            }
        }
    }

    double total_payoff = 0.0;
    for (int i = 0; i < num_paths; ++i) {
        for (int j = 1; j <= num_steps; ++j) {
            if (cash_flows[i][j] > 0.0) {
                total_payoff += cash_flows[i][j] * std::exp(-r * j * dt);
                break;
            }
        }
    }

    return total_payoff / num_paths;
}