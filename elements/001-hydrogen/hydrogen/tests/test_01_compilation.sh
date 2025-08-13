#!/bin/bash

# Test: Compilation
# Tests compilation of the project using CMake

# FUNCTIONS
# download_unity_framework()

# CHANGELOG
# 3.1.2 - 2025-08-08 - Commented out somee of the print_command calls for "cd"
# 3.1.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.1.0 - 2025-07-31 - Removed coverage_cleanup call, another pass through to check for unnecessary comments, etc.
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.2.0 - 2025-07-14 - Added Unity framework check (moved from test 11), fixed tmpfs mount failure handling
# 2.1.0 - 2025-07-09 - Added payload generation subtest
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.0.0 - Initial version

# Test configuration
TEST_NAME="Compilation"
TEST_ABBR="CMP"
TEST_NUMBER="01"
TEST_VERSION="3.1.2"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Function to download Unity framework if missing
download_unity_framework() {
    local unity_dir="${SCRIPT_DIR}/unity"
    local framework_dir="${unity_dir}/framework"
    local unity_framework_dir="${framework_dir}/Unity"
    
    if [[ ! -d "${unity_framework_dir}" ]]; then
        print_message "Unity framework not found in ${unity_framework_dir}. Downloading now..."
        mkdir -p "${framework_dir}"
        if curl -L https://github.com/ThrowTheSwitch/Unity/archive/refs/heads/master.zip -o "${framework_dir}/unity.zip"; then
            unzip "${framework_dir}/unity.zip" -d "${framework_dir}/"
            mv "${framework_dir}/Unity-master" "${unity_framework_dir}"
            rm "${framework_dir}/unity.zip"
            print_result 0 "Unity framework downloaded and extracted successfully to ${unity_framework_dir}"
            return 0
        fi
    else
        print_message "Unity framework already exists in ${unity_framework_dir}"
        return 0
    fi
}

print_subtest "Check CMakeLists.txt"
print_command "test -f cmake/CMakeLists.txt"

if [[ -f "cmake/CMakeLists.txt" ]]; then
    file_size=$(get_file_size "cmake/CMakeLists.txt")
    formatted_size=$(format_file_size "${file_size}")
    print_result 0 "CMakeLists.txt found in cmake directory (${formatted_size} bytes)"
else
    print_result 1 "CMakeLists.txt not found in cmake directory"
    EXIT_CODE=1
fi
evaluate_test_result_silent "CMakeLists.txt check" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Check Source Files"
print_command "test -d src && test -f src/hydrogen.c"

if [[ -d "src" ]] && [[ -f "src/hydrogen.c" ]]; then
    src_count=$(find . -type f \( -path "./src/*" -o -path "./tests/unity/src/*" -o -path "./extras/*" -o -path "./examples/*" \) \( -name "*.c" -o -name "*.h" \) | wc -l || true) 
    print_result 0 "Project search found ${src_count} source files"
    TEST_NAME="${TEST_NAME} {BLUE}(source code: ${src_count} files){RESET}"
else
    print_result 1 "Source files not found - src/hydrogen.c missing"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Source files check" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Check Unity Framework"

if download_unity_framework; then
    print_result 0 "Unity framework check passed"
else
    print_result 1 "Unity framework check failed"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Unity framework check" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "CMake Configuration"
#print_command "cd cmake"

if safe_cd cmake; then
    print_command "cmake -S . -B ../build --preset default"
    if cmake -S . -B ../build --preset default >/dev/null 2>&1; then
        print_result 0 "CMake configuration successful with preset default"
    else
        EXIT_CODE=1
        print_result 1 "CMake configuration failed"
    fi
    safe_cd ..
else
    print_result 1 "Failed to enter cmake directory"
    EXIT_CODE=1
fi
evaluate_test_result_silent "CMake configuration" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Check and Generate Payload"
print_command "test -f payloads/payload.tar.br.enc"

if [[ -f "payloads/payload.tar.br.enc" ]]; then
    payload_size=$(get_file_size "payloads/payload.tar.br.enc")
    formatted_size=$(format_file_size "${payload_size}")
    print_result 0 "Payload file exists: payloads/payload.tar.br.enc (${formatted_size} bytes)"
    ((PASS_COUNT++))
else
    print_message "Payload file not found, generating..."
    # print_command "cd payloads"
    if safe_cd payloads; then
        print_command "./swagger-generate.sh"
        if ./swagger-generate.sh >/dev/null 2>&1; then
            print_message "Swagger generation completed"
            print_command "./payload-generate.sh"
            if ./payload-generate.sh >/dev/null 2>&1; then
                print_message "Payload generation completed"
                safe_cd ..
                if [[ -f "payloads/payload.tar.br.enc" ]]; then
                    payload_size=$(get_file_size "payloads/payload.tar.br.enc")
                    formatted_size=$(format_file_size "${payload_size}")
                    print_result 0 "Payload file generated successfully: payloads/payload.tar.br.enc (${formatted_size} bytes)"
                    ((PASS_COUNT++))
                else
                    print_result 1 "Payload file not found after generation"
                    EXIT_CODE=1
                fi
            else
                print_result 1 "Payload generation failed"
                safe_cd ..
                EXIT_CODE=1
            fi
        else
            print_result 1 "Swagger generation failed"
            safe_cd ..
            EXIT_CODE=1
        fi
    else
        print_result 1 "Failed to enter payloads directory"
        EXIT_CODE=1
    fi
fi

print_subtest "Build All Variants"
# print_command "cd cmake"

if safe_cd cmake; then
    print_command "cmake --build ../build --preset default --target all_variants"
    if cmake --build ../build --preset default --target all_variants >/dev/null 2>&1; then
        print_result 0 "All variants build successful with preset default"
    else
        EXIT_CODE=1
        print_result 1 "Build of all variants failed"
    fi
    safe_cd ..
