#!/usr/bin/env bash

# Environment Utilities Library
# Provides functions for environment variable handling and validation

# LIBRARY FUNCTIONS
# check_env_var()
# validate_rsa_key()
# get_config_path()
# validate_websocket_key()
# convert_to_relative_path()

# CHANGELOG
# 2.0.0 - 2025-12-05 - Added HYDROGEN_ROOT and HELIUM_ROOT environment variable checks
# 1.2.2 - 2025-08-08 - Removeed validate_config_files
# 1.2.1 - 2025-08-03 - Removed extraneous command -v calls
# 1.2.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 1.1.0 - 2025-07-02 - Updated with additional functions for test_35_env_variables.sh migration
# 1.0.0 - 2025-07-02 - Initial creation for test_12_env_payload.sh migration

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${ENV_UTILS_GUARD:-}" ]] && return 0
export ENV_UTILS_GUARD="true"

# Library metadata
ENV_UTILS_NAME="Environment Utilities Library"
ENV_UTILS_VERSION="2.0.0"
# shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${ENV_UTILS_NAME} ${ENV_UTILS_VERSION}" "info"

# Function: Check if environment variable is set and non-empty
# Parameters: $1 - variable name
# Returns: 0 if set and non-empty, 1 otherwise
check_env_var() {
    local var_name="$1"
    local var_value="${!var_name}"
    
    if [[ -n "${var_value}" ]]; then
        local display_value="${var_value}"
        # Check if variable name contains sensitive keywords and value length > 20
        if [[ "${var_name}" =~ PASS|LOCK|KEY|JWT|TOKEN|SECRET ]] && [[ ${#var_value} -gt 20 ]]; then
            display_value="${var_value:0:20}..."
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${var_name} is set to: ${display_value}"
        return 0
    else
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${var_name} is not set or empty"
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
    echo "${key_data}" | base64 -d > "${temp_key}" 2>/dev/null
    
    # Check if the decoded file exists and has content
    if [[ ! -s "${temp_key}" ]]; then
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${key_name} failed base64 decode - no output generated"
        rm -f "${temp_key}"
        return 1
    fi
    
    # Set appropriate OpenSSL command based on key type
    case "${key_type}" in
        "private")
            openssl_cmd="openssl rsa -in ${temp_key} -check -noout"
            ;;
        "public")
            openssl_cmd="openssl pkey -pubin -in ${temp_key} -check -noout"
            ;;
        *)
            print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Unknown key type: ${key_type}"
            rm -f "${temp_key}"
            return 1
            ;;
    esac
    
    # Validate key format - this is the real test
    if ${openssl_cmd} >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${key_name} is a valid RSA ${key_type} key"
        rm -f "${temp_key}"
        return 0
    else
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${key_name} is not a valid RSA ${key_type} key"
        rm -f "${temp_key}"
        return 1
    fi
}

# Function to get the full path to a configuration file
get_config_path() {
    local config_file="$1"
    local config_path="${HYDROGEN_ROOT}/tests/configs/${config_file}"
    echo "${config_path}"
}

# Function: Validate WebSocket key format
# Parameters: $1 - key name, $2 - key value
# Returns: 0 if valid, 1 otherwise
# Note: WebSocket key must be at least 8 printable ASCII characters (33-126, no spaces)
validate_websocket_key() {
    local key_name="$1"
    local key_value="$2"
    
    # Check if key is empty
    if [[ -z "${key_value}" ]]; then
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${key_name} is empty"
        return 1
    fi
    
    # Check minimum length (8 characters)
    if [[ ${#key_value} -lt 8 ]]; then
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${key_name} must be at least 8 characters long (got ${#key_value})"
        return 1
    fi
    
    # Check for printable ASCII characters only (33-126, no spaces/control chars)
    if [[ "${key_value}" =~ [[:space:]] ]]; then
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${key_name} contains spaces or control characters"
        return 1
    fi
    
    # Check for non-printable characters
    if [[ "${key_value}" =~ [^[:print:]] ]]; then
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${key_name} contains non-printable characters"
        return 1
    fi
    
    # All checks passed
    local display_value="${key_value:0:8}..."
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${key_name} is a valid WebSocket key: ${display_value}"
    return 0
}

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    
    # Extract the part starting from "hydrogen" and keep everything after
    local relative_path
    # shellcheck disable=SC2154 # SED defined externally in framework.sh
    relative_path=$(echo "${absolute_path}" | "${SED}" -n 's|.*/hydrogen/|hydrogen/|p')
    
    # If the path contains elements/001-hydrogen/hydrogen but not starting with hydrogen/
    if [[ -z "${relative_path}" ]]; then
        relative_path=$(echo "${absolute_path}" | "${SED}" -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
    fi
    
    # If we still couldn't find a match, return the original
    if [[ -z "${relative_path}" ]]; then
        echo "${absolute_path}"
    else
        echo "${relative_path}"
    fi
}
