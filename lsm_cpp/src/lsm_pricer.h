#ifndef LSM_PRICER_H
#define LSM_PRICER_H
#include <vector>
#include "arena_allocator.h"

template <typename VecType> std::vector<double> polyfit(const VecType& x, const VecType& y);
double price_american_put_lsm_cpp(double, double, double, double, double, int, int, int);
double price_american_put_lsm_arena(Arena&, double, double, double, double, double, int, int, int);
double price_american_put_lsm_simd(double, double, double, double, double, int, int, int);
double price_american_put_lsm_ultimate(Arena&, double, double, double, double, double, int, int, int);
double price_american_put_lsm_mp(Arena&, double, double, double, double, double, int, int, int); // <-- ADD THIS

#endif // LSM_PRICER_H