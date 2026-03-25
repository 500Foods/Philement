#!/bin/bash
# WebSocket Single Connection Test - keeps ONE connection alive
# Sends pings every 30 seconds while maintaining the connection

WS_URL="wss://lithium.philement.com/wss?key=ABCDEFGHIJKLMNOP"
PROTO="hydrogen"

echo "=== WebSocket Single Connection Test ==="
echo "Opening connection and sending periodic pings..."
echo "Watch server logs for PING activity"
echo ""

START_TIME=$(date +%s)
TIMEOUT=300  # 5 minutes
INTERVAL=30   # 30 seconds between pings

# Use a FIFO to send periodic messages
FIFO=$(mktemp -u)
mkfifo "${FIFO}"
trap 'rm -f "${FIFO}"' EXIT

# Start websocat reading from FIFO
websocat --protocol "${PROTO}" "${WS_URL}" < "${FIFO}" 2>&1 &
WS_PID=$!

# Open FIFO for writing
exec 3>"${FIFO}"

# Send initial ping
echo '{"type":"ping"}' >&3
echo "Ping sent at 0s"

# Monitor connection and send periodic pings
ELAPSED=0
while [[ ${ELAPSED} -lt ${TIMEOUT} ]]; do
    sleep "${INTERVAL}"
    ELAPSED=$(( ELAPSED + INTERVAL ))

    # Check if websocat is still running
    if ! kill -0 "${WS_PID}" 2>/dev/null; then
        echo "Connection lost at ${ELAPSED}s"
        break
    fi

    # Send ping
    if ! echo '{"type":"ping"}' >&3 2>/dev/null; then
        echo "Send failed at ${ELAPSED}s - connection closed"
        break
    fi

    echo "Ping sent at ${ELAPSED}s - connection alive"
done

# Close the FIFO
exec 3>&-

# Wait for websocat to finish
wait "${WS_PID}" 2>/dev/null
WS_EXIT=$?

END_TIME=$(date +%s)
TOTAL=$(( END_TIME - START_TIME ))

echo ""
echo "=== Results ==="
echo "Duration: ${TOTAL}s"
echo "Exit code: ${WS_EXIT}"

if [[ ${TOTAL} -ge ${TIMEOUT} ]]; then
    echo "SUCCESS: Connection maintained for ${TIMEOUT}s"
    exit 0
else
    echo "Connection ended early at ${TOTAL}s"
    exit 1
fi