else
    print_result 1 "Failed to enter cmake directory for building all variants"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Build all variants" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Default Executable"
print_command "test -f hydrogen"

if [[ -f "hydrogen" ]]; then
    exe_size=$(get_file_size "hydrogen")
    formatted_size=$(format_file_size "${exe_size}")
    print_result 0 "Executable created: hydrogen (${formatted_size} bytes)"
else
    print_result 1 "Default executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify default executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Debug Executable"
print_command "test -f hydrogen_debug"

if [[ -f "hydrogen_debug" ]]; then
    exe_size=$(get_file_size "hydrogen_debug")
    formatted_size=$(format_file_size "${exe_size}")
    print_result 0 "Executable created: hydrogen_debug (${formatted_size} bytes)"
else
    print_result 1 "Debug executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify debug executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Performance Executable"
print_command "test -f hydrogen_perf"

if [[ -f "hydrogen_perf" ]]; then
    exe_size=$(get_file_size "hydrogen_perf")
    formatted_size=$(format_file_size "${exe_size}")
    print_result 0 "Executable created: hydrogen_perf (${formatted_size} bytes)"
else
    print_result 1 "Performance executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify performance executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Valgrind Executable"
print_command "test -f hydrogen_valgrind"

if [[ -f "hydrogen_valgrind" ]]; then
    exe_size=$(get_file_size "hydrogen_valgrind")
    formatted_size=$(format_file_size "${exe_size}")
    print_result 0 "Executable created: hydrogen_valgrind (${formatted_size} bytes)"
else
    print_result 1 "Valgrind executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify valgrind executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Coverage Executable"
print_command "test -f hydrogen_coverage"

if [[ -f "hydrogen_coverage" ]]; then
    exe_size=$(get_file_size "hydrogen_coverage")
    formatted_size=$(format_file_size "${exe_size}")
    print_result 0 "Executable created: hydrogen_coverage (${formatted_size} bytes)"
else
    print_result 1 "Coverage executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify coverage executable" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Release and Naked Executables"
print_command "test -f hydrogen_release && test -f hydrogen_naked"

if [[ -f "hydrogen_release" ]] && [[ -f "hydrogen_naked" ]]; then
    release_size=$(get_file_size "hydrogen_release")
    naked_size=$(get_file_size "hydrogen_naked")
    formatted_release_size=$(format_file_size "${release_size}")
    formatted_naked_size=$(format_file_size "${naked_size}")
    print_result 0 "Executables created: hydrogen_release (${formatted_release_size} bytes), hydrogen_naked (${formatted_naked_size} bytes)"
else
    print_result 1 "Release or naked executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify release and naked executables" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Build Examples"
# print_command "cd cmake"

if safe_cd cmake; then
    print_command "cmake --build ../build --preset default --target all_examples"
    if cmake --build ../build --preset default --target all_examples >/dev/null 2>&1; then
        print_result 0 "Examples build successful with preset default"
    else
        EXIT_CODE=1
        print_result 1 "Examples build failed"
    fi
    safe_cd ..
else
    print_result 1 "Failed to enter cmake directory for examples build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Build examples" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Examples Executables"
print_command "test -f examples/C/auth_code_flow && test -f examples/C/client_credentials && test -f examples/C/password_flow"

if [[ -f "examples/C/auth_code_flow" ]] && [[ -f "examples/C/client_credentials" ]] && [[ -f "examples/C/password_flow" ]]; then
    auth_size=$(get_file_size "examples/C/auth_code_flow")
    client_size=$(get_file_size "examples/C/client_credentials")
    password_size=$(get_file_size "examples/C/password_flow")
    formatted_auth_size=$(format_file_size "${auth_size}")
    formatted_client_size=$(format_file_size "${client_size}")
    formatted_password_size=$(format_file_size "${password_size}")
    print_result 0 "Executables created: auth_code_flow (${formatted_auth_size} bytes), client_credentials (${formatted_client_size} bytes), password_flow (${formatted_password_size} bytes)"
else
    print_result 1 "One or more examples executables not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify examples executables" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Coverage Executable Payload"
print_command "test -f hydrogen_coverage && grep -q '<<< HERE BE ME TREASURE >>>' hydrogen_coverage"

if [[ -f "hydrogen_coverage" ]]; then
    # Check if the coverage binary has payload embedded using the correct marker
    if grep -q "<<< HERE BE ME TREASURE >>>" "hydrogen_coverage" 2>/dev/null; then
        coverage_size=$(get_file_size "hydrogen_coverage")
        formatted_size=$(format_file_size "${coverage_size}")
        print_result 0 "Coverage executable has embedded payload (${formatted_size} bytes total)"
    else
        print_result 1 "Coverage executable appears to be missing embedded payload marker"
        EXIT_CODE=1
    fi
else
    print_result 1 "Coverage executable not found for payload verification"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify coverage executable payload" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Verify Release Executable Payload"
print_command "test -f hydrogen_release && grep -q '<<< HERE BE ME TREASURE >>>' hydrogen_release"

if [[ -f "hydrogen_release" ]]; then
    # Check if the release binary has payload embedded using the correct marker
    if grep -q "<<< HERE BE ME TREASURE >>>" "hydrogen_release" 2>/dev/null; then
        release_size=$(get_file_size "hydrogen_release")
        formatted_size=$(format_file_size "${release_size}")
        print_result 0 "Release executable has embedded payload (${formatted_size} bytes total)"
    else
        print_result 1 "Release executable appears to be missing embedded payload marker"
        EXIT_CODE=1
    fi
else
    print_result 1 "Release executable not found for payload verification"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify release executable payload" "${EXIT_CODE}" "PASS_COUNT" "EXIT_CODE"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
