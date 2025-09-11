# lsm_performance_suite/CMakeLists.txt

# --- Project Setup ---
# Minimum version of CMake required
cmake_minimum_required(VERSION 3.12)
# Project name and language
project(LsmPerformanceSuite LANGUAGES CXX)

# --- Find pybind11 ---
# Add the submodule directory to CMake's search path
add_subdirectory(external/pybind11)
add_subdirectory(external/xsimd) # <-- ADD THIS

# --- Create the Python Module ---
# pybind11_add_module is a special command provided by pybind11.
# It handles all the complex compiler and linker settings needed to create a
# Python C++ extension.
#
# Target Name: lsm_cpp_backend (this will be the name you import in Python)
# Sources: The .cpp files to compile
pybind11_add_module(lsm_cpp_backend
    lsm_cpp/src/lsm_pricer.cpp
    lsm_cpp/src/lsm_pricer_simd.cpp
    lsm_cpp/src/bindings.cpp
)

# Link the xsimd library to our target
target_link_libraries(lsm_cpp_backend PRIVATE xsimd) # <-- ADD THIS

# --- Target Properties ---
# On Windows, pybind11 creates a .pyd file (Python Dynamic Module).
# This command ensures it's placed in a directory where Python can find it.
# We will output the compiled file into a new 'lsm_cpp' directory in the root.
# For single-config generators, it places the library in lsm_cpp/
set_target_properties(lsm_cpp_backend PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lsm_cpp"
)

# For multi-config generators (like MSVC), it places the library for a specific
# configuration (e.g., Release) into the parent lsm_cpp/ directory.
# $<CONFIG> is a "generator expression" that gets replaced by "Release", "Debug", etc.
set_target_properties(lsm_cpp_backend PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_$<CONFIG> "${CMAKE_SOURCE_DIR}/lsm_cpp"
)

# --- Compiler Flags ---
# Set the C++ standard to C++17 for modern features.
set_target_properties(lsm_cpp_backend PROPERTIES CXX_STANDARD 17)

# Add compiler flags for optimization.
# In release mode, we want maximum optimization (-O3) and we tell the
# compiler it can use instructions specific to our CPU architecture (-march=native).
if(MSVC)
    # Microsoft Visual C++ Compiler flags
    target_compile_options(lsm_cpp_backend PRIVATE /O2 /arch:AVX2)
else()
    # GCC/Clang flags
    target_compile_options(lsm_cpp_backend PRIVATE -O3 -march=native)
endif()