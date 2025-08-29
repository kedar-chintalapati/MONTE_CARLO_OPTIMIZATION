# tests_quant/test_lsm_pricer.py

import pytest
from core.oracles import binomial_tree_american_vanilla
from lsm_py.lsm_pricer import price_american_put_lsm

def test_lsm_vs_binomial_tree():
    """
    Test the LSM pricer against the binomial tree oracle for a standard case.
    
    LSM is a Monte Carlo method, so we don't expect an exact match. We test
    if the LSM price is reasonably close to the more accurate binomial price.
    """
    # --- Parameters ---
    # Use the same parameters as the validated binomial tree test case
    S0, K, T, r, sigma = 100, 105, 1.0, 0.05, 0.2
    
    # --- Oracle Price ---
    # Use a high number of steps for an accurate oracle price
    N_binomial = 2000
    expected_price = binomial_tree_american_vanilla(S0, K, T, r, sigma, N_binomial, "put")
    
    # --- LSM Price ---
    # Use a reasonable number of paths and steps for the test
    num_paths = 20000
    num_steps = 100
    calculated_price = price_american_put_lsm(S0, K, T, r, sigma, num_paths, num_steps)
    
    # --- Assertion ---
    # We check if the LSM result is within a certain tolerance of the oracle.
    # A 2% relative tolerance is reasonable for this number of paths.
    # pytest.approx handles the comparison gracefully.
    assert calculated_price == pytest.approx(expected_price, rel=0.02)