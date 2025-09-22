# api.py (DEFINITIVE, SELF-CONTAINED VERSION)

import asyncio
import uuid
import datetime
import platform
import subprocess
import yaml
import json
from pathlib import Path
from fastapi import FastAPI, BackgroundTasks, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import List, Dict, Any, Literal

# --- Self-Contained Backend Definitions ---
# By defining these here, the API is robust and independent of the runner.
from lsm_py.lsm_pricer import price_american_put_lsm
from lsm_py.lsm_numba import price_american_put_lsm_numba
from lsm_cpp.lsm_cpp_backend import (
    price_american_put_lsm_cpp, price_american_put_lsm_arena,
    price_american_put_lsm_simd, price_american_put_lsm_mp,
    price_american_put_lsm_ultimate
)

BACKENDS = {
    "py": price_american_put_lsm, "numba": price_american_put_lsm_numba,
    "cpp": price_american_put_lsm_cpp, "cpp_arena": price_american_put_lsm_arena,
    "cpp_simd": price_american_put_lsm_simd, "cpp_mp": price_american_put_lsm_mp,
    "cpp_ultimate": price_american_put_lsm_ultimate,
}

# --- Pydantic Models (Unchanged) ---
class OptionParams(BaseModel): S0: float = 100.0; K: float = 105.0; T: float = 1.0; r: float = 0.05; sigma: float = 0.2
class SimulationParams(BaseModel): num_paths: int = 102400; num_steps: int = 100; seed: int = 42
class SweepConfig(BaseModel): parameter: Literal["S0", "K", "T", "r", "sigma", "num_paths", "num_steps"]; start: float; end: float; steps: int
class ExperimentConfig(BaseModel): option_params: OptionParams; simulation_params: SimulationParams; backends: List[str]; sweep: SweepConfig | None = None

# --- In-Memory Task Storage ---
tasks: Dict[str, Dict[str, Any]] = {}

# --- App Configuration ---
app = FastAPI(title="LSM Workbench API", version="2.0.0")
app.add_middleware(CORSMiddleware, allow_origins=["*"], allow_credentials=True, allow_methods=["*"], allow_headers=["*"])
PROJECT_ROOT = Path(__file__).resolve().parent

# --- Helper Function ---
def get_system_info() -> dict:
    try:
        commit_hash = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=PROJECT_ROOT).decode('ascii').strip()
    except Exception:
        commit_hash = "N/A"
    return {"python_version": platform.python_version(), "os": platform.system(), "git_commit": commit_hash}

# --- Core Experiment Logic (Unchanged) ---
# In api.py

# ... (keep all imports, Pydantic models, tasks dict, app setup, etc.) ...

# --- The Core Experiment Logic (DEFINITIVELY CORRECTED) ---
def do_run_experiment(task_id: str, config: ExperimentConfig):
    tasks[task_id]['status'] = 'RUNNING'
    results = []
    system_info = get_system_info()

    # Define which parameters belong to which model for robust updating
    sim_params_keys = SimulationParams.model_fields.keys()
    opt_params_keys = OptionParams.model_fields.keys()

    sweep_param = config.sweep.parameter if config.sweep else None
    
    if sweep_param and config.sweep:
        # Ensure step count is at least 2 for a valid range
        steps = max(2, config.sweep.steps)
        sweep_values = [config.sweep.start + i * (config.sweep.end - config.sweep.start) / (steps - 1) for i in range(steps)]
    else:
        sweep_values = [None] # This ensures the loop runs exactly once for non-sweep runs

    total_runs = len(sweep_values) * len(config.backends)
    run_count = 0

    for i, sweep_val in enumerate(sweep_values):
        sim_params = config.simulation_params.model_copy()
        opt_params = config.option_params.model_copy()

        if sweep_param and sweep_val is not None:
            # Safely determine if the value should be an int or float
            is_int = sweep_param in ['num_paths', 'num_steps', 'seed']
            val_to_set = int(sweep_val) if is_int else sweep_val
            
            # Robustly set the swept parameter on the correct model
            if sweep_param in sim_params_keys:
                setattr(sim_params, sweep_param, val_to_set)
            elif sweep_param in opt_params_keys:
                setattr(opt_params, sweep_param, val_to_set)

        for backend_name in config.backends:
            run_count += 1
            tasks[task_id]['progress'] = f"Running {run_count}/{total_runs} ({backend_name})"

            if backend_name not in BACKENDS: continue
            
            args = {**opt_params.model_dump(), **sim_params.model_dump()}
            
            import time
            start_time = time.perf_counter()
            price = BACKENDS[backend_name](**args)
            end_time = time.perf_counter()
            duration_ms = (end_time - start_time) * 1000

            result = {
                "case_name": "dynamic_run", "backend": backend_name, "timestamp_utc": datetime.datetime.utcnow().isoformat(),
                "inputs": args, "outputs": {"price": price, "time_ms": duration_ms}, "system_info": system_info
            }
            results.append(result)

    tasks[task_id]['status'] = 'COMPLETED'
    tasks[task_id]['results'] = results
    
# ... (the rest of the file, including endpoints, is correct) ...


# --- API Endpoints ---
@app.post("/run-experiment")
def run_experiment(config: ExperimentConfig, background_tasks: BackgroundTasks):
    task_id = str(uuid.uuid4())
    tasks[task_id] = {"status": "PENDING", "results": None, "progress": "Queued"}
    background_tasks.add_task(do_run_experiment, task_id, config)
    return {"task_id": task_id}

@app.get("/task-status/{task_id}")
def get_task_status(task_id: str):
    if task_id not in tasks: raise HTTPException(status_code=404, detail="Task not found")
    return tasks[task_id]

@app.get("/backends")
def get_available_backends():
    # --- Using Descriptive Names in the API ---
    # This addresses your other point perfectly. The API will now send the clean names.
    backend_name_map = {
        "py": "Python (NumPy)", "numba": "Numba (JIT)", "cpp": "C++ (Scalar)",
        "cpp_arena": "C++ (Scalar + Arena)", "cpp_simd": "C++ (SIMD)",
        "cpp_mp": "C++ (MP + Arena)", "cpp_ultimate": "C++ (MP + SIMD + Arena)"
    }
    # We send both the key and the descriptive name to the frontend
    return [{"key": k, "name": v} for k, v in backend_name_map.items() if k in BACKENDS]