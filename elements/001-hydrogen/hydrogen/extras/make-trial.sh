#!/bin/bash

cd cmake && cmake -S . -B ../build --preset default >/dev/null 2>&1
BUILD_OUTPUT=$(cmake --build ../build --preset default 2>&1)
ERRORS=$(echo "$BUILD_OUTPUT" | grep -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [ -n "$ERRORS" ]; then
    echo "$ERRORS"
    exit 1
fi
cmake --build ../build --preset trial
cd ..
