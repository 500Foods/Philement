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
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_pthread.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_libpq.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_libmysqlclient.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_libdb2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_libsqlite3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks/mock_terminal_websocket.c
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

set(UNITY_OBJECT_FILES "")

# Create object files for Unity sources (shared across all tests)
foreach(SOURCE_FILE ${UNITY_HYDROGEN_SOURCES})
    # Get relative path from src directory
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src" ${SOURCE_FILE})
    get_filename_component(OBJ_DIR ${REL_PATH} DIRECTORY)
    get_filename_component(OBJ_BASENAME ${REL_PATH} NAME_WE)

    # Set up output directory structure to mirror source in build/unity/src/
    if(OBJ_DIR)
        set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/${OBJ_DIR}")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    else()
        set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    endif()

    # Check if this is a websocket, terminal, mdns, postgresql, mysql, db2, or sqlite source file to include mock headers
    string(FIND "${SOURCE_FILE}" "websocket" IS_WEBSOCKET_SOURCE)
    string(FIND "${SOURCE_FILE}" "terminal" IS_TERMINAL_SOURCE)
    string(FIND "${SOURCE_FILE}" "mdns" IS_MDNS_SOURCE)
    string(FIND "${SOURCE_FILE}" "postgresql" IS_POSTGRESQL_SOURCE)
    string(FIND "${SOURCE_FILE}" "mysql" IS_MYSQL_SOURCE)
    string(FIND "${SOURCE_FILE}" "db2" IS_DB2_SOURCE)
    string(FIND "${SOURCE_FILE}" "sqlite" IS_SQLITE_SOURCE)
    if(IS_WEBSOCKET_SOURCE GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBWEBSOCKETS")
    elseif(IS_TERMINAL_SOURCE GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBMICROHTTPD")
    elseif(IS_MDNS_SOURCE GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_THREADS")
    elseif(IS_POSTGRESQL_SOURCE GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBPQ")
    elseif(IS_MYSQL_SOURCE GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBMYSQLCLIENT")
    elseif(IS_DB2_SOURCE GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBDB2")
    elseif(IS_SQLITE_SOURCE GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBSQLITE3")
    else()
        set(MOCK_INCLUDES "")
        set(MOCK_DEFINES "")
    endif()

    # Create custom command to compile source file to object file with Unity-specific flags
    add_custom_command(
        OUTPUT ${OUTPUT_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_C_COMPILER}
            ${HYDROGEN_BASE_CFLAGS}
            -O0 -g -fno-omit-frame-pointer
            --coverage -fprofile-arcs -ftest-coverage
            -DVERSION='"${HYDROGEN_VERSION}"'
            -DRELEASE='"${HYDROGEN_RELEASE}"'
            -DBUILD_TYPE='"Unity"'
            -DUNITY_INCLUDE_DOUBLE
            -DUNITY_TEST_MODE
            -DUSE_MOCK_LOGGING
            -Dlog_this=mock_log_this
            ${MOCK_DEFINES}
            -I${CMAKE_CURRENT_SOURCE_DIR}/../src
            -I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity
            -I${UNITY_FRAMEWORK_DIR}/src
            ${MOCK_INCLUDES}
            ${JANSSON_CFLAGS}
            ${MICROHTTPD_CFLAGS}
            ${WEBSOCKETS_CFLAGS}
            ${BROTLI_CFLAGS}
            ${UUID_CFLAGS}
            ${LUA_CFLAGS}
            -c ${SOURCE_FILE} -o ${OUTPUT_OBJ}
        DEPENDS ${SOURCE_FILE}
        COMMENT "Compiling ${REL_PATH} to Unity object file"
    )

    list(APPEND UNITY_OBJECT_FILES ${OUTPUT_OBJ})
endforeach()

# Create target that depends on all Unity object files
add_custom_target(unity_objects DEPENDS ${UNITY_OBJECT_FILES})

# Create static library for Unity tests to improve build performance and ccache effectiveness
add_library(hydrogen_unity STATIC ${UNITY_OBJECT_FILES})
add_dependencies(hydrogen_unity unity_objects)

# Set linker language explicitly for the static library
set_target_properties(hydrogen_unity PROPERTIES LINKER_LANGUAGE C)

# Set include directories for the Unity library
target_include_directories(hydrogen_unity PRIVATE ${HYDROGEN_INCLUDE_DIRS})

# Add precompiled headers for hydrogen.h (included in nearly every file)
target_precompile_headers(hydrogen_unity PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/hydrogen.h"
)

# Set Unity-specific compile definitions
target_compile_definitions(hydrogen_unity PRIVATE
    VERSION="${HYDROGEN_VERSION}"
    RELEASE="${HYDROGEN_RELEASE}"
    BUILD_TYPE="Unity"
    UNITY_INCLUDE_DOUBLE
    UNITY_TEST_MODE
)

# Create object libraries for Unity components
add_library(unity_framework OBJECT ${UNITY_FRAMEWORK_SOURCES})
target_compile_options(unity_framework PRIVATE
    ${HYDROGEN_BASE_CFLAGS}
    -O0 -g -fno-omit-frame-pointer
    -Wno-double-promotion
    -I${UNITY_FRAMEWORK_DIR}/src
    -DUNITY_INCLUDE_DOUBLE
)

add_library(unity_mocks OBJECT ${UNITY_MOCK_SOURCES})
target_compile_options(unity_mocks PRIVATE
    ${HYDROGEN_BASE_CFLAGS}
    -O0 -g -fno-omit-frame-pointer
    -DVERSION="${HYDROGEN_VERSION}"
    -DRELEASE="${HYDROGEN_RELEASE}"
    -DBUILD_TYPE="Unity"
    -DUNITY_INCLUDE_DOUBLE
    -DUSE_MOCK_LIBWEBSOCKETS
    -DUSE_MOCK_TERMINAL_WEBSOCKET
    -DUSE_MOCK_SYSTEM
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
    -O0 -g -fno-omit-frame-pointer
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

    # Check if this is a websocket, terminal, mdns, postgresql, mysql, db2, sqlite, or print test to include mock headers
    string(FIND "${TEST_SOURCE}" "websocket" IS_WEBSOCKET_TEST)
    string(FIND "${TEST_SOURCE}" "terminal" IS_TERMINAL_TEST)
    string(FIND "${TEST_SOURCE}" "mdns" IS_MDNS_TEST)
    string(FIND "${TEST_SOURCE}" "postgresql" IS_POSTGRESQL_TEST)
    string(FIND "${TEST_SOURCE}" "mysql" IS_MYSQL_TEST)
    string(FIND "${TEST_SOURCE}" "db2" IS_DB2_TEST)
    string(FIND "${TEST_SOURCE}" "sqlite" IS_SQLITE_TEST)
    string(FIND "${TEST_SOURCE}" "print" IS_PRINT_TEST)
    if(IS_WEBSOCKET_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBWEBSOCKETS -DUSE_MOCK_STATUS -DUSE_MOCK_TERMINAL_WEBSOCKET")
    elseif(IS_TERMINAL_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBMICROHTTPD -DUSE_MOCK_SYSTEM")
    elseif(IS_MDNS_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_NETWORK -DUSE_MOCK_SYSTEM -DUSE_MOCK_THREADS")
    elseif(IS_POSTGRESQL_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBPQ")
    elseif(IS_MYSQL_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBMYSQLCLIENT")
    elseif(IS_DB2_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBDB2")
    elseif(IS_SQLITE_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LIBSQLITE3")
    elseif(IS_PRINT_TEST GREATER -1)
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LOGGING -Dlog_this=mock_log_this")
    else()
        set(MOCK_INCLUDES "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/mocks")
        set(MOCK_DEFINES "-DUSE_MOCK_LOGGING -DUSE_MOCK_LIBMICROHTTPD -DUSE_MOCK_SYSTEM -Dlog_this=mock_log_this")
    endif()

    # Create test source object file in the appropriate subdirectory
    get_filename_component(TEST_BASENAME ${TEST_SOURCE} NAME_WE)
    if(TEST_DIR AND NOT TEST_DIR STREQUAL ".")
        set(TEST_OUTPUT_OBJ "${CMAKE_BINARY_DIR}/unity/src/${TEST_DIR}/${TEST_BASENAME}.o")
    else()
        set(TEST_OUTPUT_OBJ "${CMAKE_BINARY_DIR}/unity/src/${TEST_BASENAME}.o")
    endif()

    # Create custom command to compile test source file to object file
    add_custom_command(
        OUTPUT ${TEST_OUTPUT_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_OUTPUT_DIR}
        COMMAND ${CMAKE_C_COMPILER}
            ${HYDROGEN_BASE_CFLAGS}
            -O0 -g -fno-omit-frame-pointer
            --coverage -fprofile-arcs -ftest-coverage
            -DVERSION='"${HYDROGEN_VERSION}"'
            -DRELEASE='"${HYDROGEN_RELEASE}"'
            -DBUILD_TYPE='"Coverage"'
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
            -c ${TEST_SOURCE} -o ${TEST_OUTPUT_OBJ}
        DEPENDS ${TEST_SOURCE}
        COMMENT "Compiling ${TEST_BASENAME} test to object file"
    )

    # Create the Unity test executable using the test object and shared library
    add_executable(${TEST_NAME} ${TEST_OUTPUT_OBJ})

    # Link with Unity static library and required libraries (excluding hydrogen.c which has main())
    target_link_libraries(${TEST_NAME}
        hydrogen_unity
        unity_framework
        unity_mocks
        unity_print_mocks
        ${HYDROGEN_BASE_LIBS}
        --coverage
        -lgcov
    )

    # Add extra link flags
    target_link_options(${TEST_NAME} PRIVATE -Wl,--gc-sections)

    # Set output directory for test executable (place in subdirectory to match source structure)
    if(TEST_DIR AND NOT TEST_DIR STREQUAL ".")
        set(TEST_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/${TEST_DIR}")
    else()
        set(TEST_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src")
    endif()

    set_target_properties(${TEST_NAME} PROPERTIES
        OUTPUT_NAME ${TEST_NAME}
        RUNTIME_OUTPUT_DIRECTORY "${TEST_RUNTIME_DIR}"
        LINKER_LANGUAGE C
    )

    # Add custom command to show build completion
    add_custom_command(TARGET ${TEST_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}Unity test build${NORMAL} ${GREEN}completed successfully: ${TEST_NAME}${NORMAL}"
        COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}${INFO} Unity test executable located in: ${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/${NORMAL}"
        VERBATIM
    )

    # Special case: Embed payload into payload_test_process_payload_data for testing real payload processing
    if(TEST_NAME STREQUAL "payload_test_process_payload_data")
        add_custom_command(TARGET ${TEST_NAME} POST_BUILD
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_payload.sh $<TARGET_FILE:${TEST_NAME}> ${CMAKE_CURRENT_SOURCE_DIR}/../payloads/payload.tar.br.enc
            COMMENT "Embedding payload into ${TEST_NAME} for real payload processing tests"
            VERBATIM
        )
    endif()

    # Add test to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME}
        WORKING_DIRECTORY ${TEST_RUNTIME_DIR}
    )
endforeach()


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
    # COMMAND ${CMAKE_COMMAND} -E echo "🛈  Cleaning up Unity object files..."
    # COMMAND find ${CMAKE_BINARY_DIR}/unity/src/ -type f -name "*.o" -delete
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Unity tests built successfully"
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Unity coverage files located in: ${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/"
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