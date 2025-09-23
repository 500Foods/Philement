# CMakeLists-ninja.cmake - Ninja-specific build configurations for Hydrogen
#
# This file contains Ninja generator-specific configurations and optimizations
# for the Hydrogen build system. It is included when using the Ninja generator.

# Ninja-specific optimizations
if(CMAKE_GENERATOR STREQUAL "Ninja")
    message(STATUS "Ninja generator detected - using standard parallel build settings")
    # Note: Job pools and special Ninja settings removed to avoid path conflicts
    # Standard CMAKE_BUILD_PARALLEL_LEVEL is used instead
endif()

# Ninja-specific targets
if(CMAKE_GENERATOR STREQUAL "Ninja")
    # Target to show Ninja build status
    add_custom_target(ninja_status
        COMMAND ninja -C ${CMAKE_BINARY_DIR} -t commands
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Ninja Build Status:"
        COMMAND ninja -C ${CMAKE_BINARY_DIR} -t targets | head -20
        COMMENT "Show Ninja build status and available targets"
    )

    # Target to clean Ninja's build cache
    add_custom_target(ninja_clean_cache
        COMMAND ninja -C ${CMAKE_BINARY_DIR} -t clean
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/.ninja_deps
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/.ninja_log
        COMMENT "Clean Ninja build cache and dependencies"
    )

    # Target to show Ninja dependencies for a specific target
    add_custom_target(ninja_deps
        COMMAND ${CMAKE_COMMAND} -E echo "Usage: ninja -C ${CMAKE_BINARY_DIR} -t deps <target>"
        COMMAND ${CMAKE_COMMAND} -E echo "Example: ninja -C ${CMAKE_BINARY_DIR} -t deps hydrogen"
        COMMENT "Show how to check Ninja dependencies for targets"
    )
endif()