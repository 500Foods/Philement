# Coverage Build Configuration for Hydrogen
#
# This file contains the coverage build configuration with gcov instrumentation.
#
# Usage:
# cmake --build . --target hydrogen_coverage : Build with gcov coverage instrumentation
# cmake --build . --target coverage          : Build coverage executable with embedded payload
# cmake --build . --target all_variants      : Build all variants including coverage
#

# Coverage build for unit testing with gcov - uses clean build/coverage/src directory structure like Unity tests
# Create object files for each hydrogen source file to maintain clean directory structure
set(HYDROGEN_COVERAGE_OBJECT_FILES "")
foreach(SOURCE_FILE ${HYDROGEN_SOURCES})
    # Get relative path from src directory
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src" ${SOURCE_FILE})
    get_filename_component(OBJ_DIR ${REL_PATH} DIRECTORY)
    get_filename_component(OBJ_BASENAME ${REL_PATH} NAME_WE)

    # Set up output directory structure to mirror source in build/coverage/src/
    if(OBJ_DIR)
        set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/coverage/src/${OBJ_DIR}")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    else()
        set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/coverage/src")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    endif()

    # Generate project-specific include flags for Unity (same as main build)
    set(PROJECT_INCLUDE_FLAGS
        "-I${CMAKE_CURRENT_SOURCE_DIR}/.."              # Root: enables <src/...>
        "-I${CMAKE_CURRENT_SOURCE_DIR}/../src"          # Explicit src if needed
        "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests"        # Enables <unity/...>
        "-I${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity"  # Enables <unity/...>
        "-I${CMAKE_CURRENT_SOURCE_DIR}"                 # Current dir
    )

    # Create custom command to compile source file to object file
    add_custom_command(
        OUTPUT ${OUTPUT_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_C_COMPILER}
            ${HYDROGEN_BASE_CFLAGS}
            -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
            -DHYDROGEN_COVERAGE_BUILD
            -DVERSION='"${HYDROGEN_VERSION}"'
            -DRELEASE='"${HYDROGEN_RELEASE}"'
            -DBUILD_TYPE='"Coverage"'
            -I${CMAKE_CURRENT_SOURCE_DIR}/../src
            ${JANSSON_CFLAGS}
            ${MICROHTTPD_CFLAGS}
            ${WEBSOCKETS_CFLAGS}
            ${BROTLI_CFLAGS}
            ${UUID_CFLAGS}
            ${LUA_CFLAGS}
            ${PROJECT_INCLUDE_FLAGS}
            -c ${SOURCE_FILE} -o ${OUTPUT_OBJ}
        DEPENDS ${SOURCE_FILE}
        COMMENT "Compiling ${REL_PATH} to coverage object file"
    )

    list(APPEND HYDROGEN_COVERAGE_OBJECT_FILES ${OUTPUT_OBJ})
endforeach()

# Create target that depends on all coverage object files
add_custom_target(hydrogen_coverage_objects DEPENDS ${HYDROGEN_COVERAGE_OBJECT_FILES})

# Create the coverage executable using the individual object files
add_executable(hydrogen_coverage ${HYDROGEN_COVERAGE_OBJECT_FILES})
add_dependencies(hydrogen_coverage hydrogen_coverage_objects)
target_link_libraries(hydrogen_coverage PRIVATE ${HYDROGEN_BASE_LIBS})
target_link_options(hydrogen_coverage PRIVATE --coverage -lgcov -no-pie)
set_target_properties(hydrogen_coverage PROPERTIES
    OUTPUT_NAME hydrogen_coverage
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    LINKER_LANGUAGE C
)

# Add custom command to show build completion for coverage target
add_custom_command(TARGET hydrogen_coverage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}Coverage build${NORMAL} ${GREEN}completed successfully: hydrogen_coverage${NORMAL}"
    COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}${INFO} Coverage files located in: ${CMAKE_CURRENT_SOURCE_DIR}/../build/coverage/src/${NORMAL}"
    VERBATIM
)

# Coverage build with payload embedding (similar to release but without UPX compression)
# This matches the coverage binary behavior for comprehensive testing
add_custom_target(coverage
    DEPENDS hydrogen_coverage payload
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Building coverage executable with payload..."
    # Build the coverage executable
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target hydrogen_coverage
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Creating copy for payload embedding..."
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage_temp
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Appending encrypted payload to coverage executable..."
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_payload.sh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage_temp ${CMAKE_CURRENT_SOURCE_DIR}/../payloads/payload.tar.br.enc
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage_temp ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage_temp
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Payload embedded successfully"
    COMMAND ${CMAKE_COMMAND} -E true
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Coverage build with encrypted payload appended successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating coverage build with payload (no compression)"
)

# Clean coverage executables target (used by coverage with payload)
add_custom_target(clean_coverage_executables
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Cleaning previous coverage executables..."
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage
    COMMENT "Cleaning previous coverage executables"
)