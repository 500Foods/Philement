# Release Build Configuration for Hydrogen
#
# This file contains the release build configuration with special optimizations,
# UPX compression, and payload embedding.
#
# Usage:
# cmake --build . --target hydrogen_release  : Build stripped, compressed executable
# cmake --build . --target all_variants      : Build all variants including release
#
# Release build with special optimizations, UPX compression, and payload embedding
# This matches the original Makefile 'make release' behavior exactly

# Clean executables target (used by release)
add_custom_target(clean_executables
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Cleaning previous release executables..."
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked
    COMMENT "Cleaning previous release executables"
)

# Release build with special optimizations, UPX compression, and payload embedding
# This matches the original Makefile 'make release' behavior exactly
add_custom_target(hydrogen_release
    DEPENDS clean_executables payload
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Building release executable..."
    # Build the release executable
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target release
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Stripping debug information from release executable..."
    COMMAND strip -s ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E echo "STEP 1: Moving hydrogen_release to hydrogen_naked"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked
    COMMAND ${CMAKE_COMMAND} -E echo "STEP 2: UPX compress hydrogen_naked"
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/apply_upx.sh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked
    COMMAND ${CMAKE_COMMAND} -E echo "STEP 3: Copy hydrogen_naked to hydrogen_release"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E echo "STEP 4: Embed payload in hydrogen_release - hydrogen_naked untouched"
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_payload.sh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release ${CMAKE_CURRENT_SOURCE_DIR}/../payloads/payload.tar.br.enc
    COMMAND ${CMAKE_COMMAND} -E echo "FINAL CHECK:"
    COMMAND ls -lh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Payload embedded successfully"
    COMMAND ${CMAKE_COMMAND} -E true
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Release build with encrypted payload appended successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating release build with compression and payload (matches Makefile behavior)"
)

# Internal target for building just the release executable (without payload processing)
hydrogen_add_executable_target(release "Release"
    "-Os -s -DNDEBUG -march=x86-64 -flto=auto -fno-stack-protector -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections"
    "-flto=auto -Wl,--gc-sections -Wl,--strip-all"
)