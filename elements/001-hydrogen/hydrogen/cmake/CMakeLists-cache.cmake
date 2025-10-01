# Compiler Caching Configuration
#
# This file configures ccache for optimal performance with coverage builds.

# Enable compiler caching if available
find_program(CCACHE_EXECUTABLE ccache)
if(CCACHE_EXECUTABLE)
    message(STATUS "ccache found: ${CCACHE_EXECUTABLE} - Enabling compiler caching")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})

    # Configure ccache for optimal performance with coverage builds
    set(CCACHE_ENV_VARS
        "CCACHE_MAXSIZE=20G"
        "CCACHE_SLOPPINESS=file_macro,time_macros,pch_defines,include_file_mtime,include_file_ctime,system_headers"
        "CCACHE_CPP2=yes"
        "CCACHE_DEPEND=yes"
        "CCACHE_HARDLINK=yes"
        "CCACHE_BASEDIR=${CMAKE_CURRENT_SOURCE_DIR}/.."
        "CCACHE_IGNOREOPTIONS=-DVERSION* -DRELEASE*"
    )

    # Create target to configure ccache environment
    add_custom_target(configure_ccache
        COMMAND ${CMAKE_COMMAND} -E echo "Configuring ccache for optimal performance..."
        COMMAND ${CMAKE_COMMAND} -E env ${CCACHE_ENV_VARS} ${CMAKE_COMMAND} -E echo "ccache configured with:"
        COMMAND ${CMAKE_COMMAND} -E env ${CCACHE_ENV_VARS} ccache -p
        COMMAND ${CMAKE_COMMAND} -E echo "ccache configuration complete"
        COMMENT "Configure ccache with optimal settings for coverage builds"
    )

    # Create target to show ccache statistics
    add_custom_target(ccache_stats
        COMMAND ${CMAKE_COMMAND} -E echo "🛈  ccache Statistics:"
        COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
        COMMAND ccache -s
        COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
        COMMAND ${CMAKE_COMMAND} -E echo "💡 Tips to improve ccache performance:"
        COMMAND ${CMAKE_COMMAND} -E echo "   • Run 'make configure_ccache' to apply optimal settings"
        COMMAND ${CMAKE_COMMAND} -E echo "   • Clear cache if needed: ccache -C"
        COMMAND ${CMAKE_COMMAND} -E echo "   • Check cache size: ccache -s | grep 'size'"
        COMMENT "Display ccache performance statistics and optimization tips"
    )
endif()