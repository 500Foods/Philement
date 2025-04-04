#!/bin/bash
#
# Test environment variable handling for the Hydrogen payload system
# Validates that required payload environment variables are present and valid

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Create results directory
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/payload_test_${TIMESTAMP}.log"

# Start the test
EXIT_CODE=0
start_test "Payload Environment Variables Test" | tee -a "$RESULT_LOG"

# Test Case 1: Check Environment Variables Exist
print_header "Test Case 1: Environment Variables Present" | tee -a "$RESULT_LOG"

failed_checks=0
passed_checks=0

# Check PAYLOAD_KEY
if [ -n "${PAYLOAD_KEY}" ]; then
    print_info "✓ PAYLOAD_KEY is set" | tee -a "$RESULT_LOG"
    passed_checks=$((passed_checks + 1))
else
    print_warning "✗ PAYLOAD_KEY is not set" | tee -a "$RESULT_LOG"
    failed_checks=$((failed_checks + 1))
fi

# Check PAYLOAD_LOCK
if [ -n "${PAYLOAD_LOCK}" ]; then
    print_info "✓ PAYLOAD_LOCK is set" | tee -a "$RESULT_LOG"
    passed_checks=$((passed_checks + 1))
else
    print_warning "✗ PAYLOAD_LOCK is not set" | tee -a "$RESULT_LOG"
    failed_checks=$((failed_checks + 1))
fi

if [ $failed_checks -eq 0 ]; then
    print_result 0 "Environment variables present test PASSED" | tee -a "$RESULT_LOG"
else
    print_result 1 "Environment variables present test FAILED" | tee -a "$RESULT_LOG"
    EXIT_CODE=1
fi

# Test Case 2: Validate RSA Key Format
print_header "Test Case 2: RSA Key Format Validation" | tee -a "$RESULT_LOG"

failed_checks=0
passed_checks=0

if [ -n "${PAYLOAD_KEY}" ]; then
    # Create temporary file for key validation
    TEMP_KEY="/tmp/test_key.$$.pem"
    echo "${PAYLOAD_KEY}" | base64 -d > "$TEMP_KEY" 2>/dev/null
    
    # Check if it's a valid RSA private key
    if openssl rsa -in "$TEMP_KEY" -check -noout 2>/dev/null; then
        print_info "✓ PAYLOAD_KEY is a valid RSA private key" | tee -a "$RESULT_LOG"
        passed_checks=$((passed_checks + 1))
    else
        print_warning "✗ PAYLOAD_KEY is not a valid RSA private key" | tee -a "$RESULT_LOG"
        failed_checks=$((failed_checks + 1))
    fi
    
    # Clean up
    rm -f "$TEMP_KEY"
fi

if [ -n "${PAYLOAD_LOCK}" ]; then
    # Create temporary file for key validation
    TEMP_LOCK="/tmp/test_lock.$$.pem"
    echo "${PAYLOAD_LOCK}" | base64 -d > "$TEMP_LOCK" 2>/dev/null
    
    # Check if it's a valid RSA public key
    if openssl rsa -pubin -in "$TEMP_LOCK" -noout 2>/dev/null; then
        print_info "✓ PAYLOAD_LOCK is a valid RSA public key" | tee -a "$RESULT_LOG"
        passed_checks=$((passed_checks + 1))
    else
        print_warning "✗ PAYLOAD_LOCK is not a valid RSA public key" | tee -a "$RESULT_LOG"
        failed_checks=$((failed_checks + 1))
    fi
    
    # Clean up
    rm -f "$TEMP_LOCK"
fi

if [ $failed_checks -eq 0 ] && [ $passed_checks -gt 0 ]; then
    print_result 0 "RSA key format validation test PASSED" | tee -a "$RESULT_LOG"
else
    print_result 1 "RSA key format validation test FAILED" | tee -a "$RESULT_LOG"
    EXIT_CODE=1
fi

# Track subtest results
TOTAL_SUBTESTS=2  # Environment vars present, RSA key format
PASS_COUNT=0

# Check which subtests passed
if grep -q "Environment variables present test PASSED" "$RESULT_LOG"; then
    ((PASS_COUNT++))
fi
if grep -q "RSA key format validation test PASSED" "$RESULT_LOG"; then
    ((PASS_COUNT++))
fi

# Overall test result
if [ $EXIT_CODE -eq 0 ]; then
    print_result 0 "All payload environment variable tests PASSED" | tee -a "$RESULT_LOG"
else
    print_result 1 "Some payload environment variable tests FAILED" | tee -a "$RESULT_LOG"
fi

# Get test name from script name
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results for test_all.sh to pick up
export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "Payload Environment Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

print_info "Test results saved to: $(convert_to_relative_path "$RESULT_LOG")" | tee -a "$RESULT_LOG"
end_test $EXIT_CODE "Payload Environment Variables Test" | tee -a "$RESULT_LOG"