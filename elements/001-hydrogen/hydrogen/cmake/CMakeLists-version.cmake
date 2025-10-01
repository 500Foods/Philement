# Version Information Management
#
# This file handles version numbering based on git commits and release timestamps.
# IMPORTANT: Version and release values are cached to avoid unnecessary recompilation
# when they haven't actually changed, improving ccache effectiveness.
#
# - MAJOR.MINOR.PATCH follows semantic versioning
# - BUILD number is derived from git commit count plus offset
# - RELEASE timestamp is in ISO 8601 format (YYYYMMDD)
# - Values are cached in build/.version_cache to avoid recomputation

set(HYDROGEN_VERSION_MAJOR 1)
set(HYDROGEN_VERSION_MINOR 0)
set(HYDROGEN_VERSION_PATCH 0)

# Cache file for version information
set(VERSION_CACHE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../build/.version_cache")

# Function to read cached version/release if available
function(read_version_cache)
    if(EXISTS "${VERSION_CACHE_FILE}")
        file(READ "${VERSION_CACHE_FILE}" CACHE_CONTENT)
        string(REGEX MATCH "VERSION=([^\n]+)" _ "${CACHE_CONTENT}")
        set(CACHED_VERSION "${CMAKE_MATCH_1}" PARENT_SCOPE)
        string(REGEX MATCH "RELEASE=([^\n]+)" _ "${CACHE_CONTENT}")
        set(CACHED_RELEASE "${CMAKE_MATCH_1}" PARENT_SCOPE)
        string(REGEX MATCH "COMMIT_COUNT=([^\n]+)" _ "${CACHE_CONTENT}")
        set(CACHED_COMMIT_COUNT "${CMAKE_MATCH_1}" PARENT_SCOPE)
    else()
        set(CACHED_VERSION "" PARENT_SCOPE)
        set(CACHED_RELEASE "" PARENT_SCOPE)
        set(CACHED_COMMIT_COUNT "" PARENT_SCOPE)
    endif()
endfunction()

# Function to write version cache
function(write_version_cache VERSION RELEASE COMMIT_COUNT)
    file(WRITE "${VERSION_CACHE_FILE}" "VERSION=${VERSION}\nRELEASE=${RELEASE}\nCOMMIT_COUNT=${COMMIT_COUNT}\n")
endfunction()

# Read cached values
read_version_cache()

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

# Calculate build number from commit count
math(EXPR HYDROGEN_BUILD_NUMBER "${GIT_COMMIT_COUNT} + 1000")
set(HYDROGEN_VERSION "${HYDROGEN_VERSION_MAJOR}.${HYDROGEN_VERSION_MINOR}.${HYDROGEN_VERSION_PATCH}.${HYDROGEN_BUILD_NUMBER}")

# Get release timestamp in YYYYMMDD format
string(TIMESTAMP CURRENT_RELEASE "%Y%m%d")

# Check if we need to update the cache
set(VERSION_CHANGED FALSE)
set(RELEASE_CHANGED FALSE)

if(NOT CACHED_VERSION STREQUAL HYDROGEN_VERSION)
    set(VERSION_CHANGED TRUE)
    message(STATUS "Version changed: ${CACHED_VERSION} -> ${HYDROGEN_VERSION}")
endif()

if(NOT CACHED_RELEASE STREQUAL CURRENT_RELEASE)
    set(RELEASE_CHANGED TRUE)
    message(STATUS "Release date changed: ${CACHED_RELEASE} -> ${CURRENT_RELEASE}")
endif()

# Use cached values if nothing changed, otherwise update
if(NOT VERSION_CHANGED AND NOT RELEASE_CHANGED)
    set(HYDROGEN_VERSION "${CACHED_VERSION}")
    set(HYDROGEN_RELEASE "${CACHED_RELEASE}")
    message(STATUS "Using cached version: ${HYDROGEN_VERSION}, release: ${HYDROGEN_RELEASE}")
else()
    set(HYDROGEN_RELEASE "${CURRENT_RELEASE}")
    write_version_cache("${HYDROGEN_VERSION}" "${HYDROGEN_RELEASE}" "${GIT_COMMIT_COUNT}")
    message(STATUS "Version cache updated")
endif()

message(STATUS "Hydrogen Version: ${HYDROGEN_VERSION}")
message(STATUS "Release Timestamp: ${HYDROGEN_RELEASE}")

# Add a custom target to check if version needs updating
add_custom_target(check_version
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Checking if version number needs updating..."
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_version.sh "${CMAKE_CURRENT_SOURCE_DIR}/.." "${GIT_COMMIT_COUNT}"
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Version check completed."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking if version number needs updating based on git commits"
)