#!/bin/bash
#
# Test environment variable handling for the Hydrogen payload system
# This script validates the payload encryption/decryption system and its environment variables

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Configuration
# Prefer release build if available, fallback to standard build
if [ -f "$HYDROGEN_DIR/hydrogen_release" ]; then
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen_release"
    print_info "Using release build for testing"
else
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen"
    print_info "Using standard build"
fi

CONFIG_FILE=$(get_config_path "hydrogen_test_json.json")
if [ ! -f "$CONFIG_FILE" ]; then
    print_result 1 "Test config file not found: $CONFIG_FILE"
    exit 1
fi

# Create output directories
RESULTS_DIR="$SCRIPT_DIR/results"
DIAG_DIR="$SCRIPT_DIR/diagnostics"
mkdir -p "$RESULTS_DIR" "$DIAG_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/payload_test_${TIMESTAMP}.log"
TEST_LOG_FILE="$SCRIPT_DIR/logs/hydrogen_env_test.log"

# Function to check for strings in log file
check_log_contains() {
    local search_string="$1"
    local log_file="$2"
    local description="$3"
    
    if grep -q "$search_string" "$log_file"; then
        print_info "✓ $description" | tee -a "$RESULT_LOG"
        return 0
    else
        print_warning "✗ $description" | tee -a "$RESULT_LOG"
        return 1
    fi
}

# Function to generate test keys
generate_test_keys() {
    local private_key_file="/tmp/test_private.pem"
    local public_key_file="/tmp/test_public.pem"
    
    print_info "Generating test RSA keys..." | tee -a "$RESULT_LOG"
    openssl genrsa -out "$private_key_file" 2048 2>/dev/null
    openssl rsa -in "$private_key_file" -pubout -out "$public_key_file" 2>/dev/null
    
    export PAYLOAD_KEY=$(cat "$private_key_file" | base64 -w 0)
    export PAYLOAD_LOCK=$(cat "$public_key_file" | base64 -w 0)
    
    rm -f "$private_key_file" "$public_key_file"
}

# Handle cleanup on script termination
cleanup() {
    # Kill hydrogen process if still running
    if [ ! -z "$HYDROGEN_PID" ] && ps -p $HYDROGEN_PID > /dev/null 2>&1; then
        echo "Cleaning up Hydrogen (PID $HYDROGEN_PID)..."
        kill -SIGINT $HYDROGEN_PID 2>/dev/null || kill -9 $HYDROGEN_PID 2>/dev/null
    fi
    
    # Reset environment variables
    unset PAYLOAD_KEY
    unset PAYLOAD_LOCK
    
    # Clean up temporary files
    rm -f /tmp/test_private.pem /tmp/test_public.pem
    
    # Save test log if we've started writing it
    if [ -n "$RESULT_LOG" ] && [ -f "$RESULT_LOG" ]; then
        echo "Saving test results to $(convert_to_relative_path "$RESULT_LOG")"
    fi
    
    exit $EXIT_CODE
}

# Set up trap for clean termination
trap cleanup SIGINT SIGTERM EXIT

# Start the test
EXIT_CODE=0
start_test "Payload Environment Variables Test" | tee -a "$RESULT_LOG"
print_info "Testing payload system environment variables" | tee -a "$RESULT_LOG"
print_info "Using config: $(convert_to_relative_path "$CONFIG_FILE")" | tee -a "$RESULT_LOG"

# Clean up any existing log file
mkdir -p "$(dirname "$TEST_LOG_FILE")"
rm -f "$TEST_LOG_FILE"

# Make sure no Hydrogen instances are running
print_info "Ensuring no Hydrogen instances are running..." | tee -a "$RESULT_LOG"
pkill -f hydrogen || true
sleep 2

# Test Case 1: Missing Environment Variables
print_header "Test Case 1: Missing Environment Variables" | tee -a "$RESULT_LOG"

# Unset both variables
unset PAYLOAD_KEY
unset PAYLOAD_LOCK

print_info "Starting server without payload keys..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$TEST_LOG_FILE" 2>&1 &
HYDROGEN_PID=$!

# Wait for server to initialize
sleep 5

# Check if server is running
if ! ps -p $HYDROGEN_PID > /dev/null; then
    print_result 1 "Hydrogen failed to start without payload keys" | tee -a "$RESULT_LOG"
    cat "$TEST_LOG_FILE" | tee -a "$RESULT_LOG"
    EXIT_CODE=1
else
    print_info "Server started successfully without payload keys" | tee -a "$RESULT_LOG"
    
    # Check for expected warning messages
    failed_checks=0
    passed_checks=0
    
    if check_log_contains "No valid PAYLOAD_KEY configuration" "$TEST_LOG_FILE" "Missing PAYLOAD_KEY warning detected"; then
        passed_checks=$((passed_checks + 1))
    else
        failed_checks=$((failed_checks + 1))
    fi
    
    if check_log_contains "Cannot decrypt payload" "$TEST_LOG_FILE" "Payload decryption failure detected"; then
        passed_checks=$((passed_checks + 1))
    else
        failed_checks=$((failed_checks + 1))
    fi
    
    # Stop server
    kill -SIGINT $HYDROGEN_PID
    sleep 3
    
    if [ $failed_checks -eq 0 ]; then
        print_result 0 "Missing environment variables test PASSED" | tee -a "$RESULT_LOG"
    else
        print_result 1 "Missing environment variables test FAILED" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
fi

# Test Case 2: Invalid Environment Variables
print_header "Test Case 2: Invalid Environment Variables" | tee -a "$RESULT_LOG"

# Clean up log file
rm -f "$TEST_LOG_FILE"

