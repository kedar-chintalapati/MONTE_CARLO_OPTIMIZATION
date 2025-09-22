# Reproducing the LSM Performance Workbench Environment

This guide provides detailed instructions to set up the complete development environment for this project on a Windows machine.

## Prerequisites

1.  **Git:** [Download and install Git for Windows](https://git-scm.com/download/win).
2.  **Miniconda (or Anaconda):** [Download and install Miniconda](https://docs.conda.io/en/latest/miniconda.html). This is used for managing the Python environment.
3.  **Node.js LTS:** [Download and install Node.js](https://nodejs.org/). This is used for the frontend React application.
4.  **Visual Studio 2022 Community Edition:** [Download and install Visual Studio Community](https://visualstudio.microsoft.com/downloads/). This is required for the C++ compiler (MSVC).
    -   During installation, you **must** select the **"Desktop development with C++"** workload. This includes the necessary compiler, CMake integration, and SDKs.

## Step 1: Clone the Repository

Clone the project and all its submodules (`pybind11`, `xsimd`) using the `--recursive` flag.

```bash
git clone --recursive https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git
cd lsm-performance-suite
```


## Step 2: Set Up the Python Backend
**Create and Activate Conda Environment:**
```Powershell
conda create -n quant_perf python=3.11
conda activate quant_perf
```
Initialize PowerShell for Conda (if not already done):
Open the "Anaconda Prompt" from the Start Menu.
Run conda init powershell.
Close and re-open PowerShell.
If you get an error about script execution, open an Administrator PowerShell and run: Set-ExecutionPolicy RemoteSigned -Scope CurrentUser.

**Install Python Dependencies:**

A requirements.txt file is provided for convenience.
```Powershell
# Make sure you are in the project root and the 'quant_perf' env is active
pip install -r requirements.txt
```

**Step 3: Compile the C++ Module**
Generate Build Files:
Create a build directory and run CMake. This will detect your Visual Studio compiler.
```Powershell
mkdir build
cd build
cmake ..
```
Compile the Code:
Run the build command. We compile in "Release" mode for maximum performance.
```Powershell
cmake --build . --config Release
cd ..
```
Move the Compiled File:
After compilation, a .pyd file (e.g., lsm_cpp_backend.cp311-win_amd64.pyd) will be located in lsm_cpp/Release/. You must manually move this file up one level into the lsm_cpp/ directory.

**Step 4: Set Up the Frontend**

Navigate to the UI Directory:
```Powershell
cd ui
```
Install Node.js Dependencies:

```Powershell
npm install
```

**Step 5: Run the Application**
You are now ready to run the application as described in the main README.md.
