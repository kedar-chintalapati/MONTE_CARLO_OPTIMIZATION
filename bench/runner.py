import yaml
import time
import json
import datetime
import platform
import subprocess
import os
from pathlib import Path

# --- Robust Path Calculation ---
# Get the path to this file, then find its parent directory (bench/), then that parent's parent (project root).
PROJECT_ROOT = Path(__file__).resolve().parent.parent

# --- Backend Registration ---
# We need to add the project root to sys.path for module imports to work reliably
# when running as a script. However, when running with python -m, this might not be necessary,
# but it provides robustness. Let's first try to fix the file paths and keep imports simple.

from lsm_py.lsm_pricer import price_american_put_lsm
from lsm_py.lsm_numba import price_american_put_lsm_numba # <--- ADD THIS

# ADD THIS IMPORT
from lsm_cpp.lsm_cpp_backend import price_american_put_lsm_cpp


# This dictionary acts as a dispatcher. The key is the backend name from the
# YAML file, and the value is the function to call.
BACKENDS = {
    "py": price_american_put_lsm, "numba": price_american_put_lsm_numba, "cpp": price_american_put_lsm_cpp
}

def get_git_commit_hash() -> str:
    """Gets the current git commit hash to ensure reproducibility."""
    try:
        return subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode('ascii').strip()
    except Exception:
        return "N/A"

def get_system_info() -> dict:
    """Collects basic system and environment information."""
    return {
        "python_version": platform.python_version(),
        "os": platform.system(),
        "os_version": platform.version(),
        "processor": platform.processor(),
        "git_commit": get_git_commit_hash(),
    }

def run_benchmark():
    """
    Main function to load experiments, run them, and save results.
    """
    print("--- Starting Benchmark Runner ---")
    
    # Load experiment definitions from the YAML file, ensuring we use the project root path
    config_path = PROJECT_ROOT / "experiments.yml"
    with open(config_path, "r") as f:
        config = yaml.safe_load(f)

    # Prepare the results directory and file, ensuring we use the project root path
    results_dir = PROJECT_ROOT / "RESULTS"
    results_dir.mkdir(exist_ok=True)
    results_file = results_dir / f"run_results_{datetime.datetime.now():%Y%m%d_%H%M%S}.jsonl"

    print(f"Saving results to: {results_file}")
    
    system_info = get_system_info()
    
    # ... rest of run_benchmark function remains exactly the same ...
    for case in config["cases"]:
        print(f"\nRunning case: {case['name']}...")
        
        for backend_name in case["backends"]:
            if backend_name not in BACKENDS:
                print(f"  - Backend '{backend_name}' not found. Skipping.")
                continue

            print(f"  - Using backend: '{backend_name}'")
            
            args = {**case["params"], **case["simulation"]}
            
            start_time = time.perf_counter_ns()
            price = BACKENDS[backend_name](**args)
            end_time = time.perf_counter_ns()
            
            duration_ms = (end_time - start_time) / 1_000_000
            print(f"    - Price: {price:.5f}, Time: {duration_ms:.2f} ms")

            result = {
                "case_name": case["name"],
                "backend": backend_name,
                "timestamp_utc": datetime.datetime.utcnow().isoformat(),
                "inputs": args,
                "outputs": {"price": price, "time_ms": duration_ms},
                "system_info": system_info,
            }
            
            with open(results_file, "a") as f:
                f.write(json.dumps(result) + "\n")

    print("\n--- Benchmark run finished successfully! ---")

if __name__ == "__main__":
    # The check for experiments.yml is now less critical as we use PROJECT_ROOT,
    # but we keep the main execution guard.
    run_benchmark()