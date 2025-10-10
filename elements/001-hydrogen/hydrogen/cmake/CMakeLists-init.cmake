# Basic CMake Configuration and Package Setup
#
# This file contains the fundamental CMake setup, package finding,
# compiler configuration, and base settings.

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Compiler configuration and base flags
set(HYDROGEN_BASE_CFLAGS
    -std=c17
    -Wall
    -Wextra
    -pedantic
    -Werror
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wold-style-definition
    -Wformat=2
    -Wshadow
    -Wconversion
    -Wnull-dereference
    -Wuninitialized
    -Wunused
    -Wcast-align
    -Wdouble-promotion
    -Wswitch-enum
    -Wmissing-declarations
    -Wundef
    -Wpointer-arith
    -Wstrict-aliasing
    -Wwrite-strings
    -D_GNU_SOURCE
    -D_POSIX_C_SOURCE=200809L
    -ffunction-sections
    -fdata-sections
    -pipe  # Use pipes instead of temporary files for faster compilation
)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find required libraries
pkg_check_modules(JANSSON REQUIRED jansson)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(WEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(BROTLI REQUIRED libbrotlienc libbrotlidec)
pkg_check_modules(UUID REQUIRED uuid)
pkg_check_modules(LUA REQUIRED lua)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Base libraries
set(HYDROGEN_BASE_LIBS
    m
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${WEBSOCKETS_LIBRARIES}
    ${MICROHTTPD_LIBRARIES}
    ${JANSSON_LIBRARIES}
    ${BROTLI_LIBRARIES}
    ${UUID_LIBRARIES}
    ${LUA_LIBRARIES}
)

# Include directories
set(HYDROGEN_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${JANSSON_INCLUDE_DIRS}
    ${MICROHTTPD_INCLUDE_DIRS}
    ${WEBSOCKETS_INCLUDE_DIRS}
    ${BROTLI_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
)

# Source file discovery
file(GLOB_RECURSE HYDROGEN_SOURCES
    "../src/*.c"
)

# Remove files that should not be linked (from .trial-ignore)
list(REMOVE_ITEM HYDROGEN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/hydrogen_not.c"
)

# Function to add an executable target with specific build configuration using clean directory structure
function(hydrogen_add_executable_target target_name build_type extra_cflags extra_ldflags)
    # Create object files for each hydrogen source file to maintain clean directory structure
    set(TARGET_OBJECT_FILES "")
    foreach(SOURCE_FILE ${HYDROGEN_SOURCES})
        # Get relative path from src directory
        file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src" ${SOURCE_FILE})
        get_filename_component(OBJ_DIR ${REL_PATH} DIRECTORY)
        get_filename_component(OBJ_BASENAME ${REL_PATH} NAME_WE)

        # Set up output directory structure to mirror source in build/[target]/src/
        if(OBJ_DIR)
            set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/${target_name}/src/${OBJ_DIR}")
            set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
        else()
            set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/${target_name}/src")
            set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
        endif()

        # Prepare extra compile flags as a list
        if(extra_cflags)
            separate_arguments(extra_cflags_list UNIX_COMMAND "${extra_cflags}")
        else()
            set(extra_cflags_list "")
        endif()

        # Generate project-specific include flags (prefix plain dirs with -I; systems already have it)
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
                ${extra_cflags_list}
                -DVERSION='"${HYDROGEN_VERSION}"'
                -DRELEASE='"${HYDROGEN_RELEASE}"'
                -DBUILD_TYPE='"${build_type}"'
                ${JANSSON_CFLAGS}
                ${MICROHTTPD_CFLAGS}
                ${WEBSOCKETS_CFLAGS}
                ${BROTLI_CFLAGS}
                ${UUID_CFLAGS}
                ${LUA_CFLAGS}
                ${PROJECT_INCLUDE_FLAGS}
                -c ${SOURCE_FILE} -o ${OUTPUT_OBJ}
            DEPENDS ${SOURCE_FILE}
            COMMENT "Compiling ${REL_PATH} to ${target_name} object file"
        )

        list(APPEND TARGET_OBJECT_FILES ${OUTPUT_OBJ})
    endforeach()

    # Create target that depends on all object files
    add_custom_target(${target_name}_objects DEPENDS ${TARGET_OBJECT_FILES})

    # Create the executable using the individual object files
    add_executable(${target_name} ${TARGET_OBJECT_FILES})
    add_dependencies(${target_name} ${target_name}_objects)

    # Set include directories
    target_include_directories(${target_name} PUBLIC ${HYDROGEN_INCLUDE_DIRS})
    
    # Add extra compile flags if provided
    if(extra_cflags)
        separate_arguments(extra_cflags_list UNIX_COMMAND "${extra_cflags}")
        target_compile_options(${target_name} PRIVATE ${extra_cflags_list})
    endif()

    # Set compile definitions
    target_compile_definitions(${target_name} PRIVATE
        VERSION="${HYDROGEN_VERSION}"
        RELEASE="${HYDROGEN_RELEASE}"
        BUILD_TYPE="${build_type}"
    )

    # Link libraries
    target_link_libraries(${target_name} PRIVATE ${HYDROGEN_BASE_LIBS})

    # Add extra link flags if provided
    if(extra_ldflags)
        separate_arguments(extra_ldflags_list UNIX_COMMAND "${extra_ldflags}")
        target_link_options(${target_name} PRIVATE ${extra_ldflags_list})
    endif()

    # Add garbage collection flags
    target_link_options(${target_name} PRIVATE -Wl,--gc-sections)

    # Generate map file in target directory
    target_link_options(${target_name} PRIVATE -Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/../build/${target_name}/${target_name}.map)

    # Set output name and properties
    # Binary names should have hydrogen_ prefix except for regular which is just "hydrogen"
    if(${target_name} STREQUAL "regular")
        set(BINARY_NAME "hydrogen")
    else()
        set(BINARY_NAME "hydrogen_${target_name}")
    endif()

    set_target_properties(${target_name} PROPERTIES
        OUTPUT_NAME ${BINARY_NAME}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
        LINKER_LANGUAGE C
    )

    # Add custom command to show build completion
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}${build_type} build${NORMAL} ${GREEN}completed successfully: ${target_name}${NORMAL}"
        COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}${INFO} Object files located in: ${CMAKE_CURRENT_SOURCE_DIR}/../build/${target_name}/src/${NORMAL}"
        VERBATIM
    )

    if(target_name)
        message(STATUS "Debug for ${target_name}: HYDROGEN_INCLUDE_DIRS = ${HYDROGEN_INCLUDE_DIRS}")
        get_target_property(INCLUDES ${target_name} INCLUDE_DIRECTORIES)
        message(STATUS "Debug for ${target_name}: Actual target includes after = ${INCLUDES}")
    endif()

endfunction()