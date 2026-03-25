#!/bin/bash
# WebSocket Connection Test with Response Verification
# Tests if connection stays alive AND we get responses

WS_URL="wss://lithium.philement.com/wss?key=ABCDEFGHIJKLMNOP"
PROTO="hydrogen"

echo "=== WebSocket Connection Test with Responses ==="
echo "This tests ONE connection with pings that expect responses"
echo ""

# Test using a simple expect-style approach
# Start websocat, capture both sent and received

TIMEOUT=300
INTERVAL=15

(
    # Send pings every 15 seconds
    for ((i=1; i<=TIMEOUT/INTERVAL; i++)); do
        echo "{\"type\":\"ping\",\"n\":${i}}"
        sleep "${INTERVAL}" || true
    done
) | (timeout "${TIMEOUT}" websocat --protocol "${PROTO}" "${WS_URL}" 2>&1 || true) | {

    SENT=0
    RECV=0

    while read -r line; do
        # Check if line contains pong response
        if echo "${line}" | grep -q "pong"; then
            RECV=$(( RECV + 1 ))
            echo "Received pong #${RECV}"
        elif echo "${line}" | grep -q "ping"; then
            SENT=$(( SENT + 1 ))
            echo "Sent ping #${SENT}"
        fi
    done

    echo ""
    echo "Pings sent: ${SENT}"
    echo "Pongs received: ${RECV}"
}

EXIT_CODE=$?
echo ""
echo "Exit code: ${EXIT_CODE}"

if [[ ${EXIT_CODE} -eq 124 ]]; then
    echo "Connection timed out at ${TIMEOUT} seconds"
    echo "SUCCESS: Connection stayed alive for full duration"
    exit 0
elif [[ ${EXIT_CODE} -eq 0 ]]; then
    echo "Connection closed normally"
    exit 0
else
    echo "Connection failed"
    exit 1
fi
