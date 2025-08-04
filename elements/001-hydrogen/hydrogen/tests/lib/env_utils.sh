#!/bin/bash

# Environment Utilities Library
# Provides functions for environment variable handling and validation

# LIBRARY FUNCTIONS
# check_env_var()
# validate_rsa_key()
# reset_environment_variables()
# set_basic_test_environment()
# validate_config_files()
# get_config_path()
# validate_websocket_key()
# convert_to_relative_path()
# set_type_conversion_environment()
# set_validation_test_environment()

# CHANGELOG
# 1.2.1 - 2025-08-03 - Removed extraneous command -v calls
# 1.2.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 1.1.0 - 2025-07-02 - Updated with additional functions for test_35_env_variables.sh migration
# 1.0.0 - 2025-07-02 - Initial creation for test_12_env_payload.sh migration

# Guard clause to prevent multiple sourcing
[[ -n "${ENV_UTILS_GUARD}" ]] && return 0
export ENV_UTILS_GUARD="true"

# Library metadata
ENV_UTILS_NAME="Environment Utilities Library"
ENV_UTILS_VERSION="1.2.1"
print_message "${ENV_UTILS_NAME} ${ENV_UTILS_VERSION}" "info"

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
        print_message "✓ ${var_name} is set to: ${display_value}"
        return 0
    else
        print_warning "✗ ${var_name} is not set or empty"
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
        print_warning "✗ ${key_name} failed base64 decode - no output generated"
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
            print_warning "✗ Unknown key type: ${key_type}"
            rm -f "${temp_key}"
            return 1
            ;;
    esac
    
    # Validate key format - this is the real test
    if ${openssl_cmd} >/dev/null 2>&1; then
        print_message "✓ ${key_name} is a valid RSA ${key_type} key"
        rm -f "${temp_key}"
        return 0
    else
        print_warning "✗ ${key_name} is not a valid RSA ${key_type} key"
        rm -f "${temp_key}"
        return 1
    fi
}

# Function to reset all environment variables used in testing
reset_environment_variables() {
    unset H_SERVER_NAME H_WEB_ENABLED H_WEB_PORT H_UPLOAD_DIR
    unset H_MAX_UPLOAD_SIZE H_SHUTDOWN_WAIT H_MAX_QUEUE_BLOCKS H_DEFAULT_QUEUE_CAPACITY
    unset H_MEMORY_WARNING H_LOAD_WARNING H_PRINT_QUEUE_ENABLED H_CONSOLE_LOG_LEVEL
    unset H_DEVICE_ID H_FRIENDLY_NAME
    
    print_message "All Hydrogen environment variables have been unset"
}

# Function to set environment variables for basic test
set_basic_test_environment() {
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
    
    print_message "Basic environment variables for Hydrogen test have been set"
    echo "INFO: Basic environment variables for Hydrogen test have been set"
}

# Function to validate required configuration files exist
validate_config_files() {
    local config_file
    config_file=$(get_config_path "hydrogen_test_env.json")
    
    if [[ ! -f "${config_file}" ]]; then
        print_error "Env test config file not found: ${config_file}"
        return 1
    fi
    
    print_message "Configuration file validated: ${config_file}"
    return 0
}

# Function to get the full path to a configuration file
get_config_path() {
    local config_file="$1"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    local config_path="${script_dir}/../configs/${config_file}"
    
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
        print_warning "✗ ${key_name} is empty"
        return 1
    fi
    
    # Check minimum length (8 characters)
    if [[ ${#key_value} -lt 8 ]]; then
        print_warning "✗ ${key_name} must be at least 8 characters long (got ${#key_value})"
        return 1
    fi
    
    # Check for printable ASCII characters only (33-126, no spaces/control chars)
    if [[ "${key_value}" =~ [[:space:]] ]]; then
        print_warning "✗ ${key_name} contains spaces or control characters"
        return 1
    fi
    
    # Check for non-printable characters
    if [[ "${key_value}" =~ [^[:print:]] ]]; then
        print_warning "✗ ${key_name} contains non-printable characters"
        return 1
    fi
    
    # All checks passed
    local display_value="${key_value:0:8}..."
    print_message "✓ ${key_name} is a valid WebSocket key: ${display_value}"
    return 0
}

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    
    # Extract the part starting from "hydrogen" and keep everything after
    local relative_path
    relative_path=$(echo "${absolute_path}" | sed -n 's|.*/hydrogen/|hydrogen/|p')
    
    # If the path contains elements/001-hydrogen/hydrogen but not starting with hydrogen/
    if [[ -z "${relative_path}" ]]; then
        relative_path=$(echo "${absolute_path}" | sed -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
    fi
    
    # If we still couldn't find a match, return the original
    if [[ -z "${relative_path}" ]]; then
        echo "${absolute_path}"
    else
        echo "${relative_path}"
    fi
}

# Function to set environment variables for type conversion testing
set_type_conversion_environment() {
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
    
    print_message "Type conversion environment variables for Hydrogen test have been set"
}

# Function to set environment variables for validation testing
set_validation_test_environment() {
    export H_SERVER_NAME="hydrogen-validation-test"
    export H_WEB_ENABLED="yes"   # non-standard boolean value
    export H_WEB_PORT="invalid"  # invalid port number
    export H_MEMORY_WARNING="150" # invalid percentage (>100)
    export H_LOAD_WARNING="-1.0"  # invalid negative load
    export H_PRINT_QUEUE_ENABLED="maybe" # invalid boolean
    export H_CONSOLE_LOG_LEVEL="debug"   # string instead of number
    export H_SHUTDOWN_WAIT="0"    # edge case: zero timeout
    export H_MAX_QUEUE_BLOCKS="0" # edge case: zero blocks
    
    print_message "Validation test environment variables for Hydrogen test have been set"
}
