# Debug Build Configuration for Hydrogen
#
# This file contains the debug build configuration with AddressSanitizer.
#
# Usage:
# cmake --build . --target hydrogen_debug    : Build with AddressSanitizer for debugging
# cmake --build . --target all_variants      : Build all variants including debug
#
# Debug build with AddressSanitizer
hydrogen_add_executable_target(debug "Debug"
    "-g -fsanitize=address,leak -fno-omit-frame-pointer"
    "-lasan -fsanitize=address,leak -no-pie"
)