# Regular Build Configuration for Hydrogen
#
# This file contains the regular build configuration with standard optimizations and debug symbols.
#
# Usage:
# cmake --build . --target hydrogen          : Build default version with standard optimizations and embedded payload
# cmake --build . --target all_variants      : Build all variants including regular
#
# Default build target
# -rdynamic: export Lua symbols so C rocks (e.g. lua-brotli) can dlopen
# against a statically linked liblua (Lua 5.5 / LUA_55_PLAN).
hydrogen_add_executable_target(regular "Regular" "-O2 -g" "-no-pie -rdynamic")

# Regular build with payload embedding (similar to release but without UPX compression)
# This matches the regular binary behavior for comprehensive functionality
add_custom_target(hydrogen
    DEPENDS regular payload
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Building regular executable with payload..."
    # Build the regular executable
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target regular
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Creating copy for payload embedding..."
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_temp
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Appending encrypted payload to regular executable..."
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_payload.sh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_temp ${CMAKE_CURRENT_SOURCE_DIR}/../payloads/payload.tar.br.enc
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_temp ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_temp
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Payload embedded successfully"
    COMMAND ${CMAKE_COMMAND} -E true
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Regular build with encrypted payload appended successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating regular build with payload (standard build with embedded functionality)"
)