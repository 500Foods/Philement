#!/bin/bash
# =============================================================================
# WebSocket Connection Test Script
# =============================================================================
# This script tests WebSocket connectivity and connection maintenance.
# Configure the variables below and run: bash test-ws-connection.sh
# =============================================================================

# ---------------------------------------------------------------------------
# CONFIGURATION - Modify these variables to adjust test behavior
# ---------------------------------------------------------------------------

# Server URL (no trailing slash)
WS_URL="${WS_URL:-wss://lithium.philement.com/wss}"

# WebSocket key for authentication
WS_KEY="${WS_KEY:-ABCDEFGHIJKLMNOP}"

# WebSocket protocol
WS_PROTOCOL="${WS_PROTOCOL:-hydrogen}"

# Full URL with key (auto-generated)
FULL_URL="${WS_URL}?key=${WS_KEY}"

# Test modes (set to "true" or "false")
RUN_IMMEDIATE_TEST="true"      # Quick connect/disconnect test
RUN_DELAYED_TESTS="true"       # Tests with delays between connections
RUN_MAINTENANCE_TEST="true"    # Single connection kept alive

# Delay intervals in seconds (space-separated)
DELAY_INTERVALS="5 15 30 60 90 120"

# Number of sequential connections for immediate test
SEQUENTIAL_COUNT=10

# Single connection maintenance test duration (seconds)
MAINTENANCE_DURATION=180

# Single connection ping interval during maintenance test (seconds)
MAINTENANCE_PING_INTERVAL=30

# Request to send during tests (use 'keepalive' for lightweight, 'status' for full server info)
TEST_REQUEST='{"type":"keepalive"}'

# Verbose output (show server responses)
VERBOSE="false"

# ---------------------------------------------------------------------------
# END CONFIGURATION
# ---------------------------------------------------------------------------

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
PASSED=0
FAILED=0

print_header() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_result() {
    local name=$1
    local result=$2
    
    if [ "$result" = "PASS" ]; then
        echo -e "  ${GREEN}✓${NC} $name"
        PASSED=$((PASSED + 1))
    else
        echo -e "  ${RED}✗${NC} $name"
        FAILED=$((FAILED + 1))
    fi
}

test_connection() {
    local name=$1
    local timeout=$2
    
    local output
    local exit_code
    
    output=$(echo "$TEST_REQUEST" | timeout "$timeout" websocat --protocol "$WS_PROTOCOL" "$FULL_URL" 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        if [ "$VERBOSE" = "true" ]; then
            echo "    Response: ${output:0:100}..."
        fi
        print_result "$name" "PASS"
        return 0
    else
        print_result "$name" "FAIL (exit: $exit_code)"
        return 1
    fi
}

# Main script
echo -e "${YELLOW}"
echo "╔════════════════════════════════════════════════════════════╗"
echo "║         WebSocket Connection Test Script                   ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

echo "Configuration:"
echo "  URL: $FULL_URL"
echo "  Protocol: $WS_PROTOCOL"
echo ""

# Test 1: Immediate sequential connections
if [ "$RUN_IMMEDIATE_TEST" = "true" ]; then
    print_header "Test 1: Sequential Connections ($SEQUENTIAL_COUNT)"
    
    for i in $(seq 1 $SEQUENTIAL_COUNT); do
        test_connection "Connection $i" 10
    done
    
    echo ""
fi

# Test 2: Connections with delays
if [ "$RUN_DELAYED_TESTS" = "true" ]; then
    print_header "Test 2: Connections with Delays"
    
    for delay in $DELAY_INTERVALS; do
        echo "  Waiting ${delay}s..."
        sleep "$delay"
        test_connection "After ${delay}s delay" 10
    done
    
    echo ""
fi

# Test 3: Single connection maintenance
if [ "$RUN_MAINTENANCE_TEST" = "true" ]; then
    print_header "Test 3: Single Connection Maintenance (${MAINTENANCE_DURATION}s)"
    
    echo "  Opening connection and sending periodic pings..."
    
    # Use a FIFO for bidirectional communication
    FIFO=$(mktemp -u)
    mkfifo "$FIFO"
    trap "rm -f $FIFO" EXIT
    
    # Start websocat in background
    websocat --protocol "$WS_PROTOCOL" "$FULL_URL" < "$FIFO" > /tmp/ws_output_$$.txt 2>&1 &
    WS_PID=$!
    
    # Open FIFO for writing
    exec 3>"$FIFO"
    
    # Send initial request
    echo "$TEST_REQUEST" >&3
    echo -n "  0s: "
    sleep 1
    
    if kill -0 $WS_PID 2>/dev/null; then
        echo -e "${GREEN}Connected${NC}"
    else
        echo -e "${RED}Connection failed${NC}"
        FAILED=$((FAILED + 1))
        exec 3>&-
        wait $WS_PID 2>/dev/null
    fi
    
    # Send periodic pings
    MAINTENANCE_PINGS=$((MAINTENANCE_DURATION / MAINTENANCE_PING_INTERVAL))
    
    for i in $(seq 1 $MAINTENANCE_PINGS); do
        sleep "$MAINTENANCE_PING_INTERVAL"
        ELAPSED=$((i * MAINTENANCE_PING_INTERVAL))
        
        if kill -0 $WS_PID 2>/dev/null; then
            echo "$TEST_REQUEST" >&3 2>/dev/null
            if [ $? -eq 0 ]; then
                echo -n "  ${ELAPSED}s: "
                echo -e "${GREEN}Ping sent, alive${NC}"
            else
                echo "  ${ELAPSED}s: Send failed"
                break
            fi
        else
            echo -e "  ${ELAPSED}s: ${RED}Connection lost${NC}"
            FAILED=$((FAILED + 1))
            break
        fi
    done
    
    # Check if we completed the full duration
    if [ $ELAPSED -ge $MAINTENANCE_DURATION ]; then
        PASSED=$((PASSED + 1))
    fi
    
    # Close connection
    exec 3>&-
    wait $WS_PID 2>/dev/null
    
    echo ""
fi

# Summary
print_header "Test Summary"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi
