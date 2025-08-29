# lsm_py/lsm_pricer.py

import numpy as np

def price_american_put_lsm(
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

    Args:
        S0: Initial stock price.
        K: Strike price.
        T: Time to maturity (in years).
        r: Risk-free interest rate (annualized).
        sigma: Volatility of the underlying stock (annualized).
        num_paths: The number of Monte Carlo simulation paths.
        num_steps: The number of time steps for the simulation.
        seed: Seed for the random number generator for reproducibility.

    Returns:
        The estimated price of the American put option.
    """
    # --- 1. Path Simulation ---
    dt = T / num_steps
    rng = np.random.default_rng(seed)
    
    # Generate standard normal random variables for all paths and steps at once
    Z = rng.standard_normal(size=(num_paths, num_steps))
    
    # Pre-allocate array for stock paths
    # Shape: (num_paths, num_steps + 1) to include the initial price S0
    S = np.zeros((num_paths, num_steps + 1))
    S[:, 0] = S0
    
    # Use the exact solution for Geometric Brownian Motion
    for t in range(1, num_steps + 1):
        S[:, t] = S[:, t - 1] * np.exp((r - 0.5 * sigma**2) * dt + sigma * np.sqrt(dt) * Z[:, t - 1])

    # --- 2. Backward Induction ---
    
    # Calculate the intrinsic value (payoff) at each time step
    intrinsic_value = np.maximum(0, K - S)
    
    # Initialize cash flow matrix. This will store the cash flow at the exercise time for each path.
    # Initially, we assume exercise at maturity for all paths.
    cash_flows = np.zeros_like(S)
    cash_flows[:, -1] = intrinsic_value[:, -1]
    
    # Iterate backwards from the second-to-last time step to the first
    for t in range(num_steps - 1, 0, -1):
        # Find paths that are "in-the-money" at the current time step.
        # Regression is only performed on these paths, as there's no incentive
        # to exercise an out-of-the-money option.
        in_the_money_paths = np.where(intrinsic_value[:, t] > 0)[0]
        
        # If no paths are in the money, there's nothing to do, so we continue.
        if len(in_the_money_paths) == 0:
            continue
            
        # Select the relevant stock prices (X values for regression)
        X = S[in_the_money_paths, t]
        
        # Select the discounted future cash flows (Y values for regression)
        # Find the next exercise time for each path from t+1 onwards
        future_cash_flow_times = np.argmax(cash_flows[in_the_money_paths, t+1:], axis=1)
        future_cash_flows = cash_flows[in_the_money_paths, t+1:][np.arange(len(in_the_money_paths)), future_cash_flow_times]
        
        # Discount these future cash flows back to the current time t
        Y = future_cash_flows * np.exp(-r * (future_cash_flow_times + 1) * dt)
        
        # Perform polynomial regression (e.g., degree 2)
        # We use basis functions [1, S, S^2]
        # np.polyfit finds coefficients [c2, c1, c0] for c2*X^2 + c1*X + c0
        coeffs = np.polyfit(X, Y, 2)
        
        # Estimate the continuation value for all in-the-money paths
        continuation_value = np.polyval(coeffs, X)
        
        # --- 3. Optimal Exercise Decision ---
        
        # Identify which paths should be exercised early
        exercise_paths = in_the_money_paths[intrinsic_value[in_the_money_paths, t] > continuation_value]
        
        # For the paths we exercise early:
        # - Set the cash flow at the current time t to the intrinsic value.
        # - Zero out all future cash flows for these paths, as the option is now exercised.
        if len(exercise_paths) > 0:
            cash_flows[exercise_paths, t] = intrinsic_value[exercise_paths, t]
            cash_flows[exercise_paths, t+1:] = 0

    # --- 4. Pricing ---
    
    # The final option price is the average of all discounted cash flows.
    # Find the time of the first cash flow for each path.
    first_cash_flow_times = np.argmax(cash_flows > 0, axis=1)
    
    # Get the corresponding cash flow values
    final_cash_flows = cash_flows[np.arange(num_paths), first_cash_flow_times]
    
    # Discount them back to time 0
    discount_factors = np.exp(-r * first_cash_flow_times * dt)
    discounted_cash_flows = final_cash_flows * discount_factors
    
    # The price is the mean of these discounted cash flows
    price = np.mean(discounted_cash_flows)
    
    return price