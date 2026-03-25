#!/bin/bash
# WebSocket Connection Stability Test with Long Delays
# Tests if connections remain stable through various idle periods

WS_URL="wss://lithium.philement.com/wss?key=ABCDEFGHIJKLMNOP"
PROTO="hydrogen"
PASSED=0
FAILED=0

echo "=== WebSocket Long-Delay Stability Test ==="
echo "Testing connections with increasing delays..."
echo ""

# Connection 1: Immediate
echo -n "Connection 1 (immediate): "
if echo '{"type":"ping"}' | timeout 10 websocat -1 --protocol "${PROTO}" "${WS_URL}" >/dev/null 2>&1; then
    echo "OK"; PASSED=$(( PASSED + 1 ))
else
    echo "FAILED"; FAILED=$(( FAILED + 1 ))
fi

# Connection 2: After 30s delay
echo "Waiting 30 seconds..."
sleep 30
echo -n "Connection 2 (after 30s): "
if echo '{"type":"ping"}' | timeout 10 websocat -1 --protocol "${PROTO}" "${WS_URL}" >/dev/null 2>&1; then
    echo "OK"; PASSED=$(( PASSED + 1 ))
else
    echo "FAILED"; FAILED=$(( FAILED + 1 ))
fi

# Connection 3: After 60s delay
echo "Waiting 60 seconds..."
sleep 60
echo -n "Connection 3 (after 60s): "
if echo '{"type":"ping"}' | timeout 10 websocat -1 --protocol "${PROTO}" "${WS_URL}" >/dev/null 2>&1; then
    echo "OK"; PASSED=$(( PASSED + 1 ))
else
    echo "FAILED"; FAILED=$(( FAILED + 1 ))
fi

# Connection 4: After 90s delay
echo "Waiting 90 seconds..."
sleep 90
echo -n "Connection 4 (after 90s): "
if echo '{"type":"ping"}' | timeout 10 websocat -1 --protocol "${PROTO}" "${WS_URL}" >/dev/null 2>&1; then
    echo "OK"; PASSED=$(( PASSED + 1 ))
else
    echo "FAILED"; FAILED=$(( FAILED + 1 ))
fi

# Connection 5: After 120s delay
echo "Waiting 120 seconds..."
sleep 120
echo -n "Connection 5 (after 120s): "
if echo '{"type":"ping"}' | timeout 10 websocat -1 --protocol "${PROTO}" "${WS_URL}" >/dev/null 2>&1; then
    echo "OK"; PASSED=$(( PASSED + 1 ))
else
    echo "FAILED"; FAILED=$(( FAILED + 1 ))
fi

echo ""
echo "=== Results ==="
echo "Passed: ${PASSED}/5"
echo "Failed: ${FAILED}/5"

if [[ ${FAILED} -eq 0 ]]; then
    echo "SUCCESS: All connections succeeded with long delays"
    exit 0
else
    echo "FAILURE: Some connections failed"
    exit 1
fi