# Set invalid values
export PAYLOAD_KEY="invalid-base64-data"
export PAYLOAD_LOCK="invalid-base64-data"

print_info "Starting server with invalid payload keys..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$TEST_LOG_FILE" 2>&1 &
HYDROGEN_PID=$!

# Wait for server to initialize
sleep 5

# Check if server is running
if ! ps -p $HYDROGEN_PID > /dev/null; then
    print_result 1 "Hydrogen failed to start with invalid payload keys" | tee -a "$RESULT_LOG"
    cat "$TEST_LOG_FILE" | tee -a "$RESULT_LOG"
    EXIT_CODE=1
else
    print_info "Server started successfully with invalid keys" | tee -a "$RESULT_LOG"
    
    failed_checks=0
    passed_checks=0
    
    if check_log_contains "Failed to load private key" "$TEST_LOG_FILE" "Invalid key format detected"; then
        passed_checks=$((passed_checks + 1))
    else
        failed_checks=$((failed_checks + 1))
    fi
    
    # Stop server
    kill -SIGINT $HYDROGEN_PID
    sleep 3
    
    if [ $failed_checks -eq 0 ]; then
        print_result 0 "Invalid environment variables test PASSED" | tee -a "$RESULT_LOG"
    else
        print_result 1 "Invalid environment variables test FAILED" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
fi

# Test Case 3: Valid Environment Variables
print_header "Test Case 3: Valid Environment Variables" | tee -a "$RESULT_LOG"

# Clean up log file
rm -f "$TEST_LOG_FILE"

# Generate valid test keys
generate_test_keys

print_info "Starting server with valid payload keys..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$TEST_LOG_FILE" 2>&1 &
HYDROGEN_PID=$!

# Wait for server to initialize
sleep 5

# Check if server is running
if ! ps -p $HYDROGEN_PID > /dev/null; then
    print_result 1 "Hydrogen failed to start with valid payload keys" | tee -a "$RESULT_LOG"
    cat "$TEST_LOG_FILE" | tee -a "$RESULT_LOG"
    EXIT_CODE=1
else
    print_info "Server started successfully with valid keys" | tee -a "$RESULT_LOG"
    
    failed_checks=0
    passed_checks=0
    
    if check_log_contains "Payload decrypted successfully" "$TEST_LOG_FILE" "Payload decryption success"; then
        passed_checks=$((passed_checks + 1))
    else
        failed_checks=$((failed_checks + 1))
    fi
    
    # Test Swagger UI accessibility
    WEB_PORT=$(extract_web_server_port "$CONFIG_FILE")
    sleep 2  # Give the server time to fully initialize
    
    if curl -s "http://localhost:${WEB_PORT}/swagger/" | grep -q "swagger-ui"; then
        print_info "✓ Swagger UI is accessible" | tee -a "$RESULT_LOG"
        passed_checks=$((passed_checks + 1))
    else
        print_warning "✗ Swagger UI is not accessible" | tee -a "$RESULT_LOG"
        failed_checks=$((failed_checks + 1))
    fi
    
    # Stop server
    kill -SIGINT $HYDROGEN_PID
    sleep 3
    
    if [ $failed_checks -eq 0 ]; then
        print_result 0 "Valid environment variables test PASSED" | tee -a "$RESULT_LOG"
    else
        print_result 1 "Valid environment variables test FAILED" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
fi

# Test Case 4: Payload Generation
print_header "Test Case 4: Payload Generation" | tee -a "$RESULT_LOG"

# Clean up log file
rm -f "$TEST_LOG_FILE"

# Generate new test keys
generate_test_keys

print_info "Testing payload generation..." | tee -a "$RESULT_LOG"

# Try to generate payload
cd "$HYDROGEN_DIR/payload" && ./payload-generate.sh > "$TEST_LOG_FILE" 2>&1
GENERATE_RESULT=$?

if [ $GENERATE_RESULT -eq 0 ] && [ -f "$HYDROGEN_DIR/payload/payload.tar.br.enc" ]; then
    print_info "✓ Payload generation successful" | tee -a "$RESULT_LOG"
    passed_checks=$((passed_checks + 1))
else
    print_warning "✗ Payload generation failed" | tee -a "$RESULT_LOG"
    failed_checks=$((failed_checks + 1))
    EXIT_CODE=1
fi

# Track subtest results
TOTAL_SUBTESTS=4  # Missing vars, Invalid vars, Valid vars, Payload generation
PASS_COUNT=0

# Check which subtests passed
if grep -q "Missing environment variables test PASSED" "$RESULT_LOG"; then
    ((PASS_COUNT++))
fi
if grep -q "Invalid environment variables test PASSED" "$RESULT_LOG"; then
    ((PASS_COUNT++))
fi
if grep -q "Valid environment variables test PASSED" "$RESULT_LOG"; then
    ((PASS_COUNT++))
fi
if [ $GENERATE_RESULT -eq 0 ]; then
    ((PASS_COUNT++))
fi

# Overall test result
if [ $EXIT_CODE -eq 0 ]; then
    print_result 0 "All payload environment variable tests PASSED" | tee -a "$RESULT_LOG"
else
    print_result 1 "Some payload environment variable tests FAILED" | tee -a "$RESULT_LOG"
fi

# Export subtest results for test_all.sh to pick up
export_subtest_results $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "Payload Environment Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

print_info "Test results saved to: $(convert_to_relative_path "$RESULT_LOG")" | tee -a "$RESULT_LOG"
end_test $EXIT_CODE "Payload Environment Variables Test" | tee -a "$RESULT_LOG"
# Let the cleanup() function handle the exit via the EXIT trap