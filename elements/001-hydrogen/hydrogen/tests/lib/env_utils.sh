#!/bin/bash
#
# Hydrogen Test Suite - Environment Utilities Library
# Provides functions for environment variable handling and validation
# Designed for use in test scripts to ensure required environment variables are set
#
NAME_SCRIPT="Hydrogen Environment Utilities Library"
VERS_SCRIPT="1.0.0"

# VERSION HISTORY
# 1.0.0 - 2025-07-02 - Initial creation for test_12_env_payload.sh migration

# Function to display script version information
print_env_utils_version() {
    echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="
}

# Function: Check if environment variable is set and non-empty
# Parameters: $1 - variable name, $2 - variable value
# Returns: 0 if set and non-empty, 1 otherwise
check_env_var() {
    local var_name="$1"
    local var_value="$2"
    
    if [ -n "$var_value" ]; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "✓ $var_name is set"
        else
            echo "INFO: ✓ $var_name is set"
        fi
        return 0
    else
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "✗ $var_name is not set"
        else
            echo "WARNING: ✗ $var_name is not set"
        fi
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
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "✗ $key_name failed base64 decode - no output generated"
        else
            echo "WARNING: ✗ $key_name failed base64 decode - no output generated"
        fi
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
            if command -v print_warning >/dev/null 2>&1; then
                print_warning "✗ Unknown key type: $key_type"
            else
                echo "WARNING: ✗ Unknown key type: $key_type"
            fi
            rm -f "$temp_key"
            return 1
            ;;
    esac
    
    # Validate key format - this is the real test
    if $openssl_cmd >/dev/null 2>&1; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "✓ $key_name is a valid RSA $key_type key"
        else
            echo "INFO: ✓ $key_name is a valid RSA $key_type key"
        fi
        rm -f "$temp_key"
        return 0
    else
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "✗ $key_name is not a valid RSA $key_type key"
        else
            echo "WARNING: ✗ $key_name is not a valid RSA $key_type key"
        fi
        rm -f "$temp_key"
        return 1
    fi
}
