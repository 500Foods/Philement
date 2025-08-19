#!/usr/bin/env bash

# CHANGELOG
# 2025-07-15: Added Unity test compilation to catch errors in test code during trial builds

echo "$(date +%H:%M:%S.%3N || true) - Cleaning Build Directory"
rm -rf build/*

echo "$(date +%H:%M:%S.%3N || true) - Configuring CMake"
cd cmake && cmake -S . -B ../build --preset default >/dev/null 2>&1

echo "$(date +%H:%M:%S.%3N || true) - Default Build"

# Build main project and check for errors
BUILD_OUTPUT=$(cmake --build ../build --preset default 2>&1)
ERRORS=$(echo "${BUILD_OUTPUT}" | grep -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [[ -n "${ERRORS}" ]]; then
    echo "${ERRORS}"
    exit 1
fi

echo "$(date +%H:%M:%S.%3N || true) - Unity Build"

# Build Unity tests and check for errors
UNITY_BUILD_OUTPUT=$(cmake --build ../build --target unity_tests 2>&1)
UNITY_ERRORS=$(echo "${UNITY_BUILD_OUTPUT}" | grep -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [[ -n "${UNITY_ERRORS}" ]]; then
    echo "${UNITY_ERRORS}"
    exit 1
fi

echo "$(date +%H:%M:%S.%3N || true) - Trial Build"
# Run trial build target
cmake --build ../build --preset trial -- --quiet
