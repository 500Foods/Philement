# Regular Build Configuration for Hydrogen
#
# This file contains the regular build configuration with standard optimizations and debug symbols.
#
# Usage:
# cmake --build . --target hydrogen          : Build default version with standard optimizations
# cmake --build . --target all_variants      : Build all variants including regular
#
# Default build target
hydrogen_add_executable_target(regular "Regular" "-O2 -g" "-no-pie")