#!/usr/bin/env bash

# Test: Compilation
# Tests compilation of the project using CMake

# FUNCTIONS
# download_unity_framework()

# CHANGELOG
# 3.3.0 - 2025-09-07 - Added installer build functionality and base64 encoding of distributables
# 3.2.0 - 2025-09-05 - Added CMock framework check and download alongside Unity framework
# 3.1.3 - 2025-08-23 - Added console dumps to give a more nuanced progress update
# 3.1.2 - 2025-08-08 - Commented out somee of the print_command calls for "cd"
# 3.1.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.1.0 - 2025-07-31 - Removed coverage_cleanup call, another pass through to check for unnecessary comments, etc.
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.2.0 - 2025-07-14 - Added Unity framework check (moved from test 11), fixed tmpfs mount failure handling
# 2.1.0 - 2025-07-09 - Added payload generation subtest
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.0.0 - Initial version

set -euo pipefail

# Test configuration
TEST_NAME="Compilation"
TEST_ABBR="CMP"
TEST_NUMBER="01"
TEST_COUNTER=0
TEST_VERSION="3.2.0"

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
    release_date=$(date +%Y%m%d)

    # Set installer output directory and filename
    local installer_dir="${PROJECT_DIR}/installer"
    local installer_output="${installer_dir}/hydrogen_installer_${release_date}.sh"

    # Generate SHA256 hashes for files
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "sha256sum configs/hydrogen_default.json LICENSE.md hydrogen_release"
    config_sha=$(sha256sum "${PROJECT_DIR}/configs/hydrogen_default.json" | cut -d' ' -f1)
    license_sha=$(sha256sum "${PROJECT_DIR}/LICENSE.md" | cut -d' ' -f1)
    exe_sha=$(sha256sum "${PROJECT_DIR}/hydrogen_release" | cut -d' ' -f1)

    # Base64 encode and line wrap files (76 chars)
    local config_b64
    local license_b64
    local exe_b64

    config_b64=$(base64 -w 76 "${PROJECT_DIR}/configs/hydrogen_default.json")
    license_b64=$(base64 -w 76 "${PROJECT_DIR}/LICENSE.md")
    exe_b64=$(base64 -w 76 "${PROJECT_DIR}/hydrogen_release")

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
            if signature=$(openssl dgst -sha256 -sign "${key_file}" "${installer_output}" 2>/dev/null | base64 -w 76); then
                if [[ -n "${signature}" ]]; then
                    signature_text=${signature//$'\n'/$'\n'# }
                    signature_text="# ${signature_text}"
                    # Use a temporary file to avoid sed command issues with newlines
                    local sig_file="/tmp/signature_content_$$.txt"
                    echo -e "#\n${signature_text}" > "${sig_file}"
                    sed -i "/### HYDROGEN SIGNATURE/r ${sig_file}" "${installer_output}"
                    rm -f "${sig_file}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Signature generated successfully"
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Signature generation returned empty result"
                    sed -i "s|### HYDROGEN SIGNATURE|### HYDROGEN SIGNATURE\n#|" "${installer_output}"
                fi
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "openssl command failed"
                sed -i "s|### HYDROGEN SIGNATURE|### HYDROGEN SIGNATURE\n#|" "${installer_output}"
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
        find "${installer_dir}" -maxdepth 1 -name "hydrogen_installer_*.sh" -type f -printf '%T@ %p\n' 2>/dev/null | sort -n | head -n -5 | cut -d' ' -f2- | xargs rm -f || true
    fi

    # Check if installer was created successfully
    if [[ -f "${installer_output}" ]]; then
        installer_size=$(get_file_size "${installer_output}")
        formatted_size=$(format_file_size "${installer_size}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Installer created successfully: ${installer_output} (${formatted_size} bytes)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to create installer file"
        EXIT_CODE=1
    fi
}

# Function to download Unity framework if missing
download_unity_framework() {
    local unity_dir="${SCRIPT_DIR}/unity"
    local framework_dir="${unity_dir}/framework"
    local unity_framework_dir="${framework_dir}/Unity"

    if [[ ! -d "${unity_framework_dir}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unity framework not found in ${unity_framework_dir}. Downloading now..."
        mkdir -p "${framework_dir}"
        if curl -L https://github.com/ThrowTheSwitch/Unity/archive/refs/heads/master.zip -o "${framework_dir}/unity.zip"; then
            unzip "${framework_dir}/unity.zip" -d "${framework_dir}/"
            mv "${framework_dir}/Unity-master" "${unity_framework_dir}"
            rm "${framework_dir}/unity.zip"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity framework downloaded and extracted successfully to ${unity_framework_dir}"
            return 0
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unity framework already exists in ${unity_framework_dir}"
        return 0
    fi
}

# Function to download CMock framework if missing
download_cmock_framework() {
    local unity_dir="${SCRIPT_DIR}/unity"
    local framework_dir="${unity_dir}/framework"
    local cmock_framework_dir="${framework_dir}/CMock"

    if [[ ! -d "${cmock_framework_dir}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "CMock framework not found in ${cmock_framework_dir}. Downloading now..."
        mkdir -p "${framework_dir}"
        if curl -L https://github.com/ThrowTheSwitch/CMock/archive/refs/heads/master.zip -o "${framework_dir}/cmock.zip"; then
            unzip "${framework_dir}/cmock.zip" -d "${framework_dir}/"
            mv "${framework_dir}/CMock-master" "${cmock_framework_dir}"
            rm "${framework_dir}/cmock.zip"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "CMock framework downloaded and extracted successfully to ${cmock_framework_dir}"
            return 0
        fi
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "CMock framework already exists in ${cmock_framework_dir}"
        return 0
    fi
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check CMakeLists.txt"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f cmake/CMakeLists.txt"

if [[ -f "cmake/CMakeLists.txt" ]]; then
    file_size=$(get_file_size "cmake/CMakeLists.txt")
    formatted_size=$(format_file_size "${file_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "CMakeLists.txt found in cmake directory (${formatted_size} bytes)"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "CMakeLists.txt not found in cmake directory"
    EXIT_CODE=1
fi
evaluate_test_result_silent "CMakeLists.txt check" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check Source Files"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -d src && test -f src/hydrogen.c"

if [[ -d "src" ]] && [[ -f "src/hydrogen.c" ]]; then
    src_count=$("${FIND}" . -type f \( -path "./src/*" -o -path "./tests/unity/src/*" -o -path "./extras/*" -o -path "./examples/*" \) \( -name "*.c" -o -name "*.h" \) | wc -l || true) 
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Project search found ${src_count} source files"
    TEST_NAME="${TEST_NAME} {BLUE}(source code: ${src_count} files){RESET}"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Source files not found - src/hydrogen.c missing"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Source files check" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check Unity Framework"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if download_unity_framework; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity framework check passed"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Unity framework check failed"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Unity framework check" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check CMock Framework"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if download_cmock_framework; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "CMock framework check passed"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "CMock framework check failed"
    EXIT_CODE=1
fi
evaluate_test_result_silent "CMock framework check" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "CMake Configuration"
#print_command "${TEST_NUMBER}" "cd cmake"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if safe_cd cmake; then
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "cmake -S . -B ../build --preset default"

    dump_collected_output
    clear_collected_output

    if cmake -S . -B ../build --preset default >/dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "CMake configuration successful with preset default"
    else
        EXIT_CODE=1
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "CMake configuration failed"
    fi
    safe_cd ..
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to enter cmake directory"
    EXIT_CODE=1
fi
evaluate_test_result_silent "CMake configuration" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check and Generate Payload"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f payloads/payload.tar.br.enc"

if [[ -f "payloads/payload.tar.br.enc" ]]; then
    payload_size=$(get_file_size "payloads/payload.tar.br.enc")
    formatted_size=$(format_file_size "${payload_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Payload file exists: payloads/payload.tar.br.enc (${formatted_size} bytes)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Payload file not found, generating..."
    # print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "cd payloads"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if safe_cd payloads; then
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "./swagger-generate.sh"
        if ./swagger-generate.sh >/dev/null 2>&1; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Swagger generation completed"
            print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "./payload-generate.sh"
            if ./payload-generate.sh >/dev/null 2>&1; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Payload generation completed"
                safe_cd ..
                if [[ -f "payloads/payload.tar.br.enc" ]]; then
                    payload_size=$(get_file_size "payloads/payload.tar.br.enc")
                    formatted_size=$(format_file_size "${payload_size}")
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Payload file generated successfully: payloads/payload.tar.br.enc (${formatted_size} bytes)"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Payload file not found after generation"
                    EXIT_CODE=1
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Payload generation failed"
                safe_cd ..
                EXIT_CODE=1
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Swagger generation failed"
            safe_cd ..
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to enter payloads directory"
        EXIT_CODE=1
    fi
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Build All Variants"
# print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "cd cmake"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if safe_cd cmake; then
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "cmake --build ../build --preset default --target all_variants"

    dump_collected_output
    clear_collected_output

    if cmake --build ../build --preset default --target all_variants >/dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All variants build successful with preset default"
    else
        EXIT_CODE=1
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Build of all variants failed"
    fi
    safe_cd ..
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to enter cmake directory for building all variants"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Build all variants" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Default Executable"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen"

if [[ -f "hydrogen" ]]; then
    exe_size=$(get_file_size "hydrogen")
    formatted_size=$(format_file_size "${exe_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Executable created: hydrogen (${formatted_size} bytes)"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Default executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify default executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Debug Executable"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_debug"

if [[ -f "hydrogen_debug" ]]; then
    exe_size=$(get_file_size "hydrogen_debug")
    formatted_size=$(format_file_size "${exe_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Executable created: hydrogen_debug (${formatted_size} bytes)"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Debug executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify debug executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Performance Executable"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_perf"

if [[ -f "hydrogen_perf" ]]; then
    exe_size=$(get_file_size "hydrogen_perf")
    formatted_size=$(format_file_size "${exe_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Executable created: hydrogen_perf (${formatted_size} bytes)"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Performance executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify performance executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Valgrind Executable"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_valgrind"

if [[ -f "hydrogen_valgrind" ]]; then
    exe_size=$(get_file_size "hydrogen_valgrind")
    formatted_size=$(format_file_size "${exe_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Executable created: hydrogen_valgrind (${formatted_size} bytes)"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Valgrind executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify valgrind executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Coverage Executable"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_coverage"

if [[ -f "hydrogen_coverage" ]]; then
    exe_size=$(get_file_size "hydrogen_coverage")
    formatted_size=$(format_file_size "${exe_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Executable created: hydrogen_coverage (${formatted_size} bytes)"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Coverage executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify coverage executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Release and Naked Executables"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_release && test -f hydrogen_naked"

if [[ -f "hydrogen_release" ]] && [[ -f "hydrogen_naked" ]]; then
    release_size=$(get_file_size "hydrogen_release")
    naked_size=$(get_file_size "hydrogen_naked")
    formatted_release_size=$(format_file_size "${release_size}")
    formatted_naked_size=$(format_file_size "${naked_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Executables created: hydrogen_release (${formatted_release_size} bytes), hydrogen_naked (${formatted_naked_size} bytes)"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Release or naked executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify release and naked executables" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Build Examples"
# print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "cd cmake"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if safe_cd cmake; then
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "cmake --build ../build --preset default --target all_examples"
    if cmake --build ../build --preset default --target all_examples >/dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Examples build successful with preset default"
    else
        EXIT_CODE=1
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Examples build failed"
    fi
    safe_cd ..
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to enter cmake directory for examples build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Build examples" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Examples Executables"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f examples/C/auth_code_flow && test -f examples/C/client_credentials && test -f examples/C/password_flow"

if [[ -f "examples/C/auth_code_flow" ]] && [[ -f "examples/C/client_credentials" ]] && [[ -f "examples/C/password_flow" ]]; then
    auth_size=$(get_file_size "examples/C/auth_code_flow")
    client_size=$(get_file_size "examples/C/client_credentials")
    password_size=$(get_file_size "examples/C/password_flow")
    formatted_auth_size=$(format_file_size "${auth_size}")
    formatted_client_size=$(format_file_size "${client_size}")
    formatted_password_size=$(format_file_size "${password_size}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Executables created: auth_code_flow (${formatted_auth_size} bytes), client_credentials (${formatted_client_size} bytes), password_flow (${formatted_password_size} bytes)"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "One or more examples executables not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify examples executables" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Default Executable Payload"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen && grep -q '<<< HERE BE ME TREASURE >>>' hydrogen"

if [[ -f "hydrogen" ]]; then
    # Check if the default binary has payload embedded using the correct marker
    if "${GREP}" -q "<<< HERE BE ME TREASURE >>>" "hydrogen" 2>/dev/null; then
        hydrogen_size=$(get_file_size "hydrogen")
        formatted_size=$(format_file_size "${hydrogen_size}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Default executable has embedded payload (${formatted_size} bytes total)"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Default executable appears to be missing embedded payload marker"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Default executable not found for payload verification"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify default executable payload" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Coverage Executable Payload"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_coverage && grep -q '<<< HERE BE ME TREASURE >>>' hydrogen_coverage"

if [[ -f "hydrogen_coverage" ]]; then
    # Check if the coverage binary has payload embedded using the correct marker
    if "${GREP}" -q "<<< HERE BE ME TREASURE >>>" "hydrogen_coverage" 2>/dev/null; then
        coverage_size=$(get_file_size "hydrogen_coverage")
        formatted_size=$(format_file_size "${coverage_size}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Coverage executable has embedded payload (${formatted_size} bytes total)"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Coverage executable appears to be missing embedded payload marker"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Coverage executable not found for payload verification"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify coverage executable payload" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Release Executable Payload"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f hydrogen_release && grep -q '<<< HERE BE ME TREASURE >>>' hydrogen_release"

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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Unity Payload Test Executable Payload"
print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "test -f build/unity/src/payload/payload_test_process_payload_data && grep -q '<<< HERE BE ME TREASURE >>>' build/unity/src/payload/payload_test_process_payload_data"

if [[ -f "build/unity/src/payload/payload_test_process_payload_data" ]]; then
    # Check if the unity payload test binary has payload embedded using the correct marker
    if "${GREP}" -q "<<< HERE BE ME TREASURE >>>" "build/unity/src/payload/payload_test_process_payload_data" 2>/dev/null; then
        unity_payload_size=$(get_file_size "build/unity/src/payload/payload_test_process_payload_data")
        formatted_size=$(format_file_size "${unity_payload_size}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity payload test executable has embedded payload (${formatted_size} bytes total)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Unity payload test executable appears to be missing embedded payload marker"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Unity payload test executable not found for payload verification"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify unity payload test executable payload" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

lcov --capture --initial --directory "${PROJECT_DIR}/build/unity" --output-file "${PROJECT_DIR}/build/tests/results/coverage_unity.info" --ignore-errors empty >/dev/null 2>&1 &
lcov --capture --initial --directory "${PROJECT_DIR}/build/coverage" --output-file "${PROJECT_DIR}/build/tests/results/coverage_blackbox.info" --ignore-errors empty >/dev/null 2>&1 &

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
