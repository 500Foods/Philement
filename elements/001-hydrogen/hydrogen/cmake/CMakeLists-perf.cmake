# Performance Build Configuration for Hydrogen
#
# This file contains the performance-optimized build configuration.
#
# Usage:
# cmake --build . --target hydrogen_perf     : Build with aggressive optimizations
# cmake --build . --target all_variants      : Build all variants including performance
#
# Performance-optimized build
hydrogen_add_executable_target(perf "Performance"
    "-O3 -g -ffast-math -finline-functions -funroll-loops -DNDEBUG"
    "-flto=auto -no-pie"
)