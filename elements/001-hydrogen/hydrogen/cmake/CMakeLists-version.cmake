# Version Information Management
#
# This file handles version numbering based on git commits and release timestamps.
#
# - MAJOR.MINOR.PATCH follows semantic versioning
# - BUILD number is derived from git commit count plus offset
# - RELEASE timestamp is in ISO 8601 format

set(HYDROGEN_VERSION_MAJOR 1)
set(HYDROGEN_VERSION_MINOR 0)
set(HYDROGEN_VERSION_PATCH 0)

# Get git commit count
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log --oneline .
        COMMAND wc -l
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT GIT_COMMIT_COUNT)
        set(GIT_COMMIT_COUNT 0)
    endif()
else()
    set(GIT_COMMIT_COUNT 0)
endif()

# Add a custom target to check if version needs updating
add_custom_target(check_version
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Checking if version number needs updating..."
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_version.sh "${CMAKE_CURRENT_SOURCE_DIR}/.." "${GIT_COMMIT_COUNT}"
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Version check completed."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking if version number needs updating based on git commits"
)

math(EXPR HYDROGEN_BUILD_NUMBER "${GIT_COMMIT_COUNT} + 1000")
set(HYDROGEN_VERSION "${HYDROGEN_VERSION_MAJOR}.${HYDROGEN_VERSION_MINOR}.${HYDROGEN_VERSION_PATCH}.${HYDROGEN_BUILD_NUMBER}")

# Release timestamp in YYYYMMDD format
string(TIMESTAMP HYDROGEN_RELEASE "%Y%m%d" )

message(STATUS "Hydrogen Version: ${HYDROGEN_VERSION}")
message(STATUS "Release Timestamp: ${HYDROGEN_RELEASE}")