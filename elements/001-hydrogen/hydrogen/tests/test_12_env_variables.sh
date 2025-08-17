#!/usr/bin/env bash

# Test: Env Var Substitution
# Tests that configuration values can be provided via environment variables

# FUNCTIONS
# reset_environment_variables()

# CHANGELOG
# 4.1.0 - 2025-08-07 - Clean up of logging files, moved env vars back to here from env_uitls
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.1.0 - 2025-07-06 - Added missing shellcheck justificaitons
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries and match test 15 structure
# 2.1.0 - 2025-07-02 - Migrated to use modular lib/ scripts
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic environment variable testing functionality

# Test configuration
TEST_NAME="Env Var Substitution"
TEST_ABBR="VAR"
TEST_NUMBER="12"
TEST_VERSION="4.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Configuration file paths
CONFIG_FILE="${CONFIG_DIR}/hydrogen_test_12_vars.json"

# Timeouts and limits
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity

# Function to reset all environment variables used in testing
reset_environment_variables() {
    unset H_SERVER_NAME H_WEB_ENABLED H_WEB_PORT H_UPLOAD_DIR
    unset H_MAX_UPLOAD_SIZE H_SHUTDOWN_WAIT H_MAX_QUEUE_BLOCKS H_DEFAULT_QUEUE_CAPACITY
    unset H_MEMORY_WARNING H_LOAD_WARNING H_PRINT_QUEUE_ENABLED H_CONSOLE_LOG_LEVEL
    unset H_DEVICE_ID H_FRIENDLY_NAME
    
    print_message "All Hydrogen environment variables have been unset"
}

print_subtest "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_result 0 "Hydrogen binary found and validated: ${HYDROGEN_BIN_BASE}"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "Validate Configuration File"

if validate_config_file "${CONFIG_FILE}"; then
    print_message "Using config: $(convert_to_relative_path "${CONFIG_FILE}" || true)"
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test basic environment variables with lifecycle test
reset_environment_variables
export H_SERVER_NAME="hydrogen-env-test"
export H_WEB_ENABLED="true"
export H_WEB_PORT="9000"
export H_UPLOAD_DIR="/tmp/hydrogen_env_test_uploads"
export H_MAX_UPLOAD_SIZE="1048576"
export H_SHUTDOWN_WAIT="3"
export H_MAX_QUEUE_BLOCKS="64"
export H_DEFAULT_QUEUE_CAPACITY="512"
export H_MEMORY_WARNING="95"
export H_LOAD_WARNING="7.5"
export H_PRINT_QUEUE_ENABLED="false"
export H_CONSOLE_LOG_LEVEL="1"
export H_DEVICE_ID="hydrogen-env-test"
export H_FRIENDLY_NAME="Hydrogen Environment Test"
config_name="basic_env_vars"
LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${config_name}.log"
   
print_message "Basic environment variables for Hydrogen test have been set"
run_lifecycle_test "${CONFIG_FILE}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Test missing environment variables fallback
reset_environment_variables
config_name="missing_env_vars"
LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${config_name}.log"

print_message "Environment variables have been removed"
run_lifecycle_test "${CONFIG_FILE}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Test environment variable type conversion
reset_environment_variables
export H_SERVER_NAME="hydrogen-type-test"
export H_WEB_ENABLED="TRUE"  # uppercase, should convert to boolean true
export H_WEB_PORT="8080"     # string that should convert to number
export H_MEMORY_WARNING="75" # string that should convert to number
export H_LOAD_WARNING="2.5"  # string that should convert to float
export H_PRINT_QUEUE_ENABLED="FALSE" # uppercase, should convert to boolean false
export H_CONSOLE_LOG_LEVEL="0"
export H_SHUTDOWN_WAIT="5"
export H_MAX_QUEUE_BLOCKS="128"
export H_DEFAULT_QUEUE_CAPACITY="1024"
config_name="type_conversion"
LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${config_name}.log"

print_message "Type conversion environment variables for Hydrogen test have been set"
run_lifecycle_test "${CONFIG_FILE}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Test environment variable validation
reset_environment_variables
export H_SERVER_NAME="hydrogen-validation-test"
export H_WEB_ENABLED="yes"   # non-standard boolean value
export H_WEB_PORT="invalid"  # invalid port number
export H_MEMORY_WARNING="150" # invalid percentage (>100)
export H_LOAD_WARNING="-1.0"  # invalid negative load
export H_PRINT_QUEUE_ENABLED="maybe" # invalid boolean
export H_CONSOLE_LOG_LEVEL="debug"   # string instead of number
export H_SHUTDOWN_WAIT="0"    # edge case: zero timeout
export H_MAX_QUEUE_BLOCKS="0" # edge case: zero blocks
config_name="env_validation"
LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${config_name}.log"

print_message "Validation environment variables for Hydrogen test have been set"
run_lifecycle_test "${CONFIG_FILE}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Reset Environment Variables"
reset_environment_variables
print_result 0 "All environment variables reset"
((PASS_COUNT++))

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
