# Valgrind Build Configuration for Hydrogen
#
# This file contains the valgrind-compatible build configuration.
#
# Usage:
# cmake --build . --target hydrogen_valgrind : Build optimized for Valgrind analysis
# cmake --build . --target all_variants      : Build all variants including valgrind
#
# Valgrind-compatible build
hydrogen_add_executable_target(valgrind "Valgrind-Compatible"
    "-O0 -g"
    "-no-pie"
)