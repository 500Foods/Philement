#!/bin/bash
# ──────────────────────────────────────────────────────────────────────────────
# Trial Build Script for Hydrogen CMake Build System
# 
# This script performs a trial build with advanced diagnostics, similar to the
# Makefile's trial target. It shows only warnings and errors, detects unused
# source files, and runs shutdown tests.
# ──────────────────────────────────────────────────────────────────────────────

set -e

BUILD_DIR="$1"
SOURCE_DIR="$2"
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NORMAL='\033[0m'
INFO='🛈'
PASS='✅'
WARN='⚠️'

if [ -z "$BUILD_DIR" ] || [ -z "$SOURCE_DIR" ]; then
    echo "Usage: $0 <build_dir> <source_dir>"
    exit 1
fi

cd "$BUILD_DIR"

printf "%s%s Starting trial build (warnings and errors only)...%s\n" "$CYAN" "$INFO" "$NORMAL"

# Clean previous build
cmake --build . --target clean >/dev/null 2>&1 || true

# Build and capture output
BUILD_OUTPUT=$(cmake --build . --target hydrogen 2>&1)

# Filter output to show only warnings, errors, and success messages
echo "$BUILD_OUTPUT" | grep -v -E "^[[:space:]]*\[|^make\[[0-9]+\]: (Entering|Leaving) directory|^[[:space:]]*Compiling|^[[:space:]]*${INFO}" | \
grep -E "error:|warning:|undefined reference|collect2|ld returned|built successfully|completed successfully" --color=always || true

# Check if build was successful
if echo "$BUILD_OUTPUT" | grep -q "completed successfully" && ! echo "$BUILD_OUTPUT" | grep -q -E "warning:|error:|undefined reference|ld returned"; then
    printf "\n%s%s Build successful with no warnings/errors. Copying binary to root directory for testing...%s\n" "$CYAN" "$INFO" "$NORMAL"
    cp "$BUILD_DIR/hydrogen" "$SOURCE_DIR/../hydrogen"
    printf "\n%s%s Running shutdown test...%s\n" "$CYAN" "$INFO" "$NORMAL"
    
# Run shutdown test if available
if [ -f "$SOURCE_DIR/../tests/test_20_shutdown.sh" ]; then
    "$SOURCE_DIR/../tests/test_20_shutdown.sh"
else
    printf "%s%s Shutdown test script not found%s\n" "$YELLOW" "$WARN" "$NORMAL"
fi
    
    printf "\n%s%s Analyzing unused source files...%s\n" "$CYAN" "$INFO" "$NORMAL"
    
    # Check for unused source files using the map file
    MAP_FILE="$BUILD_DIR/hydrogen.map"
    if [ -f "$MAP_FILE" ]; then
        printf "%s%s Analyzing map file for used source files...%s\n" "$CYAN" "$INFO" "$NORMAL"
        
        # Extract CMake object file paths from the map file
        # These have the format: CMakeFiles/hydrogen.dir/full/path/to/source.c.o
        USED_OBJS=$(grep -oE "CMakeFiles/hydrogen\.dir/[^[:space:]]*\.c\.o" "$MAP_FILE" | sort -u)
        
        # Show sample of object/source references for debugging
        if [ -n "$USED_OBJS" ]; then
            printf "%s%s Sample of object/source references from map file (first 5):%s\n" "$CYAN" "$INFO" "$NORMAL"
            echo "$USED_OBJS" | head -n 5 | while read -r ref; do
                printf "  %s- %s%s\n" "$CYAN" "$ref" "$NORMAL"
            done
        else
            printf "%s%s No CMake object file references found in map file.%s\n" "$YELLOW" "$WARN" "$NORMAL"
        fi
        
        # Find all source files
        ALL_SRCS=$(find "$SOURCE_DIR/../src" -name "*.c" | sed "s|$SOURCE_DIR/../||g" | sort)
        
        printf "%s%s Total source files found: %s%s\n" "$CYAN" "$INFO" "$(echo "$ALL_SRCS" | wc -l)" "$NORMAL"
        printf "%s%s Total object/source references in map: %s%s\n" "$CYAN" "$INFO" "$(echo "$USED_OBJS" | wc -l)" "$NORMAL"
        
        # Map used objects back to source files
        USED_SRCS=""
        for obj in $USED_OBJS; do
            # Extract the source path from CMakeFiles/hydrogen.dir/full/path/to/source.c.o
            # We want to extract just the src/... part
            src_path=$(echo "$obj" | sed 's|^.*hydrogen/||' | sed 's|\.o$||')
            
            # Check if this source file exists in our source list
            if echo "$ALL_SRCS" | grep -q "$src_path"; then
                USED_SRCS="$USED_SRCS\n$src_path"
            fi
        done
        USED_SRCS=$(echo -e "$USED_SRCS" | grep -v "^$" | sort -u)
        
        printf "%s%s Total matched source files: %s%s\n" "$CYAN" "$INFO" "$(echo "$USED_SRCS" | wc -l)" "$NORMAL"
        
        # Read ignored files
        IGNORED_SRCS=""
        if [ -f "$SOURCE_DIR/../.trial-ignore" ]; then
            IGNORED_SRCS=$(grep -v '^#' "$SOURCE_DIR/../.trial-ignore" | grep -v '^$' | sed "s|^\./|src/|")
        fi
        
        # Count ignored files properly (only count non-empty lines)
        if [ -n "$IGNORED_SRCS" ]; then
            IGNORED_COUNT=$(echo "$IGNORED_SRCS" | grep -c .)
        else
            IGNORED_COUNT=0
        fi
        printf "%s%s Total ignored files: %s%s\n" "$CYAN" "$INFO" "$IGNORED_COUNT" "$NORMAL"
        
        # Find unused sources
        UNUSED_SRCS=$(comm -23 <(echo "$ALL_SRCS") <(echo "$USED_SRCS"))
        REPORTABLE_SRCS=$(comm -23 <(echo "$UNUSED_SRCS") <(echo "$IGNORED_SRCS" | sort))
        
        if [ -z "$REPORTABLE_SRCS" ]; then
            printf "%s%s No unexpected unused source files detected%s\n" "$GREEN" "$PASS" "$NORMAL"
        else
            printf "%s%s Unexpected unused source files detected:%s\n" "$YELLOW" "$WARN" "$NORMAL"
            echo "$REPORTABLE_SRCS" | while read -r file; do
                if [ -n "$file" ]; then
                    printf "  %s- %s%s\n" "$YELLOW" "$file" "$NORMAL"
                fi
            done
        fi
    else
        printf "%s%s Map file not found, skipping unused file analysis%s\n" "$YELLOW" "$WARN" "$NORMAL"
    fi
    
    printf "%s────────────────────────────────────────────────────────────────%s\n" "$BLUE" "$NORMAL"
fi
