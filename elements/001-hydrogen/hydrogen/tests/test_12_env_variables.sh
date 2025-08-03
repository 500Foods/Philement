#!/bin/bash

# Test: Env Var Substitution
# Tests that configuration values can be provided via environment variables

# CHANGELOG
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
TEST_VERSION="4.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Configuration file paths
CONFIG_FILE="${CONFIG_DIR}/hydrogen_test_12.json"

# Timeouts and limits
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity

print_subtest "Locate Hydrogen Binary"

# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "${PROJECT_DIR}" "HYDROGEN_BIN"; then
    print_message "Using Hydrogen binary: $(basename "${HYDROGEN_BIN}")"
    print_result 0 "Hydrogen binary found and validated"
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

print_subtest "Create Output Directories"

if setup_output_directories "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOG_FILE}" "${DIAG_TEST_DIR}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test basic environment variables with lifecycle test
set_basic_test_environment
config_name="basic_env_vars"
run_lifecycle_test "${CONFIG_FILE}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Test missing environment variables fallback
reset_environment_variables
config_name="missing_env_vars"
run_lifecycle_test "${CONFIG_FILE}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Test environment variable type conversion
set_type_conversion_environment
config_name="type_conversion"
run_lifecycle_test "${CONFIG_FILE}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Test environment variable validation
set_validation_test_environment
config_name="env_validation"
run_lifecycle_test "${CONFIG_FILE}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Reset Environment Variables"

reset_environment_variables

print_result 0 "All environment variables reset"
((PASS_COUNT++))

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
