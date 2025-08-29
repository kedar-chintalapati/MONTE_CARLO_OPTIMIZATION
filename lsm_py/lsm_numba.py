# lsm_py/lsm_numba.py

import numpy as np
from numba import njit

@njit
def polyval_numba(p, x):
    """
    A Numba-compatible implementation of np.polyval.
    Evaluates a polynomial at points x.
    p is an array of coefficients, from highest degree to lowest.
    """
    # Get the degree of the polynomial from the length of the coefficient array
    deg = len(p) - 1
    # Initialize the result array with zeros
    y = np.zeros_like(x)
    # Iterate through the coefficients and powers of x
    for i in range(deg + 1):
        y += p[i] * x**(deg - i)
    return y

@njit
def polyfit_numba(x, y, deg):
    """
    A Numba-compatible implementation of a simple polynomial fit (least-squares).
    This replaces np.polyfit, which is not supported in nopython mode.
    """
    A = np.zeros((len(x), deg + 1))
    for i in range(deg + 1):
        A[:, i] = x**(deg - i)
        
    ATA = A.T @ A
    ATy = A.T @ y
    
    coeffs = np.linalg.solve(ATA, ATy)
    
    return coeffs

@njit
def price_american_put_lsm_numba(
    S0: float,
    K: float,
    T: float,
    r: float,
    sigma: float,
    num_paths: int,
    num_steps: int,
    seed: int = 42,
) -> float:
    """
    Prices an American put option using the Longstaff-Schwartz Monte Carlo (LSM) algorithm.
    This version is accelerated with Numba's @njit decorator.
    """
    dt = T / num_steps
    
    np.random.seed(seed)
    Z = np.random.standard_normal((num_paths, num_steps))
    
    S = np.zeros((num_paths, num_steps + 1))
    S[:, 0] = S0
    
    for t in range(1, num_steps + 1):
        S[:, t] = S[:, t - 1] * np.exp((r - 0.5 * sigma**2) * dt + sigma * np.sqrt(dt) * Z[:, t - 1])

    intrinsic_value = np.maximum(0.0, K - S)
    cash_flows = np.zeros((num_paths, num_steps + 1))
    cash_flows[:, -1] = intrinsic_value[:, -1]
    
    for t in range(num_steps - 1, 0, -1):
        in_the_money_paths = np.where(intrinsic_value[:, t] > 0)[0]
        
        if len(in_the_money_paths) == 0:
            continue
            
        X = S[in_the_money_paths, t]
        
        future_discounted_cash_flows = np.zeros(len(in_the_money_paths))
        for i in range(len(in_the_money_paths)):
            path_idx = in_the_money_paths[i]
            for j in range(t + 1, num_steps + 1):
                if cash_flows[path_idx, j] > 0:
                    future_discounted_cash_flows[i] = cash_flows[path_idx, j] * np.exp(-r * (j - t) * dt)
                    break
        Y = future_discounted_cash_flows
        
        coeffs = polyfit_numba(X, Y, 2)
        
        # --- THE FIX IS HERE ---
        # Replace the unsupported np.polyval with our new Numba-compatible version
        continuation_value = polyval_numba(coeffs, X)
        
        exercise_paths = in_the_money_paths[intrinsic_value[in_the_money_paths, t] > continuation_value]
        
        if len(exercise_paths) > 0:
            cash_flows[exercise_paths, t] = intrinsic_value[exercise_paths, t]
            cash_flows[exercise_paths, t + 1 :] = 0.0

    discount_factors = np.exp(-r * np.arange(num_steps + 1) * dt)
    discounted_cash_flows = np.sum(cash_flows * discount_factors, axis=1)
    price = np.mean(discounted_cash_flows)
    
    return price