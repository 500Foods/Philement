#!/bin/bash
#
# About this Script
#
# Hydrogen Payload Environment Variables Test
# Test environment variable handling for the Hydrogen payload system
# Validates that required payload environment variables are present and valid
#
NAME_SCRIPT="Hydrogen Payload Environment Variables Test"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, enhanced comments, added version tracking
# 1.0.0 - Initial version - Basic environment variable validation

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Create results directory
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/payload_test_${TIMESTAMP}.log"

# Global test tracking variables
EXIT_CODE=0
TOTAL_SUBTESTS=2  # Environment vars present, RSA key format
PASS_COUNT=0

# Function: Check if environment variable is set and non-empty
# Parameters: $1 - variable name, $2 - variable value
# Returns: 0 if set and non-empty, 1 otherwise
check_env_var() {
    local var_name="$1"
    local var_value="$2"
    
    if [ -n "$var_value" ]; then
        print_info "✓ $var_name is set" | tee -a "$RESULT_LOG"
        return 0
    else
        print_warning "✗ $var_name is not set" | tee -a "$RESULT_LOG"
        return 1
    fi
}

# Function: Validate RSA key format
# Parameters: $1 - key name, $2 - base64 encoded key, $3 - key type (private|public)
# Returns: 0 if valid, 1 otherwise
validate_rsa_key() {
    local key_name="$1"
    local key_data="$2"
    local key_type="$3"
    local temp_key="/tmp/test_${key_type}_key.$$.pem"
    local openssl_cmd
    
    # Decode base64 key to temporary file (ignore base64 warnings, focus on output)
    echo "$key_data" | base64 -d > "$temp_key" 2>/dev/null
    
    # Check if the decoded file exists and has content
    if [ ! -s "$temp_key" ]; then
        print_warning "✗ $key_name failed base64 decode - no output generated" | tee -a "$RESULT_LOG"
        rm -f "$temp_key"
        return 1
    fi
    
    # Set appropriate OpenSSL command based on key type
    case "$key_type" in
        "private")
            openssl_cmd="openssl rsa -in $temp_key -check -noout"
            ;;
        "public")
            openssl_cmd="openssl rsa -pubin -in $temp_key -noout"
            ;;
        *)
            print_warning "✗ Unknown key type: $key_type" | tee -a "$RESULT_LOG"
            rm -f "$temp_key"
            return 1
            ;;
    esac
    
    # Validate key format - this is the real test
    if $openssl_cmd 2>/dev/null; then
        print_info "✓ $key_name is a valid RSA $key_type key" | tee -a "$RESULT_LOG"
        rm -f "$temp_key"
        return 0
    else
        print_warning "✗ $key_name is not a valid RSA $key_type key" | tee -a "$RESULT_LOG"
        rm -f "$temp_key"
        return 1
    fi
}

# Function: Run environment variables presence test
# Returns: 0 if all variables present, 1 otherwise
test_env_vars_present() {
    local failed_checks=0
    local passed_checks=0
    
    print_header "Test Case 1: Environment Variables Present" | tee -a "$RESULT_LOG"
    
    # Check PAYLOAD_KEY
    if check_env_var "PAYLOAD_KEY" "${PAYLOAD_KEY}"; then
        ((passed_checks++))
    else
        ((failed_checks++))
    fi
    
    # Check PAYLOAD_LOCK
    if check_env_var "PAYLOAD_LOCK" "${PAYLOAD_LOCK}"; then
        ((passed_checks++))
    else
        ((failed_checks++))
    fi
    
    # Evaluate test result
    if [ $failed_checks -eq 0 ]; then
        print_result 0 "Environment variables present test PASSED" | tee -a "$RESULT_LOG"
        return 0
    else
        print_result 1 "Environment variables present test FAILED" | tee -a "$RESULT_LOG"
        return 1
    fi
}

# Function: Run RSA key format validation test
# Returns: 0 if all keys valid, 1 otherwise
test_rsa_key_format() {
    local failed_checks=0
    local passed_checks=0
    
    print_header "Test Case 2: RSA Key Format Validation" | tee -a "$RESULT_LOG"
    
    # Validate PAYLOAD_KEY (private key)
    if [ -n "${PAYLOAD_KEY}" ]; then
        if validate_rsa_key "PAYLOAD_KEY" "${PAYLOAD_KEY}" "private"; then
            ((passed_checks++))
        else
            ((failed_checks++))
        fi
    fi
    
    # Validate PAYLOAD_LOCK (public key)
    if [ -n "${PAYLOAD_LOCK}" ]; then
        if validate_rsa_key "PAYLOAD_LOCK" "${PAYLOAD_LOCK}" "public"; then
            ((passed_checks++))
        else
            ((failed_checks++))
        fi
    fi
    
    # Evaluate test result
    if [ $failed_checks -eq 0 ] && [ $passed_checks -gt 0 ]; then
        print_result 0 "RSA key format validation test PASSED" | tee -a "$RESULT_LOG"
        return 0
    else
        print_result 1 "RSA key format validation test FAILED" | tee -a "$RESULT_LOG"
        return 1
    fi
}

# Function: Update pass count based on test results
update_pass_count() {
    # Check which subtests passed by examining the log
    if grep -q "Environment variables present test PASSED" "$RESULT_LOG"; then
        ((PASS_COUNT++))
    fi
    if grep -q "RSA key format validation test PASSED" "$RESULT_LOG"; then
        ((PASS_COUNT++))
    fi
}

# Main test execution
main() {
    # Start the test
    start_test "Payload Environment Variables Test" | tee -a "$RESULT_LOG"
    
    # Run Test Case 1: Environment Variables Present
    if ! test_env_vars_present; then
        EXIT_CODE=1
    fi
    
    # Run Test Case 2: RSA Key Format Validation
    if ! test_rsa_key_format; then
        EXIT_CODE=1
    fi
    
    # Update pass count based on results
    update_pass_count
    
    # Overall test result
    if [ $EXIT_CODE -eq 0 ]; then
        print_result 0 "All payload environment variable tests PASSED" | tee -a "$RESULT_LOG"
    else
        print_result 1 "Some payload environment variable tests FAILED" | tee -a "$RESULT_LOG"
    fi
    
    # Get test name from script name for result export
    TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')
    
    # Export subtest results for test_all.sh to pick up
    export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT
    
    # Log subtest results summary
    print_info "Payload Environment Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"
    
    # Display results location and end test
    print_info "Test results saved to: $(convert_to_relative_path "$RESULT_LOG")" | tee -a "$RESULT_LOG"
    end_test $EXIT_CODE "Payload Environment Variables Test" | tee -a "$RESULT_LOG"
    
    exit $EXIT_CODE
}

# Execute main function
main "$@"
