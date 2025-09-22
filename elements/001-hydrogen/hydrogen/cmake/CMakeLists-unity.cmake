# Unity Test Build Configuration for Hydrogen
#
# This file contains the Unity test build configuration.
#
# Usage:
# cmake --build . --target unity_tests       : Build Unity unit tests with coverage
# cmake --build . --target all_variants      : Build all variants including Unity tests
#
# Unity Tests Integration
#
# This section integrates Unity unit tests directly into the main build system
# instead of using a separate CMakeLists.txt file. This allows for parallel builds
# and better integration while maintaining the separate build/unity/src structure.

# Unity framework path
set(UNITY_FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/framework/Unity")

# Unity test source files - search recursively to support directory structure
file(GLOB_RECURSE UNITY_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/src/*_test*.c")

# Unity framework source
set(UNITY_FRAMEWORK_SOURCES
    ${UNITY_FRAMEWORK_DIR}/src/unity.c
)

# Add mock sources for Unity tests
set(UNITY_MOCK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_libwebsockets.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_libmicrohttpd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_launch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_status.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_landing.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_network.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_system.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_threads.c
)

# Print-specific mock sources (only linked to print tests)
set(UNITY_PRINT_MOCK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_logging.c
)

# Create Unity test object files in build/unity/src directory structure (same as original)
# Include hydrogen.c for coverage instrumentation consistency, but don't link it into test executables
set(UNITY_HYDROGEN_SOURCES ${HYDROGEN_SOURCES})
set(UNITY_HYDROGEN_SOURCES_FOR_LINKING ${UNITY_HYDROGEN_SOURCES})
list(REMOVE_ITEM UNITY_HYDROGEN_SOURCES_FOR_LINKING "${CMAKE_CURRENT_SOURCE_DIR}/../src/hydrogen.c")
# Remove print_queue_manager.c from regular Unity compilation since it will be built with mock defines
list(REMOVE_ITEM UNITY_HYDROGEN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/print/print_queue_manager.c")

# Create object libraries for Unity components
add_library(unity_framework OBJECT ${UNITY_FRAMEWORK_SOURCES})
target_compile_options(unity_framework PRIVATE
    ${HYDROGEN_BASE_CFLAGS}
    -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
    -Wno-double-promotion
    -I${UNITY_FRAMEWORK_DIR}/src
    -DUNITY_INCLUDE_DOUBLE
)

add_library(unity_mocks OBJECT ${UNITY_MOCK_SOURCES})
target_compile_options(unity_mocks PRIVATE
    ${HYDROGEN_BASE_CFLAGS}
    -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
    -DVERSION="${HYDROGEN_VERSION}"
    -DRELEASE="${HYDROGEN_RELEASE}"
    -DBUILD_TYPE="Unity"
    -DUNITY_INCLUDE_DOUBLE
    -DUSE_MOCK_LIBWEBSOCKETS
    -I${CMAKE_CURRENT_SOURCE_DIR}/../src
    -I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity
    -I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks
    -I${UNITY_FRAMEWORK_DIR}/src
    ${JANSSON_CFLAGS}
    ${MICROHTTPD_CFLAGS}
    ${WEBSOCKETS_CFLAGS}
    ${BROTLI_CFLAGS}
    ${UUID_CFLAGS}
    ${LUA_CFLAGS}
)

add_library(unity_print_mocks OBJECT ${UNITY_PRINT_MOCK_SOURCES})
target_compile_options(unity_print_mocks PRIVATE
    ${HYDROGEN_BASE_CFLAGS}
    -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
    -DVERSION="${HYDROGEN_VERSION}"
    -DRELEASE="${HYDROGEN_RELEASE}"
    -DBUILD_TYPE="Unity"
    -DUNITY_INCLUDE_DOUBLE
    -DUSE_MOCK_LOGGING
    -I${CMAKE_CURRENT_SOURCE_DIR}/../src
    -I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity
    -I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks
    -I${UNITY_FRAMEWORK_DIR}/src
    ${JANSSON_CFLAGS}
    ${MICROHTTPD_CFLAGS}
    ${WEBSOCKETS_CFLAGS}
    ${BROTLI_CFLAGS}
    ${UUID_CFLAGS}
    ${LUA_CFLAGS}
)

# Create static library for Unity tests to improve build performance and ccache effectiveness
add_library(hydrogen_unity STATIC ${UNITY_HYDROGEN_SOURCES_FOR_LINKING})
target_compile_options(hydrogen_unity PRIVATE
    ${HYDROGEN_BASE_CFLAGS}
    -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
    -DVERSION="${HYDROGEN_VERSION}"
    -DRELEASE="${HYDROGEN_RELEASE}"
    -DBUILD_TYPE="Unity"
    -DUNITY_INCLUDE_DOUBLE
    -DUNITY_TEST_MODE
)
target_include_directories(hydrogen_unity PRIVATE ${HYDROGEN_INCLUDE_DIRS})
target_precompile_headers(hydrogen_unity PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src/hydrogen.h")

# Custom target to build print_queue_manager.c with MOCK_LOGGING for Unity tests
add_custom_target(print_queue_manager_mock
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/unity/src/print
    COMMAND ${CMAKE_C_COMPILER}
        ${HYDROGEN_BASE_CFLAGS}
        -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
        -DVERSION="${HYDROGEN_VERSION}"
        -DRELEASE="${HYDROGEN_RELEASE}"
        -DBUILD_TYPE="Unity"
        -DUNITY_INCLUDE_DOUBLE
        -DUNITY_TEST_MODE
        -DUSE_MOCK_LOGGING -Dlog_this=mock_log_this
        -I${CMAKE_CURRENT_SOURCE_DIR}/../src
        -I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity
        -I${UNITY_FRAMEWORK_DIR}/src
        -I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks
        ${JANSSON_CFLAGS}
        ${MICROHTTPD_CFLAGS}
        ${WEBSOCKETS_CFLAGS}
        ${BROTLI_CFLAGS}
        ${UUID_CFLAGS}
        ${LUA_CFLAGS}
        -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/print/print_queue_manager.c
        -o ${CMAKE_BINARY_DIR}/unity/src/print/print_queue_manager.o
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../src/print/print_queue_manager.c
    COMMENT "Building print_queue_manager.c with MOCK_LOGGING support"
)

# Create Unity test executables for each test file
foreach(TEST_SOURCE ${UNITY_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # Get relative path from tests/unity/src to determine output directory structure
    file(RELATIVE_PATH TEST_REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/src" ${TEST_SOURCE})
    get_filename_component(TEST_DIR ${TEST_REL_PATH} DIRECTORY)

    # Determine output directory - mirror source structure in build/unity/src
    if(TEST_DIR AND NOT TEST_DIR STREQUAL ".")
        set(TEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/unity/src/${TEST_DIR}")
    else()
        set(TEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/unity/src")
    endif()

    # Check if this is a websocket, terminal, mdns, or print test to include mock headers
    string(FIND "${TEST_SOURCE}" "websocket" IS_WEBSOCKET_TEST)
    string(FIND "${TEST_SOURCE}" "terminal" IS_TERMINAL_TEST)
    string(FIND "${TEST_SOURCE}" "mdns" IS_MDNS_TEST)
    string(FIND "${TEST_SOURCE}" "print" IS_PRINT_TEST)
    if(IS_WEBSOCKET_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBWEBSOCKETS -DUSE_MOCK_STATUS")
    elseif(IS_TERMINAL_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBMICROHTTPD")
    elseif(IS_MDNS_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_NETWORK -DUSE_MOCK_SYSTEM -DUSE_MOCK_THREADS")
    elseif(IS_PRINT_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LOGGING -Dlog_this=mock_log_this")
    else()
        set(MOCK_INCLUDES "")
        set(MOCK_DEFINES "")
    endif()

    # Create the Unity test executable
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    # Set compile options for the test
    target_compile_options(${TEST_NAME} PRIVATE
        ${HYDROGEN_BASE_CFLAGS}
        -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
        -DVERSION="${HYDROGEN_VERSION}"
        -DRELEASE="${HYDROGEN_RELEASE}"
        -DBUILD_TYPE="Coverage"
        -DUNITY_INCLUDE_DOUBLE
        ${MOCK_DEFINES}
        -I${CMAKE_CURRENT_SOURCE_DIR}/../src
        -I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity
        -I${UNITY_FRAMEWORK_DIR}/src
        ${MOCK_INCLUDES}
        ${JANSSON_CFLAGS}
        ${MICROHTTPD_CFLAGS}
        ${WEBSOCKETS_CFLAGS}
        ${BROTLI_CFLAGS}
    )

    # Link libraries
    target_link_libraries(${TEST_NAME}
        unity_framework
        unity_mocks
        hydrogen_unity
        ${HYDROGEN_BASE_LIBS}
        --coverage
        -lgcov
    )

    # For print tests, also link print mocks and mock print_queue_manager
    if(IS_PRINT_TEST GREATER -1)
        target_link_libraries(${TEST_NAME} unity_print_mocks)
        add_dependencies(${TEST_NAME} print_queue_manager_mock)
        target_link_options(${TEST_NAME} PRIVATE "${CMAKE_BINARY_DIR}/unity/src/print/print_queue_manager.o")
    endif()

    # Add extra link flags
    target_link_options(${TEST_NAME} PRIVATE -Wl,--gc-sections)

    # Set output directory for test executable
    set_target_properties(${TEST_NAME} PROPERTIES
        OUTPUT_NAME ${TEST_NAME}
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/unity/src"
        LINKER_LANGUAGE C
    )

    # Add custom command to show build completion
    add_custom_command(TARGET ${TEST_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}Unity test build${NORMAL} ${GREEN}completed successfully: ${TEST_NAME}${NORMAL}"
        COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}${INFO} Unity test artifacts located in: ${TEST_OUTPUT_DIR}/${NORMAL}"
        VERBATIM
    )

    # Add test to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/unity/src
    )
endforeach()

# Create target that depends on all Unity object files
add_custom_target(unity_objects DEPENDS ${UNITY_OBJECT_FILES})

# Create list of all Unity test targets
set(UNITY_TEST_TARGETS "")
foreach(TEST_SOURCE ${UNITY_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    list(APPEND UNITY_TEST_TARGETS ${TEST_NAME})
endforeach()

# Unity tests build target
add_custom_target(unity_tests
    DEPENDS ${UNITY_TEST_TARGETS}
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Building Unity tests with coverage instrumentation..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target unity_objects --parallel
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target ${UNITY_TEST_TARGETS} --parallel
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Unity tests built successfully"
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Unity coverage files located in: ${CMAKE_BINARY_DIR}/unity/src/"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Unity tests with coverage instrumentation"
)

# Enable testing
enable_testing()

# Add tests
add_test(NAME hydrogen_basic_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tests/22_startup_shutdown.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME hydrogen_compilation_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_01_compilation.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME hydrogen_dependencies_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_16_library_dependencies.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME hydrogen_unity_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_11_unity.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set test properties
set_tests_properties(
    hydrogen_basic_test
    hydrogen_compilation_test
    hydrogen_dependencies_test
    PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "CMAKE_BUILD=1"
)