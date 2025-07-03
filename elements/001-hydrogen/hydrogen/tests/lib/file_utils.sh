#!/bin/bash
#
# Hydrogen Test Suite - File Utilities Library
# Provides file and path manipulation functions for test scripts
#
FILE_UTILS_NAME="Hydrogen File Utilities Library"
FILE_UTILS_VERSION="1.0.0"

# VERSION HISTORY
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

# Function to display script version information
print_file_utils_version() {
    echo "=== $FILE_UTILS_NAME v$FILE_UTILS_VERSION ==="
}

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    
    # Extract the part starting from "hydrogen" and keep everything after
    local relative_path
    relative_path=$(echo "$absolute_path" | sed -n 's|.*/hydrogen/|hydrogen/|p')
    
    # If the path contains elements/001-hydrogen/hydrogen but not starting with hydrogen/
    if [ -z "$relative_path" ]; then
        relative_path=$(echo "$absolute_path" | sed -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
    fi
    
    # If we still couldn't find a match, return the original
    if [ -z "$relative_path" ]; then
        echo "$absolute_path"
    else
        echo "$relative_path"
    fi
}

# Function to safely change directory with error handling
safe_cd() {
    local target_dir="$1"
    if ! cd "$target_dir"; then
        # Note: This function uses print_error from log_output.sh
        # The calling script should source log_output.sh before file_utils.sh
        if command -v print_error >/dev/null 2>&1; then
            print_error "Failed to change directory to $target_dir"
        else
            echo "ERROR: Failed to change directory to $target_dir" >&2
        fi
        return 1
    fi
    return 0
}

# Function to get file size safely
get_file_size() {
    local file_path="$1"
    local size
    if size=$(stat -c %s "$file_path" 2>/dev/null); then
        echo "$size"
        return 0
    else
        echo "0"
        return 1
    fi
}

# Function to get the full path to a configuration file
# This centralizes config file access and handles the configs/ subdirectory
get_config_path() {
    local config_file="$1"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    local config_path="$script_dir/../configs/$config_file"
    
    echo "$config_path"
}

# Function to extract web server port from a JSON configuration file
extract_web_server_port() {
    local config_file="$1"
    local default_port=5000
    
    # Extract port using grep and sed (simple approach, could be improved with jq)
    if command -v jq &> /dev/null; then
        # Use jq if available for proper JSON parsing
        local port
        port=$(jq -r '.WebServer.Port // 5000' "$config_file" 2>/dev/null)
        if jq -r '.WebServer.Port // 5000' "$config_file" >/dev/null 2>&1 && [ -n "$port" ] && [ "$port" != "null" ]; then
            echo "$port"
            return 0
        fi
    fi
    
    # Fallback method using grep and sed
    local port
    port=$(grep -o '"Port":[^,}]*' "$config_file" | head -1 | sed 's/"Port":\s*\([0-9]*\)/\1/')
    if [ -n "$port" ]; then
        echo "$port"
        return 0
    fi
    
    # Return default port if extraction fails
    echo "$default_port"
    return 0
}

# Function to validate JSON syntax in a file
validate_json() {
    local file="$1"
    
    if command -v jq &> /dev/null; then
        # Use jq if available for proper JSON validation
        jq . "$file" > /dev/null 2>&1
        if jq . "$file" >/dev/null 2>&1; then
            if command -v print_result >/dev/null 2>&1; then
                print_result 0 "Response contains valid JSON"
            else
                echo "Response contains valid JSON"
            fi
            return 0
        else
            if command -v print_result >/dev/null 2>&1; then
                print_result 1 "Response contains invalid JSON"
            else
                echo "Response contains invalid JSON" >&2
            fi
            return 1
        fi
    else
        # Fallback: simple check for JSON structure
        if grep -q "{" "$file" && grep -q "}" "$file"; then
            if command -v print_result >/dev/null 2>&1; then
                print_result 0 "Response appears to be JSON (basic validation only)"
            else
                echo "Response appears to be JSON (basic validation only)"
            fi
            return 0
        else
            if command -v print_result >/dev/null 2>&1; then
                print_result 1 "Response does not appear to be JSON"
            else
                echo "Response does not appear to be JSON" >&2
            fi
            return 1
        fi
    fi
}

# Function to get the web server port from config (if not already defined)
get_webserver_port() {
    local config="$1"
    local port
    
    if command -v jq &> /dev/null; then
        # Use jq if available
        port=$(jq -r '.WebServer.Port // 8080' "$config" 2>/dev/null)
        if [ -z "$port" ] || [ "$port" = "null" ]; then
            port=8080
        fi
    else
        # Fallback to grep
        port=$(grep -o '"Port":[[:space:]]*[0-9]*' "$config" | head -1 | grep -o '[0-9]*')
        if [ -z "$port" ]; then
            port=8080
        fi
    fi
    echo "$port"
}
