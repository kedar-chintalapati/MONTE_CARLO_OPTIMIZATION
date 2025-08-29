# tests_quant/test_oracles.py

import pytest
from core.oracles import black_scholes_european_vanilla, binomial_tree_american_vanilla

# Define a tolerance for floating point comparisons
TOL = 1e-4

# --- Test cases for Black-Scholes ---

def test_bs_european_put_itm():
    """Test Black-Scholes for a known in-the-money put option value."""
    S, K, T, r, sigma = 100, 105, 1.0, 0.05, 0.2
    expected_price = 7.9004
    calculated_price = black_scholes_european_vanilla(S, K, T, r, sigma, "put")
    assert calculated_price == pytest.approx(expected_price, abs=TOL)

def test_bs_european_call_otm():
    """Test Black-Scholes for a known out-of-the-money call option value."""
    S, K, T, r, sigma = 100, 105, 1.0, 0.05, 0.2
    expected_price = 8.0213
    calculated_price = black_scholes_european_vanilla(S, K, T, r, sigma, "call")
    assert calculated_price == pytest.approx(expected_price, abs=TOL)
    
def test_bs_at_maturity():
    """Test that at T=0, the price is the intrinsic value."""
    S, K, T, r, sigma = 110, 100, 0, 0.05, 0.2
    assert black_scholes_european_vanilla(S, K, T, r, sigma, "call") == 10.0
    assert black_scholes_european_vanilla(S - 20, K, T, r, sigma, "put") == 10.0

# --- Test cases for Binomial Tree ---

def test_bt_american_put_deep_itm():
    """Test Binomial Tree for an American put against a known value."""
    S, K, T, r, sigma, N = 100, 105, 1.0, 0.05, 0.2, 2000
    # CORRECTED expected price, based on verification. The previous value was wrong.
    expected_price = 8.7408 
    calculated_price = binomial_tree_american_vanilla(S, K, T, r, sigma, N, "put")
    # Binomial tree is an approximation, so tolerance is slightly looser
    assert calculated_price == pytest.approx(expected_price, abs=1e-3)

def test_bt_american_call_no_dividends_equals_european():
    """
    A key invariant: an American call on a non-dividend-paying stock
    should have the same price as its European counterpart.
    """
    S, K, T, r, sigma, N = 100, 105, 1.0, 0.05, 0.2, 1000
    
    american_price = binomial_tree_american_vanilla(S, K, T, r, sigma, N, "call")
    european_price = black_scholes_european_vanilla(S, K, T, r, sigma, "call")
    
    # They should be very close, difference due to tree discretization error
    assert american_price == pytest.approx(european_price, abs=1e-2)