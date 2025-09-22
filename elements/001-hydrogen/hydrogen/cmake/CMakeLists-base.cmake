# Common Build Settings
#
# This file contains common build optimizations and settings.

# Build Performance Optimizations

# Detect available CPU cores for parallel builds
include(ProcessorCount)
ProcessorCount(CPU_CORES)
if(CPU_CORES EQUAL 0)
    set(CPU_CORES 4)  # Fallback to 4 cores
endif()

# Set parallel job count (use 200% of available cores to avoid system overload)
math(EXPR PARALLEL_JOBS "${CPU_CORES} * 20 / 10")
if(PARALLEL_JOBS LESS 1)
    set(PARALLEL_JOBS 1)
endif()

message(STATUS "Detected ${CPU_CORES} CPU cores, using ${PARALLEL_JOBS} parallel jobs")

# Enable parallel builds by default
set(CMAKE_BUILD_PARALLEL_LEVEL ${PARALLEL_JOBS} CACHE STRING "Number of parallel build jobs")

# Use Ninja generator for faster builds if available
if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    find_program(NINJA_EXECUTABLE ninja)
    if(NINJA_EXECUTABLE)
        message(STATUS "Ninja found: ${NINJA_EXECUTABLE} - Consider using -G Ninja for faster builds")
    endif()
endif()

# CPU-specific optimizations
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    # Detect CPU features for optimal performance
    execute_process(
        COMMAND grep -m1 "model name" /proc/cpuinfo
        OUTPUT_VARIABLE CPU_MODEL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Set CPU-specific optimizations based on detected processor
    if(CPU_MODEL MATCHES "Intel.*i[357]-[0-9]+K")
        set(CPU_ARCH_FLAGS "-march=native;-mtune=native")
        message(STATUS "Intel K-series CPU detected, using native optimizations")
    elseif(CPU_MODEL MATCHES "Intel")
        set(CPU_ARCH_FLAGS "-march=x86-64-v2;-mtune=intel")
        message(STATUS "Intel CPU detected, using x86-64-v2 optimizations")
    elseif(CPU_MODEL MATCHES "AMD")
        set(CPU_ARCH_FLAGS "-march=x86-64-v2;-mtune=generic")
        message(STATUS "AMD CPU detected, using x86-64-v2 optimizations")
    else()
        set(CPU_ARCH_FLAGS "-march=x86-64")
        message(STATUS "Generic x86-64 optimizations")
    endif()
else()
    set(CPU_ARCH_FLAGS "")
endif()

# Add CPU-specific optimizations to base flags for better performance
if(CPU_ARCH_FLAGS)
    list(APPEND HYDROGEN_BASE_CFLAGS ${CPU_ARCH_FLAGS})
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()