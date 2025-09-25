#!/usr/bin/env bash

# CHANGELOG
# 2025-07-15: Added Unity test compilation to catch errors in test code during trial builds
# 2025-09-24: Added QUICK parameter to skip cleaning and cmake configuration

pushd . >/dev/null 2>&1 || return

# Check if QUICK parameter is supplied
QUICK_MODE=false
for arg in "$@"; do
    if [[ "${arg}" == "QUICK" ]]; then
        QUICK_MODE=true
        break
    fi
done

if [[ "${QUICK_MODE}" != "true" ]]; then
    echo "$(date +%H:%M:%S.%3N || true) - Cleaning Build Directory"
    rm -rf build/*
    rm -rf hydrogen_*

    echo "$(date +%H:%M:%S.%3N || true) - Configuring CMake"
    cd cmake && cmake -S . -B ../build --preset default >/dev/null 2>&1
else
    cd cmake || return
fi

echo "$(date +%H:%M:%S.%3N || true) - Default Build"

# Build main project with payload and check for errors
BUILD_OUTPUT=$(cmake --build ../build --target hydrogen 2>&1)
ERRORS=$(echo "${BUILD_OUTPUT}" | grep -B 2 -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [[ -n "${ERRORS}" ]]; then
    echo "${ERRORS}"
    exit 1
fi

echo "$(date +%H:%M:%S.%3N || true) - Unity Build"

# Build Unity tests and check for errors
UNITY_BUILD_OUTPUT=$(cmake --build ../build --target unity_tests 2>&1)
UNITY_ERRORS=$(echo "${UNITY_BUILD_OUTPUT}" | grep -B 2 -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [[ -n "${UNITY_ERRORS}" ]]; then
    echo "${UNITY_ERRORS}"
    exit 1
fi

popd >/dev/null 2>&1 || return

# Remove hydrogen_naked if it exists (byproduct of release builds)
if [[ -f "hydrogen_naked" ]]; then
    rm -f "hydrogen_naked"
fi


# Check if build was successful
if (echo "${UNITY_BUILD_OUTPUT}" | grep -q "completed successfully" || echo "${UNITY_BUILD_OUTPUT}" | grep -q "no work to do") && [[ -z "${ERRORS}" ]]; then
    echo "$(date +%H:%M:%S.%3N || true) - Build Successful"
    
    # Binary is already created in root directory by hydrogen target
    
    # Run shutdown test
    echo "$(date +%H:%M:%S.%3N || true) - Running Shutdown Test"

    if [[ -f "tests/test_16_shutdown.sh" ]]; then
        "tests/test_16_shutdown.sh" >/dev/null 2>&1 && echo "$(date +%H:%M:%S.%3N || true) - Shutdown Test Passed" || echo "❌ Shutdown test failed"
    else
        echo "⚠️  Shutdown test not found"
    fi
    
    # Analyze unused files
    MAP_FILE="build/regular/regular.map"
    if [[ -f "${MAP_FILE}" ]]; then
        echo "$(date +%H:%M:%S.%3N || true) - Checking Source Files"
        
        # Extract object files that are actually linked
        USED_OBJS=$(awk '/^LOAD.*\.o$/ {print $2}' "${MAP_FILE}" | grep -v "^/usr" | sort -u || true)
        
        # Convert object files back to source file paths
        USED_SRCS=""
        for obj in ${USED_OBJS}; do
            # Extract the source path from the object file path
            # Object files are like: regular/src/config/config.o
            if [[ "${obj}" == *"/src/"* ]]; then
                src_path=$(echo "${obj}" | sed 's|^.*/src/|./|' | sed 's|\.o$|.c|' || true)
                USED_SRCS="${USED_SRCS}\n${src_path}"
            fi
        done
        USED_SRCS=$(echo -e "${USED_SRCS}" | grep -v "^$" | sort -u || true)
        
        # Find all source files relative to src directory
        ALL_SRCS=$(find "src" -name "*.c" | sed "s|^src/|./|" | sort || true)
        
        # Read ignored files (already in ./ format)
        IGNORED_SRCS=""
        if [[ -f ".trial-ignore" ]]; then
            IGNORED_SRCS=$(grep -v '^#' ".trial-ignore" | grep -v '^$' | sort || true)
        fi
        
        # Find unused sources
        UNUSED_SRCS=$(comm -23 <(echo "${ALL_SRCS}") <(echo "${USED_SRCS}"))
        REPORTABLE_SRCS=$(comm -23 <(echo "${UNUSED_SRCS}") <(echo "${IGNORED_SRCS}"))
        
        if [[ -z "${REPORTABLE_SRCS}" ]]; then
            echo "$(date +%H:%M:%S.%3N || true) - Source Files Checked"
        else
            echo "$(date +%H:%M:%S.%3N || true) - Source Files Checked"
            echo "⚠️  Unused source files:"
            echo "${REPORTABLE_SRCS}" | while read -r file; do
                if [[ -n "${file}" ]]; then
                    echo "  ${file}"
                fi
            done
        fi
    else
        echo "⚠️  Map file not found, skipping unused file analysis"
    fi
    echo "$(date +%H:%M:%S.%3N || true) - Trial Build Complete"
else
    echo "❌ Build failed"
    exit 1
fi
