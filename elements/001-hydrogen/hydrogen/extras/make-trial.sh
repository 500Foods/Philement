#!/bin/bash

# CHANGELOG
# 2025-07-15: Added Unity test compilation to catch errors in test code during trial builds

cd cmake && cmake -S . -B ../build --preset default >/dev/null 2>&1

# Build main project and check for errors
BUILD_OUTPUT=$(cmake --build ../build --preset default 2>&1)
ERRORS=$(echo "$BUILD_OUTPUT" | grep -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [ -n "$ERRORS" ]; then
    echo "$ERRORS"
    exit 1
fi

# Build Unity tests and check for errors
UNITY_BUILD_OUTPUT=$(cmake --build ../build --target unity_tests 2>&1)
UNITY_ERRORS=$(echo "$UNITY_BUILD_OUTPUT" | grep -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [ -n "$UNITY_ERRORS" ]; then
    echo "$UNITY_ERRORS"
    exit 1
fi

# Run trial build target
cmake --build ../build --preset trial
cd ..
