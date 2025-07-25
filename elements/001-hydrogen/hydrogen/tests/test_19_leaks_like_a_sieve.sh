#!/bin/bash

# Test: Memory Leak Detection
# Uses Valgrind to detect memory leaks in the Hydrogen application

# CHANGELOG
# 3.0.4 - 2025-07-15 - No more sleep
# 3.0.3 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.2 - 2025-07-13 - Filter output to show only leak count lines
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test pattern
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, reduced script size, enhanced comments
# 1.0.0 - 2025-06-15 - Initial version with basic memory leak detection

# Test configuration
TEST_NAME="Memory Leak Detection"
SCRIPT_VERSION="3.0.4"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "${LOG_OUTPUT_SH_GUARD}" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "${SCRIPT_DIR}/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/framework.sh"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "${SCRIPT_DIR}/lib/lifecycle.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=4
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Always use build/tests/results directory
BUILD_DIR="${SCRIPT_DIR}/../build"
RESULTS_DIR="${BUILD_DIR}/tests/results"
mkdir -p "${RESULTS_DIR}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "${SCRIPT_DIR}"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Set up results directory (after navigating to project root)

# Test configuration
CONFIG_FILE="tests/configs/hydrogen_test_api_test_1.json"

# Subtest 1: Validate debug build availability and ASAN support
next_subtest
print_subtest "Validate Debug Build and ASAN Support"

# Find debug build
DEBUG_BUILD="hydrogen_debug"
if [ ! -f "${DEBUG_BUILD}" ]; then
    print_result 1 "Debug build not found. This test requires the debug build with ASAN."
    EXIT_CODE=1
else
    # Verify ASAN is enabled in debug build
    if ! readelf -s "${DEBUG_BUILD}" | grep -q "__asan"; then
        print_result 1 "Debug build does not have ASAN enabled"
        EXIT_CODE=1
    else
        print_result 0 "Debug build with ASAN support found"
        ((PASS_COUNT++))
    fi
fi

# Subtest 2: Validate configuration file
next_subtest
print_subtest "Validate Configuration File"

CONFIG_PATH="${CONFIG_FILE}"
if [ ! -f "${CONFIG_PATH}" ]; then
    print_result 1 "Configuration file not found: ${CONFIG_PATH}"
    EXIT_CODE=1
else
    print_result 0 "Configuration file validated: ${CONFIG_PATH}"
    ((PASS_COUNT++))
fi

# Skip remaining tests if prerequisites failed
if [ ${EXIT_CODE} -ne 0 ]; then
    print_warning "Prerequisites failed - skipping memory leak test"
    
    # Export results for test_all.sh integration
    export_subtest_results "95_leaks_like_a_sieve" ${TOTAL_SUBTESTS} ${PASS_COUNT} > /dev/null
    
    # Print completion table
    print_test_completion "${TEST_NAME}"
    
    exit ${EXIT_CODE}
fi

# Subtest 3: Run memory leak detection test
next_subtest
print_subtest "Memory Leak Detection Test"

# Set up log files - always use build/tests/logs for consistency
LOG_DIR="${BUILD_DIR}/tests/logs"
mkdir -p "${LOG_DIR}"
SERVER_LOG="${LOG_DIR}/server_${TIMESTAMP}.log"
LEAK_REPORT="${LOG_DIR}/leak_report_${TIMESTAMP}.log"
LEAK_SUMMARY="${LOG_DIR}/leak_summary_${TIMESTAMP}.txt"

print_message "Starting memory leak test with debug build..."

# Start hydrogen server with comprehensive ASAN options
print_command "ASAN_OPTIONS=\"detect_leaks=1:leak_check_at_exit=1:verbosity=1:log_threads=1:print_stats=1\" ./${DEBUG_BUILD} ${CONFIG_PATH}"

ASAN_OPTIONS="detect_leaks=1:leak_check_at_exit=1:verbosity=1:log_threads=1:print_stats=1" ./"${DEBUG_BUILD}" "${CONFIG_PATH}" > "${SERVER_LOG}" 2>&1 &
HYDROGEN_PID=$!

# Wait for startup with timeout
print_message "Waiting for startup (max 5s)..."
STARTUP_START=$(date +%s)
STARTUP_TIMEOUT=5

while true; do
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - STARTUP_START))
    
    if [ ${ELAPSED} -ge ${STARTUP_TIMEOUT} ]; then
        print_result 1 "Startup timeout after ${ELAPSED}s"
        kill -9 ${HYDROGEN_PID} 2>/dev/null || true
        EXIT_CODE=1
        break
    fi
    
    if ! kill -0 ${HYDROGEN_PID} 2>/dev/null; then
        print_result 1 "Server crashed during startup"
        print_message "Server log contents:"
        while IFS= read -r line; do
            print_output "${line}"
        done < "${SERVER_LOG}"
        EXIT_CODE=1
        break
    fi
    
    if grep -q "Application started" "${SERVER_LOG}"; then
        print_message "Startup completed in ${ELAPSED}s"
        break
    fi
    
    # sleep 0.2
done

if [ ${EXIT_CODE} -eq 0 ]; then
    # Let it run briefly and perform some operations
    print_message "Running operations to trigger potential leaks..."
    # sleep 0.3

    # Try some API calls to trigger potential memory operations
    print_message "Making API calls to exercise memory allocation..."
    for _ in {1..3}; do
        curl -s "http://localhost:5030/api/system/info" > /dev/null 2>&1 || true
        # sleep 0.1
    done

    # Let memory operations settle
    # sleep 0.3

    # Send SIGTERM to trigger shutdown and leak detection
    print_message "Sending SIGTERM to trigger shutdown and leak detection..."
    kill -TERM ${HYDROGEN_PID}

    # Wait for process to exit
    wait ${HYDROGEN_PID} 2>/dev/null || true

    print_result 0 "Memory leak test execution completed"
    ((PASS_COUNT++))
else
    print_result 1 "Memory leak test execution failed"
fi

# Subtest 4: Analyze leak results
next_subtest
print_subtest "Analyze Leak Results"

if [ ${EXIT_CODE} -eq 0 ]; then
    # Check server.log for ASAN output
    print_message "Analyzing ASAN output for memory leaks..."
    if grep -q "LeakSanitizer" "${SERVER_LOG}"; then
        print_message "Found ASAN leak detection output"
        cp "${SERVER_LOG}" "${LEAK_REPORT}"
        
        # Analyze leak report
        print_message "Analyzing leak report for direct and indirect leaks..."

        # Check for direct leaks
        DIRECT_LEAKS=$(grep -c "Direct leak of" "${LEAK_REPORT}" 2>/dev/null | head -1 || echo "0")
        INDIRECT_LEAKS=$(grep -c "Indirect leak of" "${LEAK_REPORT}" 2>/dev/null | head -1 || echo "0")
        
        # Ensure we have clean integer values
        DIRECT_LEAKS=$(echo "${DIRECT_LEAKS}" | tr -d '\n\r' | grep -o '[0-9]*' | head -1)
        INDIRECT_LEAKS=$(echo "${INDIRECT_LEAKS}" | tr -d '\n\r' | grep -o '[0-9]*' | head -1)
        
        # Default to 0 if empty
        [ -z "${DIRECT_LEAKS}" ] && DIRECT_LEAKS=0
        [ -z "${INDIRECT_LEAKS}" ] && INDIRECT_LEAKS=0

        # Create summary
        {
            echo "Memory Leak Analysis Summary"
            echo "============================"
            echo "Direct Leaks Found: ${DIRECT_LEAKS}"
            echo "Indirect Leaks Found: ${INDIRECT_LEAKS}"
            echo ""
            
            if [ "${DIRECT_LEAKS}" -gt 0 ]; then
                echo "Direct Leak Details:"
                grep "Direct leak of" "${LEAK_REPORT}" | head -10 || true
                echo ""
            fi
            
            if [ "${INDIRECT_LEAKS}" -gt 0 ]; then
                echo "Indirect Leak Details:"
                grep "Indirect leak of" "${LEAK_REPORT}" | head -10 || true
                echo ""
            fi
            
            if [ "${DIRECT_LEAKS}" -eq 0 ] && [ "${INDIRECT_LEAKS}" -eq 0 ]; then
                echo "No memory leaks detected!"
            else
                echo "Memory leaks detected - see full report for details"
            fi
        } > "${LEAK_SUMMARY}"

        # Display summary
        print_message "Memory leak analysis results:"
        while IFS= read -r line; do
            # Only show lines that contain "Found"
            if [[ "${line}" == *"Found:"* ]]; then
                print_output "${line}"
            fi
        done < "${LEAK_SUMMARY}"

        # Copy results to results directory
        cp "${LEAK_REPORT}" "${RESULTS_DIR}/leak_report_${TIMESTAMP}.txt"
        cp "${LEAK_SUMMARY}" "${RESULTS_DIR}/leak_summary_${TIMESTAMP}.txt"

        # Determine test result
        if [ "${DIRECT_LEAKS}" -eq 0 ] && [ "${INDIRECT_LEAKS}" -eq 0 ]; then
            print_result 0 "No memory leaks detected"
            ((PASS_COUNT++))
        else
            print_result 1 "Memory leaks detected: ${DIRECT_LEAKS} direct, ${INDIRECT_LEAKS} indirect"
            EXIT_CODE=1
        fi
    else
        print_result 1 "No ASAN leak detection output found"
        print_message "Server log contents:"
        while IFS= read -r line; do
            print_output "${line}"
        done < "${SERVER_LOG}"
        EXIT_CODE=1
    fi
else
    print_result 1 "Leak analysis skipped due to previous failures"
fi

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" "${TEST_NAME}" > /dev/null

# Print completion table
print_test_completion "${TEST_NAME}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return ${EXIT_CODE}
else
    exit ${EXIT_CODE}
fi
