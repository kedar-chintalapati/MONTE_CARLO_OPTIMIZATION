# core/oracles.py

import numpy as np
from scipy.stats import norm

def black_scholes_european_vanilla(
    S: float, K: float, T: float, r: float, sigma: float, option_type: str
) -> float:
    """
    Prices a European vanilla option using the Black-Scholes closed-form solution.

    Args:
        S: Current stock price.
        K: Strike price.
        T: Time to maturity (in years).
        r: Risk-free interest rate (annualized).
        sigma: Volatility of the underlying stock (annualized).
        option_type: 'call' or 'put'.

    Returns:
        The price of the option.
    """
    if T <= 0:
        if option_type == "call":
            return max(S - K, 0.0)
        elif option_type == "put":
            return max(K - S, 0.0)
        else:
            raise ValueError("option_type must be 'call' or 'put'")

    d1 = (np.log(S / K) + (r + 0.5 * sigma**2) * T) / (sigma * np.sqrt(T))
    d2 = d1 - sigma * np.sqrt(T)

    if option_type == "call":
        price = S * norm.cdf(d1) - K * np.exp(-r * T) * norm.cdf(d2)
    elif option_type == "put":
        price = K * np.exp(-r * T) * norm.cdf(-d2) - S * norm.cdf(-d1)
    else:
        raise ValueError("option_type must be 'call' or 'put'")
        
    return price

# core/oracles.py (replace the binomial tree function with this)

# core/oracles.py (replace the binomial tree function with this final version)

# core/oracles.py (replace the binomial tree function with this final, correct version)

# core/oracles.py (replace the binomial tree function with this final, correct version)

def binomial_tree_american_vanilla(
    S: float, K: float, T: float, r: float, sigma: float, N: int, option_type: str
) -> float:
    """
    Prices an American vanilla option using a Cox-Ross-Rubinstein binomial tree.
    (Definitively corrected implementation using separate read/write buffers to prevent state pollution)

    Args:
        S: Current stock price.
        K: Strike price.
        T: Time to maturity (in years).
        r: Risk-free interest rate (annualized).
        sigma: Volatility of the underlying stock (annualized).
        N: Number of steps in the binomial tree. A high number (e.g., 2000+) is needed for accuracy.
        option_type: 'call' or 'put'.

    Returns:
        The price of the American option.
    """
    if option_type not in ["call", "put"]:
        raise ValueError("option_type must be 'call' or 'put'")

    # --- 1. Precompute constants ---
    dt = T / N
    u = np.exp(sigma * np.sqrt(dt))
    d = 1.0 / u
    q = (np.exp(r * dt) - d) / (u - d)
    discount = np.exp(-r * dt)

    # --- 2. Initialize option values at maturity (time N) ---
    # There are N+1 possible outcomes at maturity.
    V = np.zeros(N + 1)
    for j in range(N + 1):
        price_at_maturity = S * (u**j) * (d**(N - j))
        if option_type == 'put':
            V[j] = max(0, K - price_at_maturity)
        else:  # 'call'
            V[j] = max(0, price_at_maturity - K)

    # --- 3. Backward induction from time N-1 down to 0 ---
    # V holds the values from the next time step (i+1).
    # We calculate the values for the current time step (i) and store them in a new array.
    for i in range(N - 1, -1, -1):
        # At time i, there are i+1 nodes.
        V_current_step = np.zeros(i + 1)
        for j in range(i + 1):
            # The continuation value is calculated from the two possible future nodes in V.
            continuation = discount * (q * V[j + 1] + (1 - q) * V[j])
            
            # The exercise value is calculated at the current node (i, j).
            price_at_node = S * (u**j) * (d**(i - j))
            if option_type == 'put':
                exercise = max(0, K - price_at_node)
            else:  # 'call'
                exercise = max(0, price_at_node - K)
            
            # The option's value at this node is the max of holding or exercising.
            V_current_step[j] = max(continuation, exercise)
        
        # The calculated values for this step become the future values for the previous step.
        V = V_current_step

    # After the loop, V will be an array with a single element: the price at time 0.
    return V[0]