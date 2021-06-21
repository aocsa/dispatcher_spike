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


###################################################################################################
# - conda environment -----------------------------------------------------------------------------

set(BSQL_BLD_PREFIX "")
if ("$ENV{CONDA_BUILD}" STREQUAL "1")
    set(CMAKE_SYSTEM_PREFIX_PATH "$ENV{BUILD_PREFIX};$ENV{PREFIX};${CMAKE_SYSTEM_PREFIX_PATH}")
    message(STATUS "Conda build detected, CMAKE_SYSTEM_PREFIX_PATH set to: ${CMAKE_SYSTEM_PREFIX_PATH}")
    set(BSQL_BLD_PREFIX "$ENV{BUILD_PREFIX}")

    set(AWS_SDK_CPP_INSTALL_DIR $ENV{BUILD_PREFIX})
    set(GOOGLE_CLOUD_CPP_INSTALL_DIR $ENV{BUILD_PREFIX})
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{BUILD_PREFIX}/lib/cmake)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{BUILD_PREFIX}/lib64/cmake)
    set(ENV{PKG_CONFIG_PATH} $ENV{BUILD_PREFIX}/lib/pkgconfig/) # used by find libcurl and openssl
    set(ENV{LD_LIBRARY_PATH} $ENV{BUILD_PREFIX}/lib/) # to link the tests correctly against libcurl.so
    link_directories($ENV{BUILD_PREFIX}/lib/)
    link_directories($ENV{BUILD_PREFIX}/lib64/)

elseif (DEFINED ENV{CONDA_PREFIX})
    set(CMAKE_SYSTEM_PREFIX_PATH "$ENV{CONDA_PREFIX};${CMAKE_SYSTEM_PREFIX_PATH}")
    message(STATUS "Conda environment detected, CMAKE_SYSTEM_PREFIX_PATH set to: ${CMAKE_SYSTEM_PREFIX_PATH}")
    set(BSQL_BLD_PREFIX "$ENV{CONDA_PREFIX}")

    set(AWS_SDK_CPP_INSTALL_DIR $ENV{CONDA_PREFIX})
    set(GOOGLE_CLOUD_CPP_INSTALL_DIR $ENV{CONDA_PREFIX})
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{CONDA_PREFIX}/lib/cmake)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{CONDA_PREFIX}/lib64/cmake)
    set(ENV{PKG_CONFIG_PATH} $ENV{CONDA_PREFIX}/lib/pkgconfig/) # used by find libcurl and openssl
    set(ENV{LD_LIBRARY_PATH} $ENV{CONDA_PREFIX}/lib/) # to link the tests correctly against libcurl.so
    link_directories($ENV{CONDA_PREFIX}/lib/)
    link_directories($ENV{CONDA_PREFIX}/lib64/)

endif ()
message(WARNING "BSQL_BLD_PREFIX set to: ${BSQL_BLD_PREFIX}")
message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

#########################################################################3


# Include CMake modules
include(FeatureSummary)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CTest)
include(cmake/MakeArtifact.cmake)

find_package(Threads)
# clion not works?
#find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
enable_testing ()

# BEGIN find arrow
find_path(ARROW_INCLUDE_DIR "arrow" HINTS "$ENV{ARROW_ROOT}/include")
find_library(ARROW_LIB "arrow" NAMES libarrow HINTS "$ENV{ARROW_ROOT}/lib" "$ENV{ARROW_ROOT}/build")
message(STATUS "ARROW: ARROW_INCLUDE_DIR set to ${ARROW_INCLUDE_DIR}")
message(STATUS "ARROW: ARROW_LIB set to ${ARROW_LIB}")
add_library(arrow SHARED IMPORTED ${ARROW_LIB})

if(ARROW_INCLUDE_DIR AND ARROW_LIB)
    set_target_properties(arrow PROPERTIES IMPORTED_LOCATION ${ARROW_LIB})
endif(ARROW_INCLUDE_DIR AND ARROW_LIB)
# END find arrow


link_directories($ENV{CONDA_PREFIX}/lib)

include_directories(
        ${PROJECT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}

        ${PROJECT_SOURCE_DIR} ${CMAKE_BINARY_DIR}
        ${BSQL_BLD_PREFIX}/include
        ${BSQL_BLD_PREFIX}/include/libcudf/libcudacxx
        ${BSQL_BLD_PREFIX}/include/blazingdb/io
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/thirdparty/jitify
        ${PYTHON_INCLUDE_DIRS}
        "${ARROW_INCLUDE_DIR}"
)


set(COMMON_TARGET_INCLUDE_DIRECTORIES
    $ENV{CONDA_PREFIX}/include
    "${ARROW_INCLUDE_DIR}"
    ${CMAKE_SOURCE_DIR}/src
)


set(COMMON_LIBRARIES
        pthread

        ${CUDF_COMPUTE_LIBRARIES}

        zmq

#        ${UCX_LIBRARIES}

        parquet
        arrow
        snappy

        zstd
        lz4

        ${S3_LIBRARY}

        ${GCS_LIBRARY}

        libboost_filesystem.so
        libboost_system.so
        libboost_regex.so

        # TODO jp c.gonzales make use of standard cmake find_package here (see above)
        #spdlog::spdlog
        libspdlog.so
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
            arrow_dispatcher

        INCLUDES PUBLIC
            ${COMMON_TARGET_INCLUDE_DIRECTORIES}

        LIBRARIES
            ${COMMON_LIBRARIES}

        SOURCES
           ${COMPUTE_ARROW_SRC_FILES}
)

set_target_properties(arrow_dispatcher PROPERTIES CUDA_ARCHITECTURES "72")
add_executable(arrow_main src/arrow_main.cpp)
target_link_libraries(arrow_main arrow_dispatcher)

make_artifact (
        TARGET
            cudf_dispatcher

        INCLUDES PUBLIC
            ${COMMON_TARGET_INCLUDE_DIRECTORIES}

        LIBRARIES
            ${COMMON_LIBRARIES}

        SOURCES
            ${SRC_FILES}
)

set_target_properties(cudf_dispatcher PROPERTIES CUDA_ARCHITECTURES "72")
add_executable(cudf_main src/cudf_main.cu)
set_target_properties(cudf_main PROPERTIES CUDA_ARCHITECTURES "72")
target_link_libraries(cudf_main cudf_dispatcher)

# Print the project summary
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)