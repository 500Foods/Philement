# Example Programs for Hydrogen OIDC Client
#
# This section defines targets for building the example programs located in
# examples/C/. These mirror the functionality of the Makefile in that directory.
# - auth_code_flow: Authorization Code flow with PKCE
# - client_credentials: Client Credentials flow
# - password_flow: Resource Owner Password flow
#
# Available CMake targets for examples:
# cmake --build . --target auth_code_flow          : Builds the Authorization Code flow example
# cmake --build . --target client_credentials      : Builds the Client Credentials flow example
# cmake --build . --target password_flow           : Builds the Resource Owner Password flow example
# cmake --build . --target auth_code_flow_debug    : Builds the Authorization Code flow example with debug symbols
# cmake --build . --target client_credentials_debug: Builds the Client Credentials flow example with debug symbols
# cmake --build . --target password_flow_debug     : Builds the Resource Owner Password flow example with debug symbols
# cmake --build . --target all_examples            : Builds all example programs

# Find required libraries for examples
find_package(CURL REQUIRED)

# Function to add an executable target for example programs
function(hydrogen_add_example_target target_name source_file build_type extra_cflags extra_ldflags libraries)
    # Create the executable
    add_executable(${target_name} "${CMAKE_CURRENT_SOURCE_DIR}/../examples/C/${source_file}")

    # Set include directories
    target_include_directories(${target_name} PUBLIC ${HYDROGEN_INCLUDE_DIRS})

    # Set base compile options
    target_compile_options(${target_name} PRIVATE ${HYDROGEN_BASE_CFLAGS})

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
    target_link_libraries(${target_name} PRIVATE ${libraries})

    # Add extra link flags if provided
    if(extra_ldflags)
        separate_arguments(extra_ldflags_list UNIX_COMMAND "${extra_ldflags}")
        target_link_options(${target_name} PRIVATE ${extra_ldflags_list})
    endif()

    # Set output name and properties
    set_target_properties(${target_name} PROPERTIES
        OUTPUT_NAME ${target_name}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../examples/C
    )

    # Add custom command to show build completion
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}${build_type} example build${NORMAL} ${GREEN}completed successfully: ${target_name}${NORMAL}"
        VERBATIM
    )
endfunction()

# Example libraries (using jansson, curl, and OpenSSL which are already found)
set(EXAMPLE_LIBS
    ${CURL_LIBRARIES}
    ${JANSSON_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${MICROHTTPD_LIBRARIES}
)

# Authorization Code Flow example
hydrogen_add_example_target(auth_code_flow "auth_code_flow.c" "Regular" "-O2 -g" "" "${EXAMPLE_LIBS}")

# Client Credentials Flow example
hydrogen_add_example_target(client_credentials "client_credentials.c" "Regular" "-O2 -g" "" "${EXAMPLE_LIBS}")

# Resource Owner Password Flow example
hydrogen_add_example_target(password_flow "password_flow.c" "Regular" "-O2 -g" "" "${EXAMPLE_LIBS}")

# Debug versions of examples
hydrogen_add_example_target(auth_code_flow_debug "auth_code_flow.c" "Debug" "-fsanitize=address,leak -fno-omit-frame-pointer" "-lasan -fsanitize=address,leak" "${EXAMPLE_LIBS}")

hydrogen_add_example_target(client_credentials_debug "client_credentials.c" "Debug" "-fsanitize=address,leak -fno-omit-frame-pointer" "-lasan -fsanitize=address,leak" "${EXAMPLE_LIBS}")

hydrogen_add_example_target(password_flow_debug "password_flow.c" "Debug" "-fsanitize=address,leak -fno-omit-frame-pointer" "-lasan -fsanitize=address,leak" "${EXAMPLE_LIBS}")

# Target to build all examples
add_custom_target(all_examples
    DEPENDS auth_code_flow client_credentials password_flow
    COMMAND ${CMAKE_COMMAND} -E echo "✅ All examples built successfully"
    COMMENT "Building all OIDC client examples"
)