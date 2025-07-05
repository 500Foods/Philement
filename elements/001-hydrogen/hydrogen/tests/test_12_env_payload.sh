#!/bin/bash
#
# Test 12: Payload Environment Variables
# Tests environment variable handling for the payload system
# Validates that required payload environment variables are present and valid
#
# VERSION HISTORY
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, enhanced comments, added version tracking
# 1.0.0 - Initial version - Basic environment variable validation
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Only source tables.sh if not already loaded (check guard variable)
if [[ -z "$TABLES_SH_GUARD" ]] || ! command -v tables_render_from_json >/dev/null 2>&1; then
# shellcheck source=tests/lib/tables.sh
    source "$SCRIPT_DIR/lib/tables.sh"
    export TABLES_SOURCED=true
fi

# Only source log_output.sh if not already loaded (check guard variable)
if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
# shellcheck source=tests/lib/log_output.sh
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# Source other modular test libraries (always needed, not provided by test suite)
# shellcheck source=tests/lib/framework.sh
source "$SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/env_utils.sh
source "$SCRIPT_DIR/lib/env_utils.sh"

# Test configuration
TEST_NAME="Payload Environment Variables"
SCRIPT_VERSION="3.0.0"
EXIT_CODE=0
TOTAL_SUBTESTS=2
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Subtest: Environment Variables Present
next_subtest
print_subtest "Environment Variables Present"

test_env_vars_present() {
    local failed_checks=0
    local passed_checks=0

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
    evaluate_test_result "Environment variables present" "$failed_checks" "PASS_COUNT" "EXIT_CODE"
}

test_env_vars_present

# Subtest: RSA Key Format Validation
next_subtest
print_subtest "RSA Key Format Validation"

test_rsa_key_format() {
    local failed_checks=0
    local passed_checks=0

    # Validate PAYLOAD_KEY (private key)
    if [ -n "${PAYLOAD_KEY}" ]; then
        print_command "echo \"\$PAYLOAD_KEY\" | base64 -d | openssl rsa -check -noout"
        if validate_rsa_key "PAYLOAD_KEY" "${PAYLOAD_KEY}" "private"; then
            ((passed_checks++))
        else
            ((failed_checks++))
        fi
    fi

    # Validate PAYLOAD_LOCK (public key)
    if [ -n "${PAYLOAD_LOCK}" ]; then
        print_command "echo \"\$PAYLOAD_LOCK\" | base64 -d | openssl rsa -pubin -noout"
        if validate_rsa_key "PAYLOAD_LOCK" "${PAYLOAD_LOCK}" "public"; then
            ((passed_checks++))
        else
            ((failed_checks++))
        fi
    fi

    # Evaluate test result
    evaluate_test_result "RSA key format validation" "$failed_checks" "PASS_COUNT" "EXIT_CODE"
}

test_rsa_key_format

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test $EXIT_CODE $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi
