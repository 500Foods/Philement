#!/usr/bin/env bash

# test_24_uploads.sh
#
# Test script for the system/upload API endpoint
# Validates file upload functionality through the REST API with real test artifacts

# CHANGELOG
# 1.1.0 - 2025-08-24 - Complete rewrite following test_22 pattern with parallel execution
# 1.0.0 - 2025-08-24 - Initial implementation following test_22_swagger.sh pattern

set -euo pipefail

# Test Configuration
TEST_NAME="Uploads"
TEST_ABBR="UPL"
TEST_NUMBER="24"
TEST_COUNTER=0
TEST_VERSION="1.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A UPLOAD_TEST_CONFIGS

# Upload test configuration - format: "config_file:log_suffix:test_type:artifact_file:http_method:description"
UPLOAD_TEST_CONFIGS=(
    ["T1"]="${SCRIPT_DIR}/configs/hydrogen_test_24_uploads_1.json:t1:api:3DBenchy.3mf:POST:T1 - API Upload 3DBenchy.3mf (3.0MB - should succeed)"
    ["T2"]="${SCRIPT_DIR}/configs/hydrogen_test_24_uploads_2.json:t2:api:3DBenchy.stl:POST:T2 - API Upload 3DBenchy.stl (11MB - should fail)"
    ["T3"]="${SCRIPT_DIR}/configs/hydrogen_test_24_uploads_3.json:t3:web:3DBenchy.gcode:POST:T3 - Web Upload 3DBenchy.gcode (4.2MB - should succeed with Beryllium)"
    ["T4"]="${SCRIPT_DIR}/configs/hydrogen_test_24_uploads_4.json:t4:web:3DBenchy.stl:POST:T4 - Web Upload 3DBenchy.stl (11MB - should fail)"
    ["T5"]="${SCRIPT_DIR}/configs/hydrogen_test_24_uploads_5.json:t5:api::GET:T5 - API Method Test (GET - should return 405 Method Not Allowed)"
)

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10

# Function to run upload test in parallel
run_upload_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local test_type="$4"
    local artifact_file="$5"
    local http_method="$6"
    local description="$7"

    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
    local port
    port=$(get_webserver_port "${config_file}")

    # Clear result file
    true > "${result_file}"

    # Start hydrogen server
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!

    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    # Wait for startup
    local startup_success=false
    local start_time
    start_time=${SECONDS}

    while true; do
        if [[ $((SECONDS - start_time)) -ge "${STARTUP_TIMEOUT}" ]]; then
            break
        fi

        if "${GREP}" -q "STARTUP COMPLETE" "${log_file}" 2>/dev/null; then
            startup_success=true
            break
        fi
        sleep 0.05
    done

    if [[ "${startup_success}" = true ]]; then
        echo "STARTUP_SUCCESS" >> "${result_file}"

        # Wait for server to be ready
        local base_url="http://localhost:${port}"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if wait_for_server_ready "${base_url}"; then
            # Additional check: verify webserver actually initialized successfully
            if "${GREP}" -q "WebServer.*COMPLETE" "${log_file}" 2>/dev/null; then
                echo "WEBSERVER_READY" >> "${result_file}"
                echo "SERVER_READY" >> "${result_file}"
            else
                echo "WEBSERVER_FAILED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WebServer failed to initialize properly"
            fi

            local all_tests_passed=true
            local artifact_path="${SCRIPT_DIR}/artifacts/uploads/${artifact_file}"

            if [[ "${test_type}" = "api" ]]; then
                # API endpoint test
                local api_response_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_api_response.json"
                local response
                local http_code

                if [[ "${http_method}" = "POST" ]]; then
                    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s -w 'HTTPSTATUS:%{http_code}' -X POST -F 'file=@${artifact_path}' -F 'print=false' '${base_url}/api/system/upload'"

                    response=$(curl -s -w "HTTPSTATUS:%{http_code}" \
                        -X POST \
                        -F "file=@${artifact_path}" \
                        -F "print=false" \
                        --max-time 60 \
                        "${base_url}/api/system/upload")

                    http_code=$(echo "${response}" | tr -d '\n' | "${SED}" -e 's/.*HTTPSTATUS://')
                    local response_body
                    response_body=${response%%HTTPSTATUS:*}

                    echo "${response_body}" > "${api_response_file}"

                    if [[ "${http_code}" -eq 200 ]]; then
                        # Should succeed for 3DBenchy.3mf, fail for 3DBenchy.stl
                        if [[ "${artifact_file}" = "3DBenchy.3mf" ]]; then
                            if echo "${response_body}" | jq -e '.done' >/dev/null 2>&1; then
                                echo "API_UPLOAD_SUCCESS" >> "${result_file}"

                                # Extract upload completion info from server logs
                                local upload_info
                                upload_info=$("${GREP}" -v "\[ TRACE \]" "${log_file}" | "${GREP}" -A 5 "File upload completed:" 2>/dev/null || true)
                                if [[ -n "${upload_info}" ]]; then
                                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - SUCCESS"
                                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${upload_info}"
                                else
                                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - Success: $("${STAT}" -c%s "${artifact_path}" || true) bytes uploaded"
                                fi
                            else
                                echo "API_UPLOAD_FAILED" >> "${result_file}"
                                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${description} - Response validation failed"
                                all_tests_passed=false
                            fi
                        else
                            echo "API_UPLOAD_UNEXPECTED_SUCCESS" >> "${result_file}"
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${description} - Unexpected success for oversized file"
                            all_tests_passed=false
                        fi
                    elif [[ "${http_code}" -eq 413 ]]; then
                        # Should fail for 3DBenchy.stl due to size limit
                        if [[ "${artifact_file}" = "3DBenchy.stl" ]]; then
                            echo "API_UPLOAD_CORRECTLY_REJECTED" >> "${result_file}"

                            # Extract rejection info from server logs
                            local reject_info
                            reject_info=$("${GREP}" -v "\[ TRACE \]" "${log_file}" | "${GREP}" -B 2 -A 2 "File upload exceeds maximum allowed size" 2>/dev/null || true)
                            if [[ -n "${reject_info}" ]]; then
                                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - Correctly rejected"
                                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${reject_info}"
                            else
                                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - Correctly rejected oversized file"
                            fi
                        else
                            echo "API_UPLOAD_UNEXPECTEDLY_REJECTED" >> "${result_file}"
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${description} - Unexpected rejection for valid file"
                            all_tests_passed=false
                        fi
                    else
                        echo "API_UPLOAD_UNEXPECTED_CODE_${http_code}" >> "${result_file}"
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${description} - Unexpected HTTP code: ${http_code}"
                        all_tests_passed=false
                    fi
                else
                    # Non-POST method test (e.g., GET)
                    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s -w 'HTTPSTATUS:%{http_code}' -X ${http_method} '${base_url}/api/system/upload'"

                    response=$(curl -s -w "HTTPSTATUS:%{http_code}" \
                        -X "${http_method}" \
                        --max-time 60 \
                        "${base_url}/api/system/upload")

                    http_code=$(echo "${response}" | tr -d '\n' | "${SED}" -e 's/.*HTTPSTATUS://')
                    local response_body
                    response_body=${response%%HTTPSTATUS:*}

                    echo "${response_body}" > "${api_response_file}"

                    if [[ "${http_code}" -eq 405 ]]; then
                        echo "API_METHOD_NOT_ALLOWED_SUCCESS" >> "${result_file}"
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - Correctly returned 405 Method Not Allowed"

                        # Extract method not allowed info from server logs if available
                        local method_info
                        method_info=$("${GREP}" -v "\[ TRACE \]" "${log_file}" | "${GREP}" -B 2 -A 2 "Method not allowed" 2>/dev/null || true)
                        if [[ -n "${method_info}" ]]; then
                            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${method_info}"
                        fi
                    else
                        echo "API_METHOD_UNEXPECTED_CODE_${http_code}" >> "${result_file}"
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${description} - Expected 405, got ${http_code}"
                        all_tests_passed=false
                    fi
                fi

            elif [[ "${test_type}" = "web" ]]; then
                # Web server endpoint test
                # local web_response_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_web_response.html"
                local response
                local http_code

                print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s -w 'HTTPSTATUS:%{http_code}' -X POST -F 'file=@${artifact_path}' -F 'print=false' '${base_url}/upload'"

                response=$(curl -s -w "HTTPSTATUS:%{http_code}" \
                    -X POST \
                    -F "file=@${artifact_path}" \
                    -F "print=false" \
                    --max-time 60 \
                    "${base_url}/upload")

                http_code=$(echo "${response}" | tr -d '\n' | sed -e 's/.*HTTPSTATUS://')

                if [[ "${http_code}" -eq 200 ]]; then
                    # Check if this should succeed or fail
                    if [[ "${artifact_file}" = "3DBenchy.gcode" ]]; then
                        echo "WEB_UPLOAD_SUCCESS" >> "${result_file}"

                        # Extract upload completion info from server logs
                        local upload_info
                        upload_info=$("${GREP}" -v "\[ TRACE \]" "${log_file}" | "${GREP}" -A 5 "File upload completed:" 2>/dev/null || true)
                        if [[ -n "${upload_info}" ]]; then
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - SUCCESS"
                            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${upload_info}"

                            # Check for Beryllium analysis in logs
                            if "${GREP}" -q "Running beryllium analysis" "${log_file}" 2>/dev/null; then
                                echo "BERYLLIUM_ANALYSIS_FOUND" >> "${result_file}"
                                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Beryllium analysis initiated"

                                # Extract beryllium results
                                local beryllium_info
                                beryllium_info=$("${GREP}" -v "\[ TRACE \]" "${log_file}" | "${GREP}" -A 3 "Beryllium analysis" 2>/dev/null || true)
                                if [[ -n "${beryllium_info}" ]]; then
                                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${beryllium_info}"
                                fi
                            else
                                echo "BERYLLIUM_ANALYSIS_MISSING" >> "${result_file}"
                                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "! Beryllium analysis not found in logs"
                            fi
                        else
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - Success: $("${STAT}" -c%s "${artifact_path}" || true) bytes uploaded"
                        fi
                    else
                        echo "WEB_UPLOAD_UNEXPECTED_SUCCESS" >> "${result_file}"
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${description} - Unexpected success for oversized file"
                        all_tests_passed=false
                    fi
                elif [[ "${http_code}" -eq 413 ]]; then
                    # Should fail for 3DBenchy.stl due to size limit
                    if [[ "${artifact_file}" = "3DBenchy.stl" ]]; then
                        echo "WEB_UPLOAD_CORRECTLY_REJECTED" >> "${result_file}"

                        # Extract rejection info from server logs
                        local reject_info
                        reject_info=$("${GREP}" -v "\[ TRACE \]" "${log_file}" | "${GREP}" -B 2 -A 2 "File upload exceeds maximum allowed size" 2>/dev/null || true)
                        if [[ -n "${reject_info}" ]]; then
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - Correctly rejected"
                            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${reject_info}"
                        else
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ ${description} - Correctly rejected oversized file"
                        fi
                    else
                        echo "WEB_UPLOAD_UNEXPECTEDLY_REJECTED" >> "${result_file}"
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${description} - Unexpected rejection for valid file"
                        all_tests_passed=false
                    fi
                else
                    echo "WEB_UPLOAD_UNEXPECTED_CODE_${http_code}" >> "${result_file}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ ${description} - Unexpected HTTP code: ${http_code}"
                    all_tests_passed=false
                fi
            fi

            if [[ "${all_tests_passed}" = true ]]; then
                echo "ALL_UPLOAD_TESTS_PASSED" >> "${result_file}"
            else
                echo "SOME_UPLOAD_TESTS_FAILED" >> "${result_file}"
            fi
        else
            echo "SERVER_NOT_READY" >> "${result_file}"
        fi

        # Stop the server
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true
            local shutdown_start
            shutdown_start=${SECONDS}
            while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
                if [[ $((SECONDS - shutdown_start)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
                    kill -9 "${hydrogen_pid}" 2>/dev/null || true
                    break
                fi
                sleep 0.05
            done
        fi

        echo "TEST_COMPLETE" >> "${result_file}"
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        echo "TEST_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi
}

# Function to analyze results from parallel upload test execution
analyze_upload_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local test_type="$3"
    local artifact_file="$4"
    local description="$5"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi

    # Check startup
    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen for ${description}"
        return 1
    fi

    # Check server readiness - both general readiness and webserver-specific readiness
    if ! "${GREP}" -q "SERVER_READY" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not ready for ${description}"
        return 1
    fi

    if "${GREP}" -q "WEBSERVER_FAILED" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "WebServer failed to initialize properly for ${description}"
        return 1
    fi

    # Check test-specific results
    if [[ "${test_type}" = "api" ]]; then
        if [[ "${artifact_file}" = "3DBenchy.3mf" ]]; then
            if "${GREP}" -q "API_UPLOAD_SUCCESS" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - SUCCESS"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - FAILED"
                return 1
            fi
        elif [[ "${artifact_file}" = "3DBenchy.stl" ]]; then
            if "${GREP}" -q "API_UPLOAD_CORRECTLY_REJECTED" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - SUCCESS (correctly rejected)"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - FAILED"
                return 1
            fi
        elif [[ -z "${artifact_file}" ]]; then
            # This is the method test case (e.g., GET method)
            if "${GREP}" -q "API_METHOD_NOT_ALLOWED_SUCCESS" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - SUCCESS (correctly rejected)"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - FAILED"
                return 1
            fi
        fi
    elif [[ "${test_type}" = "web" ]]; then
        if [[ "${artifact_file}" = "3DBenchy.gcode" ]]; then
            if "${GREP}" -q "WEB_UPLOAD_SUCCESS" "${result_file}" 2>/dev/null && \
               "${GREP}" -q "BERYLLIUM_ANALYSIS_FOUND" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - SUCCESS with Beryllium analysis"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - FAILED"
                return 1
            fi
        elif [[ "${artifact_file}" = "3DBenchy.stl" ]]; then
            if "${GREP}" -q "WEB_UPLOAD_CORRECTLY_REJECTED" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} - SUCCESS (correctly rejected)"
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - FAILED"
                return 1
            fi
        fi
    fi

    return 1
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Validate all configuration files
config_valid=true
for test_config in "${!UPLOAD_TEST_CONFIGS[@]}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: ${test_config}"

    # Parse test configuration
    IFS=':' read -r config_file log_suffix test_type artifact_file http_method description <<< "${UPLOAD_TEST_CONFIGS[${test_config}]}"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_config_file "${config_file}"; then
        port=$(get_webserver_port "${config_file}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description} will use port: ${port}"

        # Check if artifact file exists (skip for method tests with no artifact)
        if [[ -n "${artifact_file}" ]]; then
            if [[ -f "${SCRIPT_DIR}/artifacts/uploads/${artifact_file}" ]]; then
                file_size=$(stat -c%s "${SCRIPT_DIR}/artifacts/uploads/${artifact_file}")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test file: ${artifact_file} (${file_size} bytes)"
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: Test file not found: ${SCRIPT_DIR}/artifacts/uploads/${artifact_file}"
                config_valid=false
            fi
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Method test - no artifact file required"
        fi
    else
        config_valid=false
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate All Configurations"
if [[ "${config_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All configuration files and artifacts validated successfully"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration or artifact validation failed"
    EXIT_CODE=1
fi

# Only proceed with upload tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Upload Tests in Parallel"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting 4 parallel upload test instances..."

    # Start all upload tests in parallel with job limiting
    for test_config in "${!UPLOAD_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n  # Wait for any job to finish
        done

        # Parse test configuration
        IFS=':' read -r config_file log_suffix test_type artifact_file http_method description <<< "${UPLOAD_TEST_CONFIGS[${test_config}]}"

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (${description})"

        # Run parallel upload test in background
        run_upload_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${test_type}" "${artifact_file}" "${http_method}" "${description}" &
        PARALLEL_PIDS+=($!)
    done

    # Wait for all parallel tests to complete
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#UPLOAD_TEST_CONFIGS[@]} parallel upload tests to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel tests completed, analyzing results"

    # Process results sequentially for clean output
    successful_tests=0
    for test_config in "${!UPLOAD_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r config_file log_suffix test_type artifact_file http_method description <<< "${UPLOAD_TEST_CONFIGS[${test_config}]}"

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Upload Test: ${description}"

        # Display full server log section for this test
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        if [[ -f "${log_file}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Server Log: ${log_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Result Log: ${result_file}"

            # Extract the full server runtime log section (from 2 lines after "Press Ctrl+C to exit (SIGINT)" to 1 line before "SIGINT received")
            full_log_section=$("${GREP}" -A 10000 "Press Ctrl+C to exit (SIGINT)" "${log_file}" 2>/dev/null | tail -n +2 | "${GREP}" -B 10000 -A 3 "SIGINT received" | head -n -1 2>/dev/null || true)
            # Filter out low-level TRACE lines from the displayed log
            full_log_section=$(echo "${full_log_section}" | "${GREP}" -v "\[ TRACE \]" || true)
            if [[ -n "${full_log_section}" ]]; then
                # Process each line following Test 15 pattern for consistent log formatting
                while IFS= read -r line; do
                    output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${output_line}"
                done < <(echo "${full_log_section}" | head -n 30 || true)  # Show more lines for complete upload process
            fi
        fi

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if analyze_upload_test_results "${test_config}" "${log_suffix}" "${test_type}" "${artifact_file}" "${description}"; then
            successful_tests=$(( successful_tests + 1 ))
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_tests}/${#UPLOAD_TEST_CONFIGS[@]} upload tests passed"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - testing 10MB upload limits with real artifacts"

else
    # Skip upload tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping upload tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
