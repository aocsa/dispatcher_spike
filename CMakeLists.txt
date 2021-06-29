project(dispatcher CXX CUDA)
cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER $ENV{CC})
set(CMAKE_CXX_COMPILER $ENV{CXX})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(DCMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    # NOTE felipe percy these flags are too strict for blazingsql
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-error=deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

    # Suppress parentheses warning which causes gmock to fail
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler -Wno-parentheses")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(CMAKE_CUDA_COMPILER_VERSION)
    # Compute the version. from  CMAKE_CUDA_COMPILER_VERSION
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1" CUDA_VERSION_MAJOR ${CMAKE_CUDA_COMPILER_VERSION})
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\2" CUDA_VERSION_MINOR ${CMAKE_CUDA_COMPILER_VERSION})
    set(CUDA_VERSION "${CUDA_VERSION_MAJOR}.${CUDA_VERSION_MINOR}" CACHE STRING "Version of CUDA as computed from nvcc.")
    mark_as_advanced(CUDA_VERSION)
endif()

message(STATUS "CUDA_VERSION_MAJOR: ${CUDA_VERSION_MAJOR}")
message(STATUS "CUDA_VERSION_MINOR: ${CUDA_VERSION_MINOR}")
message(STATUS "CUDA_VERSION: ${CUDA_VERSION}")

# Always set this convenience variable
set(CUDA_VERSION_STRING "${CUDA_VERSION}")

# Auto-detect available GPU compute architectures
set(GPU_ARCHS "ALL" CACHE STRING
        "List of GPU architectures (semicolon-separated) to be compiled for. Pass 'ALL' if you want to compile for all supported GPU architectures. Empty string means to auto-detect the GPUs on the current system")

if("${GPU_ARCHS}" STREQUAL "")
    include(cmake/EvalGpuArchs.cmake)
    evaluate_gpu_archs(GPU_ARCHS)
endif()

if("${GPU_ARCHS}" STREQUAL "ALL")
    set(GPU_ARCHS "60")
    if((CUDA_VERSION_MAJOR EQUAL 9) OR (CUDA_VERSION_MAJOR GREATER 9))
        set(GPU_ARCHS "${GPU_ARCHS};70")
    endif()
    if((CUDA_VERSION_MAJOR EQUAL 10) OR (CUDA_VERSION_MAJOR GREATER 10))
        set(GPU_ARCHS "${GPU_ARCHS};75")
    endif()
endif()
message("GPU_ARCHS = ${GPU_ARCHS}")

foreach(arch ${GPU_ARCHS})
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode=arch=compute_${arch},code=sm_${arch}")
endforeach()

list(GET GPU_ARCHS -1 ptx)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode=arch=compute_${ptx},code=compute_${ptx}")

set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-extended-lambda --expt-relaxed-constexpr")

# set warnings as errors
# TODO: remove `no-maybe-unitialized` used to suppress warnings in rmm::exec_policy
# NOTE felipe percy these flags are too strict for blazingsql: -Werror,
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Werror=cross-execution-space-call -Xcompiler -Wall,-Wno-error=deprecated-declarations")

option(DISABLE_DEPRECATION_WARNING "Disable warnings generated from deprecated declarations." OFF)
if(DISABLE_DEPRECATION_WARNING)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif(DISABLE_DEPRECATION_WARNING)

# Option to enable line info in CUDA device compilation to allow introspection when profiling / memchecking
option(CMAKE_CUDA_LINEINFO "Enable the -lineinfo option for nvcc (useful for cuda-memcheck / profiler" OFF)
if(CMAKE_CUDA_LINEINFO)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -lineinfo")
endif(CMAKE_CUDA_LINEINFO)

# Debug options
if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Building with debugging flags")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -G -g -Xcompiler -rdynamic")
endif(CMAKE_BUILD_TYPE MATCHES Debug)
set(CUDACC_DEFINE -D__CUDACC__)




# Include CMake modules
include(FeatureSummary)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CTest)
include(cmake/MakeArtifact.cmake)

find_package(Threads)
enable_testing ()

include_directories(
        ${PROJECT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
)


set(COMMON_TARGET_INCLUDE_DIRECTORIES
    $ENV{CONDA_PREFIX}/include
    "${ARROW_INCLUDE_DIR}"
    ${CMAKE_SOURCE_DIR}/src
)


set(COMMON_LIBRARIES
    pthread
)

set(COMMON_SRC_FILES
 

)

set(COMPUTE_ARROW_SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/select/arrow/select.cpp 
)

set(COMPUTE_CUDF_SRC_FILES
        ${COMPUTE_ARROW_SRC_FILES}
        ${PROJECT_SOURCE_DIR}/src/select/cudf/select.cu

)


## Target source files
set(SRC_FILES
        # COMPUTE
        ${COMPUTE_CUDF_SRC_FILES}
        ${COMPUTE_ARROW_SRC_FILES}
)

make_artifact (
        TARGET
            kernels_cpu

        INCLUDES PUBLIC
            ${COMMON_TARGET_INCLUDE_DIRECTORIES}

        LIBRARIES
            ${COMMON_LIBRARIES}

        SOURCES
           ${COMPUTE_ARROW_SRC_FILES}
)

set_target_properties(kernels_cpu PROPERTIES CUDA_ARCHITECTURES "72")
add_executable(cpu_main src/cpu_main.cpp)
target_link_libraries(cpu_main kernels_cpu)

make_artifact (
        TARGET
            kernels_cuda

        INCLUDES PUBLIC
            ${COMMON_TARGET_INCLUDE_DIRECTORIES}

        LIBRARIES
            ${COMMON_LIBRARIES}

        SOURCES
            ${SRC_FILES}
)

set_target_properties(kernels_cuda PROPERTIES CUDA_ARCHITECTURES "72")
add_executable(cuda_cpu_main src/cuda_cpu_main.cu)
set_target_properties(cuda_cpu_main PROPERTIES CUDA_ARCHITECTURES "72")
target_link_libraries(cuda_cpu_main kernels_cuda)

# Print the project summary
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)