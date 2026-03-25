#!/bin/bash
# WebSocket Connection Stress Test
# Tests multiple sequential connections to verify server stability

WS_URL="wss://lithium.philement.com/wss?key=ABCDEFGHIJKLMNOP"
PROTO="hydrogen"
PASSED=0
FAILED=0

echo "=== WebSocket Connection Stress Test ==="
echo "Testing 10 sequential connections..."
echo ""

for i in $(seq 1 10); do
    echo -n "Connection ${i}: "
    echo '{"type":"ping"}' | timeout 5 websocat -1 --protocol "${PROTO}" "${WS_URL}" 2>&1
    EXIT=$?

    if [[ ${EXIT} -eq 0 ]]; then
        echo "OK"
        PASSED=$(( PASSED + 1 ))
    else
        echo "FAILED (exit ${EXIT})"
        FAILED=$(( FAILED + 1 ))
    fi

    sleep 1
done

echo ""
echo "=== Results ==="
echo "Passed: ${PASSED}"
echo "Failed: ${FAILED}"

if [[ ${FAILED} -eq 0 ]]; then
    echo "SUCCESS: All connections succeeded"
    exit 0
else
    echo "FAILURE: Some connections failed"
    exit 1
fi
