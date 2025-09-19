cmake_minimum_required(VERSION 3.12)
project(LsmPerformanceSuite LANGUAGES CXX)

add_subdirectory(external/pybind11)
add_subdirectory(external/xsimd)
find_package(OpenMP REQUIRED)

pybind11_add_module(lsm_cpp_backend
    lsm_cpp/src/lsm_pricer.cpp
    lsm_cpp/src/lsm_pricer_arena.cpp
    lsm_cpp/src/lsm_pricer_simd.cpp
    lsm_cpp/src/lsm_pricer_ultimate.cpp
    lsm_cpp/src/lsm_pricer_mp.cpp # <-- ADD THIS
    lsm_cpp/src/bindings.cpp
)

target_link_libraries(lsm_cpp_backend PRIVATE xsimd OpenMP::OpenMP_CXX)

set_target_properties(lsm_cpp_backend PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lsm_cpp")
set_target_properties(lsm_cpp_backend PROPERTIES RUNTIME_OUTPUT_DIRECTORY_$<CONFIG> "${CMAKE_SOURCE_DIR}/lsm_cpp")
set_target_properties(lsm_cpp_backend PROPERTIES CXX_STANDARD 17)
if(MSVC)
    target_compile_options(lsm_cpp_backend PRIVATE /O2 /arch:AVX2 /openmp)
else()
    target_compile_options(lsm_cpp_backend PRIVATE -O3 -march=native -fopenmp)
endif()