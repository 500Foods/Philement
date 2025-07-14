#!/bin/bash
# Trial Build Script - Quick build, shutdown test, and unused file detection

set -e

BUILD_DIR="$1"
SOURCE_DIR="$2"

if [ -z "$BUILD_DIR" ] || [ -z "$SOURCE_DIR" ]; then
    echo "Usage: $0 <build_dir> <source_dir>"
    exit 1
fi

cd "$BUILD_DIR"

# Clean and build, suppressing cmake configuration noise
cmake --build . --target clean >/dev/null 2>&1 || true

# Remove hydrogen_naked if it exists (byproduct of release builds)
if [ -f "$SOURCE_DIR/../hydrogen_naked" ]; then
    rm -f "$SOURCE_DIR/../hydrogen_naked"
fi
BUILD_OUTPUT=$(cmake --build . --target hydrogen 2>&1)

# Show only compiler/linker errors and warnings 
ERRORS=$(echo "$BUILD_OUTPUT" | grep -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [ -n "$ERRORS" ]; then
    echo "$ERRORS"
fi

# Check if build was successful
if echo "$BUILD_OUTPUT" | grep -q "completed successfully" && [ -z "$ERRORS" ]; then
    echo "✅ Build successful"
    
    # Copy binary for testing
    if [ -f "$BUILD_DIR/hydrogen" ]; then
        cp "$BUILD_DIR/hydrogen" "$SOURCE_DIR/../hydrogen"
    fi
    
    # Run shutdown test
    echo "🔄 Running shutdown test..."
    if [ -f "$SOURCE_DIR/../tests/test_26_shutdown.sh" ]; then
        "$SOURCE_DIR/../tests/test_26_shutdown.sh" >/dev/null 2>&1 && echo "✅ Shutdown test passed" || echo "❌ Shutdown test failed"
    else
        echo "⚠️  Shutdown test not found"
    fi
    
    # Analyze unused files
    MAP_FILE="$BUILD_DIR/regular/regular.map"
    if [ -f "$MAP_FILE" ]; then
        echo "🔍 Checking for unused source files..."
        
        # Extract object files that are actually linked
        USED_OBJS=$(awk '/^LOAD.*\.o$/ {print $2}' "$MAP_FILE" | grep -v "^/usr" | sort -u)
        
        # Convert object files back to source file paths
        USED_SRCS=""
        for obj in $USED_OBJS; do
            # Extract the source path from the object file path
            # Object files are like: regular/src/config/config.o
            if [[ "$obj" == *"/src/"* ]]; then
                src_path=$(echo "$obj" | sed 's|^.*/src/|./|' | sed 's|\.o$|.c|')
                USED_SRCS="$USED_SRCS\n$src_path"
            fi
        done
        USED_SRCS=$(echo -e "$USED_SRCS" | grep -v "^$" | sort -u)
        
        # Find all source files relative to src directory  
        ALL_SRCS=$(find "$SOURCE_DIR/../src" -name "*.c" | sed "s|$SOURCE_DIR/../src/|./|g" | sort)
        
        # Read ignored files (already in ./ format)
        IGNORED_SRCS=""
        if [ -f "$SOURCE_DIR/../.trial-ignore" ]; then
            IGNORED_SRCS=$(grep -v '^#' "$SOURCE_DIR/../.trial-ignore" | grep -v '^$' | sort)
        fi
        
        # Find unused sources
        UNUSED_SRCS=$(comm -23 <(echo "$ALL_SRCS") <(echo "$USED_SRCS"))
        REPORTABLE_SRCS=$(comm -23 <(echo "$UNUSED_SRCS") <(echo "$IGNORED_SRCS"))
        
        if [ -z "$REPORTABLE_SRCS" ]; then
            echo "✅ No unexpected unused source files"
        else
            echo "⚠️  Unused source files:"
            echo "$REPORTABLE_SRCS" | while read -r file; do
                if [ -n "$file" ]; then
                    echo "  $file"
                fi
            done
        fi
    else
        echo "⚠️  Map file not found, skipping unused file analysis"
    fi
else
    echo "❌ Build failed"
    exit 1
fi
