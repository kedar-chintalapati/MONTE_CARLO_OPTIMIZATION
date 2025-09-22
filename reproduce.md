### Terminal 1: Start the Backend API

```powershell
# Navigate to the project root directory
cd lsm-performance-suite

# Activate the Conda environment
conda activate quant_perf

# Set the required environment variable for OpenMP
$env:KMP_DUPLICATE_LIB_OK="TRUE"

# Start the API server
uvicorn api:app --reload

Terminal 2: Start the Frontend UI
code
Powershell
# Navigate to the UI subdirectory
cd lsm-performance-suite/ui

# Start the React development server
npm start
Your browser will automatically open to http://localhost:3000, where the application will be running.
code
Code
**Step 2: The Final `REPRODUCE.md`**

This file provides the detailed, step-by-step instructions for a new user to set up your project from scratch on a Windows machine.

*   Create a new file in the project's **root directory** named `REPRODUCE.md`.
*   Add the following content to it:

```markdown
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


Step 2: Set Up the Python Backend
Create and Activate Conda Environment:
code
Powershell
conda create -n quant_perf python=3.11
conda activate quant_perf
Initialize PowerShell for Conda (if not already done):
Open the "Anaconda Prompt" from the Start Menu.
Run conda init powershell.
Close and re-open PowerShell.
If you get an error about script execution, open an Administrator PowerShell and run: Set-ExecutionPolicy RemoteSigned -Scope CurrentUser.
Install Python Dependencies:
A requirements.txt file is provided for convenience.
code
Powershell
# Make sure you are in the project root and the 'quant_perf' env is active
pip install -r requirements.txt
Step 3: Compile the C++ Module
Generate Build Files:
Create a build directory and run CMake. This will detect your Visual Studio compiler.
code
Powershell
mkdir build
cd build
cmake ..
Compile the Code:
Run the build command. We compile in "Release" mode for maximum performance.
code
Powershell
cmake --build . --config Release
cd ..
Move the Compiled File:
After compilation, a .pyd file (e.g., lsm_cpp_backend.cp311-win_amd64.pyd) will be located in lsm_cpp/Release/. You must manually move this file up one level into the lsm_cpp/ directory.
Step 4: Set Up the Frontend
Navigate to the UI Directory:
code
Powershell
cd ui
Install Node.js Dependencies:
code
Powershell
npm install
Step 5: Run the Application
You are now ready to run the application as described in the main README.md.
