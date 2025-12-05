#!/usr/bin/env bash

# Test: Installer
# Builds installer script from Hydrogen release executable and associated files

# CHANGELOG
# 1.3.0 - 2025-12-03 - Restructured validation tests to match expected output format with separate tests for each embedded file type
# 1.2.0 - 2025-12-03 - Validation now runs against most recent installer regardless of build status
# 1.1.0 - 2025-12-03 - Added embedded file validation and signature verification
# 1.0.0 - 2025-12-03 - Extracted installer building functionality from Test 01 to standalone test
 
set -euo pipefail

# Test configuration
TEST_NAME="Installer Generator"
TEST_ABBR="INS"
TEST_NUMBER="70"
TEST_COUNTER=0
TEST_VERSION="1.3.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Function to build installer
build_installer() {
    local installer_output
    local release_date
    local config_sha
    local license_sha
    local exe_sha
    local signature
    local installer_dir

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Build Hydrogen Installer"

    # Set installer directory relative to project root
    installer_dir="${PROJECT_DIR}/installer"

    # Get release date in yyyymmdd format
    local release_date
    release_date=$(date +%Y%m%d)
    
    # Check if we can skip installer build (if hydrogen_release hasn't changed)
    local installer_output="${installer_dir}/hydrogen_installer_${release_date}.sh"
    if [[ -f "${installer_output}" ]] && [[ -f "${PROJECT_DIR}/hydrogen_release" ]]; then
        # Compare modification times
        if [[ "${installer_output}" -nt "${PROJECT_DIR}/hydrogen_release" ]]; then
            local installer_size
            installer_size=$(get_file_size "${installer_output}")
            local formatted_size
            formatted_size=$(format_file_size "${installer_size}")
            print_optimization_status "Installer Build" "SKIP" "Installer already up to date"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Installer already exists: ${installer_output} (${formatted_size} bytes)"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
            return 0
        fi
    fi
    
    print_optimization_status "Installer Build" "BUILD" "Building installer.."

    # Generate SHA256 hashes for files
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "sha256sum examples/configs/hydrogen_default.json LICENSE.md hydrogen_release"
    local config_sha
    config_sha=$(sha256sum "${PROJECT_DIR}/examples/configs/hydrogen_default.json" | cut -d' ' -f1)
    local license_sha
    license_sha=$(sha256sum "${PROJECT_DIR}/LICENSE.md" | cut -d' ' -f1)
    local exe_sha
    exe_sha=$(sha256sum "${PROJECT_DIR}/hydrogen_release" | cut -d' ' -f1)

    # Base64 encode and line wrap files (76 chars)
    local config_b64
    local license_b64
    local exe_b64
    
    # We've also snuck in brotli compression here to reduce installer size though the main executable
    # likely gets no benefit from this as it is already compressed with UPX and the payload with brotli
    config_b64=$(brotli --stdout --best "${PROJECT_DIR}/examples/configs/hydrogen_default.json" | base64 -w 76)
    license_b64=$(brotli --stdout --best "${PROJECT_DIR}/LICENSE.md" | base64 -w 76)
    exe_b64=$(brotli --stdout --best "${PROJECT_DIR}/hydrogen_release" | base64 -w 76)

    # Add '# ' prefix to each line
    config_b64=${config_b64//$'\n'/$'\n'# }
    config_b64="# ${config_b64}"
    license_b64=${license_b64//$'\n'/$'\n'# }
    license_b64="# ${license_b64}"
    exe_b64=${exe_b64//$'\n'/$'\n'# }
    exe_b64="# ${exe_b64}"

    # Get version and release from the executable directly
    local version release
    if [[ -f "${PROJECT_DIR}/hydrogen_release" ]]; then
        # Extract version and release from executable output
        local version_output
        version_output=$("${PROJECT_DIR}/hydrogen_release" --version 2>/dev/null | head -1 || echo "")

        if [[ -n "${version_output}" ]]; then
            # Parse output like "Hydrogen ver 1.0.0.1721 rel 20250907"
            if [[ "${version_output}" =~ ver[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+) ]]; then
                version="${BASH_REMATCH[1]}"
            fi
            if [[ "${version_output}" =~ rel[[:space:]]+([0-9]+) ]]; then
                release="${BASH_REMATCH[1]}"
            fi

            if [[ -z "${version}" ]]; then
                version="1.0.1"
            fi
            if [[ -z "${release}" ]]; then
                release=$(date +%Y%m%d)
            fi
        else
            version="1.0.1"
            release=$(date +%Y%m%d)
        fi
    else
        version="1.0.1"
        release=$(date +%Y%m%d)
    fi

    local public_key="${HYDROGEN_INSTALLER_LOCK:-}"

    # Copy installer template
    cp "${installer_dir}/installer_wrapper.sh" "${installer_output}"

    # Replace placeholders
    sed -i "s/HYDROGEN_VERSION/${version}/g" "${installer_output}"
    sed -i "s/HYDROGEN_RELEASE/${release_date}/g" "${installer_output}"
    sed -i "s/HYDROGEN_INSTALLER_LOCK/${public_key}/g" "${installer_output}"

    # Insert base64 content with sha256 hashes (single marker per section)
    # Order: executable, config, license

    # Use sed with r command to read from files directly (avoids argument list too long)
    local exe_content_file="/tmp/exe_content_$$.txt"
    local config_content_file="/tmp/config_content_$$.txt"
    local license_content_file="/tmp/license_content_$$.txt"

    # Create content files
    echo -e "### HYDROGEN EXECUTABLE: ${exe_sha}\n#\n${exe_b64}" > "${exe_content_file}"
    echo -e "### HYDROGEN CONFIGURATION: ${config_sha}\n#\n${config_b64}" > "${config_content_file}"
    echo -e "### HYDROGEN LICENSE: ${license_sha}\n#\n${license_b64}" > "${license_content_file}"

    # Replace markers with content using sed r command (read file then delete marker)
    sed -i "/### HYDROGEN EXECUTABLE/{
        r ${exe_content_file}
        d
    }" "${installer_output}"
    sed -i "/### HYDROGEN CONFIGURATION/{
        r ${config_content_file}
        d
    }" "${installer_output}"
    sed -i "/### HYDROGEN LICENSE/{
        r ${license_content_file}
        d
    }" "${installer_output}"

    # Clean up temp files
    rm -f "${exe_content_file}" "${config_content_file}" "${license_content_file}"

    # Set executable permissions
    chmod +x "${installer_output}"

    # Generate signature using openssl (if keys are available) - AFTER file is complete
    local signature_text=""
    if [[ -n "${HYDROGEN_INSTALLER_KEY:-}" ]]; then
        local key_file="/tmp/private_key_$$.pem"
        # Decode the base64 private key and write to file
        echo "${HYDROGEN_INSTALLER_KEY}" | base64 -d > "${key_file}"

        # Debug: Check if key file was created and has content
        if [[ ! -f "${key_file}" ]] || [[ ! -s "${key_file}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Failed to create or decode private key file"
            echo -e "\n### HYDROGEN SIGNATURE" >> "${installer_output}"
        else
            # Sign the entire installer file content
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Generating signature with openssl..."
            if signature=$(openssl dgst -sha256 -sign "${key_file}" "${installer_output}" 2>/dev/null | brotli --stdout --best | base64 -w 76); then
                if [[ -n "${signature}" ]]; then
                    # Add '# ' prefix to each line, same as other embedded files
                    signature_text=${signature//$'\n'/$'\n'# }
                    echo -e "#" >> "${installer_output}"
                    echo -e "# ${signature_text}" >> "${installer_output}"
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Signature generation returned empty result"
                    sed -i "s|### HYDROGEN SIGNATURE|### HYDROGEN SIGNATURE\n#}" "${installer_output}"
                fi
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "openssl command failed"
                sed -i "s|### HYDROGEN SIGNATURE|### HYDROGEN SIGNATURE\n#}" "${installer_output}"
            fi
            # Clean up key file
            rm -f "${key_file}"
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HYDROGEN_INSTALLER_KEY not available, skipping signature generation"
        echo -e "\n### HYDROGEN SIGNATURE" >> "${installer_output}"
    fi

    # Clean up old installers, keep only most recent 5
    local installer_count=0
    installer_count=$(find "${installer_dir}" -maxdepth 1 -name "hydrogen_installer_*.sh" -type f 2>/dev/null | wc -l || true)

    if [[ "${installer_count}" -gt 5 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Cleaning up old installers, keeping 5 most recent"
        find "${installer_dir}" -maxdepth 1 -name "hydrogen_installer_*.sh" -type f -exec stat -c '%Y %n' {} \; 2>/dev/null | sort -n | head -n -5 | cut -d' ' -f2- | xargs rm -f || true
    fi

    # Check if installer was created successfully
    if [[ -f "${installer_output}" ]]; then
        local installer_size
        installer_size=$(get_file_size "${installer_output}")
        local formatted_size
        formatted_size=$(format_file_size "${installer_size}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Installer created successfully: ${installer_output} (${formatted_size} bytes)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to create installer file"
        EXIT_CODE=1
    fi
}

# Optimisation status function (extracted from original test)
print_optimization_status() {
    local step="$1"
    local status="$2"
    local details="$3"

    if [[ "${status}" == "SKIP" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Optimization: SKIP ${step} - ${details}"
    elif [[ "${status}" == "BUILD" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Optimization: BUILD ${step} - ${details}"
    elif [[ "${status}" == "DONE" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Optimization: DONE ${step} - ${details}"
    elif [[ "${status}" == "ERROR" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Optimization: ERROR ${step} - ${details}"
    fi
}

# Main test execution
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check Release Executable"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_release"

if [[ -f "hydrogen_release" ]]; then
    release_size=$(get_file_size "hydrogen_release")
    formatted_size=$(format_file_size "${release_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Release executable found: hydrogen_release (${formatted_size} bytes)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Release executable not found - hydrogen_release missing"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Check release executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check Release Executable Payload"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_release && grep -q '<<< HERE BE ME TREASURE >>>' "

if [[ -f "hydrogen_release" ]]; then
    # Check if the release binary has payload embedded using the correct marker
    if "${GREP}" -q "<<< HERE BE ME TREASURE >>>" "hydrogen_release" 2>/dev/null; then
        release_size=$(get_file_size "hydrogen_release")
        formatted_size=$(format_file_size "${release_size}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Release executable has embedded payload (${formatted_size} bytes total)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))

        # Generate installer
        build_installer
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Release executable appears to be missing embedded payload marker"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Release executable not found for payload verification"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify release executable payload" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

# Function to validate embedded executable
validate_embedded_executable() {
    local installer_file="$1"

    # Extract executable section
    local exe_section
    exe_section=$(sed -n '/### HYDROGEN EXECUTABLE/,/### HYDROGEN CONFIGURATION/p' "${installer_file}" | head -n -1)
    local exe_b64
    exe_b64=$(echo "${exe_section}" | sed '1,2d' | sed 's/^# //' | tr -d '\n')

    if [[ -n "${exe_b64}" ]]; then
        # Use temp file to avoid null byte issues
        local exe_temp="/tmp/exe_temp_$$"
        echo "${exe_b64}" | base64 -d 2>/dev/null | brotli -d 2>/dev/null > "${exe_temp}"
        if [[ -s "${exe_temp}" ]]; then
            if head -c 4 "${exe_temp}" | grep -q $'\x7fELF'; then
                local exe_size
                exe_size=$(stat -c %s "${exe_temp}")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Executable is valid (${exe_size} bytes)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Executable validated"
                rm -f "${exe_temp}"
                return 0
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Executable section is not valid ELF binary"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Executable validation failed"
                rm -f "${exe_temp}"
                return 1
            fi
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Failed to decode/decompress executable section"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Executable validation failed"
            rm -f "${exe_temp}"
            return 1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Executable section is empty"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Executable validation failed"
        return 1
    fi
}

# Function to validate embedded configuration
validate_embedded_config() {
    local installer_file="$1"

    # Extract configuration section
    local config_section
    config_section=$(sed -n '/### HYDROGEN CONFIGURATION/,/### HYDROGEN LICENSE/p' "${installer_file}" | head -n -1)
    local config_b64
    config_b64=$(echo "${config_section}" | sed '1,2d' | sed 's/^# //' | tr -d '\n')

    if [[ -n "${config_b64}" ]]; then
        local config_temp="/tmp/config_temp_$$"
        echo "${config_b64}" | base64 -d 2>/dev/null | brotli -d 2>/dev/null > "${config_temp}"
        if [[ -s "${config_temp}" ]]; then
            if jq empty "${config_temp}" 2>/dev/null; then
                local config_size
                config_size=$(stat -c %s "${config_temp}")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration is valid JSON (${config_size} bytes)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration validated"
                rm -f "${config_temp}"
                return 0
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration section is not valid JSON"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration validation failed"
                rm -f "${config_temp}"
                return 1
            fi
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Failed to decode/decompress configuration section"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration validation failed"
            rm -f "${config_temp}"
            return 1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration section is empty"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration validation failed"
        return 1
    fi
}

# Function to validate embedded license
validate_embedded_license() {
    local installer_file="$1"

    # Extract license section
    local license_section
    license_section=$(sed -n '/### HYDROGEN LICENSE/,/### HYDROGEN SIGNATURE/p' "${installer_file}" | head -n -1)
    local license_b64
    license_b64=$(echo "${license_section}" | sed '1,2d' | sed 's/^# //' | tr -d '\n')

    if [[ -n "${license_b64}" ]]; then
        local license_temp="/tmp/license_temp_$$"
        echo "${license_b64}" | base64 -d 2>/dev/null | brotli -d 2>/dev/null > "${license_temp}"
        if [[ -s "${license_temp}" ]]; then
            if grep -q '#' "${license_temp}"; then
                local license_size
                license_size=$(stat -c %s "${license_temp}")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "License is valid markdown (${license_size} bytes)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "License validated"
                rm -f "${license_temp}"
                return 0
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "License section is not valid markdown"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "License validation failed"
                rm -f "${license_temp}"
                return 1
            fi
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Failed to decode/decompress license section"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "License validation failed"
            rm -f "${license_temp}"
            return 1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "License section is empty"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "License validation failed"
        return 1
    fi
}

# Function to validate embedded signature
validate_embedded_signature() {
    local installer_file="$1"

    # Extract signature section
    local signature_section
    signature_section=$(sed -n '/### HYDROGEN SIGNATURE/,$p' "${installer_file}")
    local signature_b64
    signature_b64=$(echo "${signature_section}" | sed '1d' | sed 's/^# *//' | tr -d '\n')

    if [[ -n "${signature_b64}" ]]; then
        local sig_temp="/tmp/sig_temp_$$"
        echo "${signature_b64}" | base64 -d 2>/dev/null | brotli -d 2>/dev/null > "${sig_temp}"
        if [[ -s "${sig_temp}" ]]; then
            local signature_size
            signature_size=$(stat -c %s "${sig_temp}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Signature is valid brotli+base64 (${signature_size} bytes)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Signature validated"
            rm -f "${sig_temp}"
            return 0
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Failed to decode signature section"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Signature validation failed"
            rm -f "${sig_temp}"
            return 1
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Signature section is empty"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Signature validation failed (no signature)"
        return 1
    fi
}

# Function to verify installer signature
verify_installer_signature() {
    local installer_file="$1"

    # Check if installer file exists
    if [[ ! -f "${installer_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Installer file not found: ${installer_file}"
        return 1
    fi

    # Check if private key is available for verification
    if [[ -z "${HYDROGEN_INSTALLER_KEY:-}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Private key not available, skipping signature verification"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Signature verification skipped (no private key)"
        return 0
    fi

    # Extract signature from installer
    local signature_b64
    signature_b64=$(sed -n '/### HYDROGEN SIGNATURE/,$p' "${installer_file}" | sed '1d' | sed '/^#$/d' | sed 's/^# *//' | tr -d '\n')

    if [[ -z "${signature_b64}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "No signature found in installer"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Signature verification failed (no signature)"
        return 1
    fi

    # Decode the private key and extract public key
    local key_file="/tmp/private_key_$$.pem"
    echo "${HYDROGEN_INSTALLER_KEY}" | base64 -d > "${key_file}"
    local pub_key_file="/tmp/public_key_$$.pem"
    openssl rsa -in "${key_file}" -pubout -out "${pub_key_file}" 2>/dev/null

    if [[ ! -f "${key_file}" ]] || [[ ! -s "${key_file}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR Failed to decode public key"
        rm -f "${key_file}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Signature verification failed"
        return 1
    fi

    # Decode the signature
    local signature_file="/tmp/signature_$$.bin"
    echo "${signature_b64}" | base64 -d | brotli -d > "${signature_file}"

    if [[ ! -f "${signature_file}" ]] || [[ ! -s "${signature_file}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR Failed to decode signature"
        rm -f "${key_file}" "${signature_file}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Signature verification failed"
        return 1
    fi

    # Create a temporary copy of the installer up to the signature marker for verification
    local installer_without_sig="/tmp/installer_without_sig_$$.sh"
    local marker_line
    marker_line=$(grep -n '^### HYDROGEN SIGNATURE$' "${installer_file}" | cut -d: -f1)
    head -n "${marker_line}" "${installer_file}" > "${installer_without_sig}"

    # Verify the signature using openssl
    if openssl dgst -sha256 -verify "${pub_key_file}" -signature "${signature_file}" "${installer_without_sig}" >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Installer signature is valid"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Installer signature verified successfully"
        result=0
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Installer signature is invalid"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Installer signature verification failed"
        result=1
    fi

    # Clean up temporary files
    rm -f "${key_file}" "${pub_key_file}" "${signature_file}" "${installer_without_sig}"

    return "${result}"
}

# Find the most recent installer for validation (either newly created or existing)
installer_dir="${PROJECT_DIR}/installer"
most_recent_installer=""
release_date=$(date +%Y%m%d)
expected_installer="${installer_dir}/hydrogen_installer_${release_date}.sh"

# Check if today's installer exists first
if [[ -f "${expected_installer}" ]]; then
    most_recent_installer="${expected_installer}"
else
    # Find the most recent existing installer
    # Use -exec stat instead of -printf to avoid null byte issues
    most_recent_installer=$(find "${installer_dir}" -maxdepth 1 -name "hydrogen_installer_*.sh" -type f -exec stat -c '%Y %n' {} \; 2>/dev/null | sort -n | tail -1 | cut -d' ' -f2- || true)
fi

# Run validation tests against the most recent installer
if [[ -n "${most_recent_installer}" ]] && [[ -f "${most_recent_installer}" ]]; then
    # Check current installer
    TEST_COUNTER=$(( TEST_COUNTER + 1 ))
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Current Installer Check"
    if [[ -f "${most_recent_installer}" ]]; then
        installer_date=$(basename "${most_recent_installer}" | sed 's/hydrogen_installer_//' | sed 's/\.sh$//')
        formatted_date="${installer_date:0:4}-${installer_date:4:2}-${installer_date:6:2}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "INFO Current installer is dated ${formatted_date}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Current Installer Found"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Current Installer Not Found"
        EXIT_CODE=1
    fi

    # Validate embedded executable
    TEST_COUNTER=$(( TEST_COUNTER + 1 ))
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Embedded Executable"
    validate_embedded_executable "${most_recent_installer}"
    result=$?
    if [[ ${result} -eq 0 ]]; then
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi

    # Validate embedded configuration
    TEST_COUNTER=$(( TEST_COUNTER + 1 ))
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Embedded Configuration"
    validate_embedded_config "${most_recent_installer}"
    result=$?
    if [[ ${result} -eq 0 ]]; then
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi

    # Validate embedded license
    TEST_COUNTER=$(( TEST_COUNTER + 1 ))
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Embedded License"
    validate_embedded_license "${most_recent_installer}"
    result=$?
    if [[ ${result} -eq 0 ]]; then
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi

    # Validate embedded signature
    TEST_COUNTER=$(( TEST_COUNTER + 1 ))
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Embedded Signature"
    validate_embedded_signature "${most_recent_installer}"
    result=$?
    if [[ ${result} -eq 0 ]]; then
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi

    # Verify installer signature
    TEST_COUNTER=$(( TEST_COUNTER + 1 ))
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Installer Signature"
    verify_installer_signature "${most_recent_installer}"
    result=$?
    if [[ ${result} -ne 0 ]]; then
        EXIT_CODE=1
    else
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    fi
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WARN No installer available for validation"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Validation skipped (no installer available)"
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
