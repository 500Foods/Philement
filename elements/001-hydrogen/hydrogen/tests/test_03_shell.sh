#!/usr/bin/env bash

# Test: Shell Environment Variables
# Tests environment variables used in the Hydrogen project
# Validates that required and optional environment variables are properly configured

# CHANGELOG
# 1.0.0 - 2025-12-06 - Initial version - Environment variable validation for shell configuration

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable" 
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable"
    echo "Please set HELIUM_ROOT to the Helium project root directory"
    exit 1
fi

set -euo pipefail

# Test configuration
TEST_NAME="Shell Variables"
TEST_ABBR="ZSH"
TEST_NUMBER="03"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "${HYDROGEN_ROOT}/tests/lib/framework.sh"
setup_test_environment

# Source the get_vars.sh script for shared variable extraction functions
# shellcheck disable=SC1091 # Can't resolve path statically
source "${HYDROGEN_ROOT}/tests/lib/get_vars.sh"

# Whitelist of environment variables to ignore (handled by framework or used by scripts internally)
declare -a ENV_WHITELIST=(
    # Commands
    "ECHO" "PRINTF" "DATE" "MKDIR" "RM" "GREP" "FIND" "MD5SUM" "LANG"
    "STAT" "SED" "AWK" "CUT" "TR" "WC" "BASENAME" "DIRNAME" "OH" "JQ"
    "NC" "XARGS" "NPROC" "SORT" "UNIQ" "HEAD" "TAIL" "TABLES" "HOME" "RANDOM"
    "REALPATH" "PWD" "DEBUG" "STRACE" "GIT" "CURL" "TAR" "TIMEOUT" "MUTT_CMD"
    # Common directory variables
    "BUILD_DIR" "LIB_DIR" "DIAGS_DIR" "DIAG_TEST_DIR" "PROJECT_DIR" "TEST_DIR"  "CONFIG_DIR"   
    "LOGS_DIR" "HYDROGEN_BIN" "HYDROGEN_BIN_BASE" "RESULTS_DIR" "SCRIPT_DIR" "WEBROOT"
    # First found in tests/lib/framework.sh
    "EPOCHREALTIME" "FRAMEWORK_NAME" "FRAMEWORK_VERSION" "LOG_OUTPUT_NAME" "LOG_OUTPUT_VERSION"  
    "TESTS_DIR" "TEST_ABBR" "TEST_COUNTER" "TEST_NAME" "TEST_NUMBER" "TEST_START_TIME" "TEST_VERSION"
    "TIMESTAMP" "CURRENT_TEST_NUMBER"
    # First found in tests/lib/log_ouptput.sh
    "COLLECT_OUTPUT_MODE" "DATA_COLOR" "DATA_ICON" "EXEC_COLOR" "EXEC_ICON" "FAIL_COLOR" "FAIL_ICON"
    "INFO_COLOR" "INFO_ICON" "OUTPUT_COLLECTION" "PASS_COLOR" "PASS_ICON" "TAB_DATA_FOOTER" "TAB_DATA_HEADER"     
    "TAB_LAYOUT_FOOTER" "TAB_LAYOUT_HEADER" "TEST_COLOR" "TEST_FAILED_COUNT" "TEST_ICON" "TEST_PASSED_COUNT"
    "WARN_COLOR" "WARN_ICON" 
    # First found in tests/lib/env_utils.sh
    "ENV_UTILS_NAME" "ENV_UTILS_VERSION" 
    # First found in tests/lib/file_utils.sh
    "FILE_UTILS_NAME" "FILE_UTILS_VERSION"  
    # First found in tests/lib/network_utils.sh
    "NETWORK_UTILS_NAME" "NETWORK_UTILS_VERSION"     
    # First found in tests/lib/get_vars.sh
    "ALL_VARS" "SEARCH_JSON" "SEARCH_SHELL" "TARGET_PATH" "VARIABLE" 
    # First found in tests/lib/lifecycle.sh
    "LIFECYCLE_NAME" "LIFECYCLE_VERSION" "LOG_PREFIX"   
    # First found in tests/lib/cloc.sh
    "CLOC" "CLOC_NAME" "CLOC_VERSION" "DATE" "FRAMEWORK_GUARD"  
    "TIMESTAMP_DISPLAY" 
    # First found in tests/lib/cloc_tables.sh
    "CLOC_DATA" "CLOC_OUTPUT"
    # First found in tests/lib/coverage.sh
    "BLACKBOX_COVERAGE_FILE" "COMBINED_COVERAGE_FILE" "CORES" "COVERAGE_NAME" "COVERAGE_VERSION" 
    "OVERLAP_COVERAGE_FILE" "UNITY_COVERAGE_FILE"                                                
    # First found in tests/lib/coverage_table.sh
    "BLACKBOX_COVS" "CACHE_DIR" "COVERAGE_TABLE_NAME" "COVERAGE_TABLE_VERSION" 
    "UNITY_COVERAGE_FILE" "UNITY_COVS" 
    # First found in tests/lib/coverage-combined.sh
    "COVERAGE_COMBINED_NAME" "COVERAGE_COMBINED_VERSION" "COVERAGE_COMMON_GUARD"    
    # First found in tests/lib/coverage-common.sh
    "IGNORE_PATTERNS_LOADED" "SOURCE_FILE_CACHE_LOADED" "COVERAGE_COMMON_NAME" "COVERAGE_COMMON_VERSION"
    # First found in tests/lib/get_diagram.sh
    "AFTER_DATA" "BASE64_CLEAN" "BASE64_CONTENT" "BASE64_LENGTH" "BASE64_TEST" "BEFORE_DATA"
    "CLEAN_LENGTH" "COMBINED_JSON" "DECODED_DATA" "DECODE_STATUS" "DECOMPRESSED_SIZE"
    "DECOMPRESS_STATUS" "DESIGN" "DESIGN_DIR" "ENGINE" "ESCAPED_ALREADY" "HAS_BROTLI" "HELIUM_DIR"
    "JSON_DATA" "JSON_INGEST_END" "JSON_INGEST_START" "METADATA" "METADATA_JSON" "METADATA_LINE"     
    "METADATA_TMP" "MIGRATION" "MIGRATION_FILE" "PREFIX" "RAW_BLOCK" "SQL_OUTPUT" "SVG_OUTPUT"
    "TEMP_AFTER_JSON" "TEMP_BEFORE_JSON" 
    # First found in tests/lib/github-sitemap.sh
    "APPVERSION" "HELP" "IGNORE_FILE" "IGNORE_PATTERNS_SERIALIZED" "INPUT_FILE"     
    "NOREPORT" "QUIET" "TABLE_THEME"     
    # First found in tests/profile_test_suite.sh
    "AWK_COUNT" "BASENAME_COUNT" "BASH_COUNT" "BC_COUNT" "CAT_COUNT" "CC_COUNT" "CLOC_COUNT" "CMAKE_COUNT"     
    "CPPCHECK_COUNT" "CURL_COUNT" "DATE_COUNT" "DEFAULT_SCRIPT" "DIRNAME_COUNT" "DU_COUNT" "ERROR_LOG"     
    "ESLINT_COUNT" "FIND_COUNT" "FORKS" "GCOV_COUNT" "GREP_COUNT" "HTMLHINT_COUNT" "HYDROGEN_COUNT"
    "JSONLINT_COUNT" "MAKE_COUNT" "MARKDOWNLINT_COUNT" "MD5_COUNT" "MKDIR_COUNT" "MKTEMP_COUNT" 
    "REALPATH_COUNT" "RM_COUNT" "SED_COUNT" "SHELLCHECK_COUNT" "SH_COUNT" "STRACE_STATUS" "STYLELINT_COUNT"
    "SUMMARY_OUT" "TABLES_COUNT" "TEST_SCRIPT" "TRACE_OUT" "TR_COUNT" "WC_COUNT" "XARGS_COUNT" 
    # First found in tests/test_00_all.sh
    "BLACKBOX_COVERAGE" "BUILD_NUMBER" "CACHE_CMD_DIR" "COMBINED_COVERAGE" "COVERAGE" "END_TIME"
    "OVERALL_EXIT_CODE" "PATH" "SCRIPT_SCALE" "SEQUENTIAL_MODE" "SKIP_TESTS" "START_TIME"
    "TOTAL_ELAPSED" "TOTAL_ELAPSED_FORMATTED" "TOTAL_FAILED" "TOTAL_PASSED" "TOTAL_RUNNING_TIME"
    "TOTAL_RUNNING_TIME_FORMATTED" "TOTAL_TESTS" "UNITY_COVERAGE" "UNITY_FAILURES_FILE" "VERSION_FULL" 
    # First found in tests/test_01_compilation.sh
    "BUILD_TIMESTAMP_FILE" "CMAKE_CONFIG_STAMP" "CMAKE_DIR" "EXIT_CODE" "PAYLOAD_DIR"     
    "SRC_DIR" "UNITY_SRC_DIR" 
    # First found in tests/test_03_shell.sh
    "ALL_FOUND_VARS" "CONFIG_VARS" "EXTRAS_VARS" "SCRIPT_VARS" "PAYLOAD_VARS" "WHITELIST_COUNT"
    "RELATIVE_LINK_COUNT"
    # First found in tests/test_04/check_links.sh
    "ISSUES_FOUND" "MARKDOWN_CHECK" "MISSING_LINKS_COUNT" "ORPHANED_FILES_COUNT" "SITEMAP"     
    "SITEMAP_EXIT_CODE" "TARGET_README" "TOTAL_EXTRACTED_ISSUES" "TOTAL_LINKS" 
    # First found in tests/test_10_unity.sh
    "CACHE_HITS" "CACHE_MISSES" "LONG_RUNNING" "SECONDS" "TEST_TIMEOUT" "TOTAL_LONG_RUNNING"     
    "TOTAL_UNITY_PASSED" "TOTAL_UNITY_TESTS" "UNITY_BUILD_DIR" "UNITY_CACHE_DIR"    
    "UNITY_CACHE_FILE" "UNITY_FAILURES_FILE" 
    # First found in tests/test_11_leaks_like_a_sieve.sh
    "DEBUG_BUILD" "DIRECT_LEAKS" "HYDROGEN_PID" "INDIRECT_LEAKS" "LEAK_REPORT" "LEAK_SUMMARY"     
    "SERVER_LOG" "STARTUP_TIMEOUT" "ELAPSED"
    # First found in tests/test_12_env_variables.sh
    "SHUTDOWN_ACTIVITY_TIMEOUT" "SHUTDOWN_TIMEOUT" "LOG_FILE"
    # First found in tests/test_13_crash_handler.sh
    "CORE_FILE_RESULT" "CORE_LIMIT" "CORE_PATTERN" "CRASH_LOG_RESULT" "CRASH_TIMEOUT" "DEBUG_SYMBOL_RESULT"     
    "GDB_ANALYSIS_RESULT" "MAX_JOBS" "ORCHESTRATION"    
    # First found in tests/test_15_json_error_handling.sh
    "ERROR_OUTPUT"  
    # First found in tests/test_16_shutdown.sh
    "MIN_CONFIG"
    # First found in tests/test_17_startup_shutdown.sh
    "LOG_MIN" "LOG_MAX" "MAX_CONFIG"
    # First found in tests/test_18_signals.sh
    "RESTART_COUNT" "SIGNAL_TIMEOUT" "TEST_CONFIG_BASE" "TEST_CONFIG"
    # First found in tests/test_19_socket_rebind.sh
    "BASE_URL" "CONFIG_FILE" "FIRST_LOG" "FIRST_PID" "PORT" "SECOND_LOG" "SECOND_PID" 
    # First found in tests/test_22_swagger.sh
    "CUSTOM_HEADERS_TEST_RESULT"
    # First found in tests/test_93_jsonlint.sh
    "CONFIG_COUNT" "JSONSCHEMA_CLI" "SCHEMA_FILE"
    # Remaining tests    
    "BLACKBOX_COVERAGE_DIR" "CACHED_VALIDATIONS" "CAPTURE_LOG" "CLOC_EXIT_CODE" "CLOC_PID" "CODE_LINES" 
    "CONFIG_1" "CONFIG_2" "CONFIG_PATH" "CONTENT_TEST_RESULT" "CPPCHECK_OUTPUT" "CROSS_CONFIG_404_TEST_RESULT" 
    "CSS_COUNT" "CURL_FORMAT" "C_COUNT" "C_COUNT_FMT" "DQM_INIT_TIMEOUT" "ENGINE_NAME" "ENGINE_REF" "EXPECTED_ERRORS" 
    "EXPECTED_WARNING" "FAILED_GENERATIONS" "FAILED_VALIDATIONS" "FILES_COUNT" "FILES_TO_RUN" "FILES_WITH_ISSUES" 
    "FILTERED_LOG" "FILTER_LOG" "HAS_EXPECTED" "HAS_OVERSIZED" "HEALTH_TEST_RESULT" "HTML_COUNT" "INDEX_TEST_RESULT" 
    "INFO_TEST_RESULT" "INTENTIONAL_ERROR_FILE" "ISSUE_COUNT" "JAVASCRIPT_TEST_RESULT" "JSON_COUNT" "JSON_ISSUES" 
    "JS_COUNT" "LARGEST_FILE" "LARGEST_LINES" "LARGE_FILES_LIST" "LARGE_FILE_COUNT" "LARGE_FILE_THRESHOLD" 
    "LINE_COUNT_FILE" "LINT_OUTPUT_LIMIT" "LOCCOUNT" "LOG_LINE_PATTERN" "LUACHECK_OUTPUT" "LUA_COUNT" 
    "MAX_SOURCE_LINES" "MD_COUNT" "NC_PID" "OTHER_ISSUES" "PACKET_LOG" "PASSED_VALIDATIONS" "REDIRECT_TEST_RESULT" 
    "SERVER_PORT" "SERVER_READY" "SHELLCHECK_CACHE_DIR" "SHELLCHECK_DIRECTIVE_TOTAL" 
    "SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION" "SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION" "SHELL_COUNT" 
    "SHELL_ISSUES" "SOURCE_FILES_LIST" "SPECIFIC_FILE_TEST_RESULT" "STATS" "SUCCESSFUL_GENERATIONS" 
    "SWAGGER_JSON_TEST_RESULT" "TCAP_PID" "TEMP_LOG" "TEMP_NEW_LOG" "TEMP_OUTPUT" "TEST_TEST_RESULT" 
    "TOPLIST" "TOTAL_COMBINATIONS" "TOTAL_DESIGNS" "TOTAL_FILES" "TOTAL_MIGRATIONS" "TRACED_LOG" 
    "TRAILING_SLASH_TEST_RESULT" "VALIDATION_DURATION" "VALIDATION_END_TIME" "VALIDATION_START_TIME" 
    "WEBSOCKET_KEY_VALIDATED" "XML_COUNT" 
    # First found in extras
    "ALL_SRCS" "BUILD_CMD" "BUILD_OUTPUT" "ELAPSED_TIME" "EMAIL_BODY" "ERRORS" "EXECUTABLE_PATH"
    "FORMATTED_FAILED" "FORMATTED_PASSED" "IGNORED_SRCS" "MAP_FILE" "METRICS_JSON" "MOCKS_C_INCLUDES"
    "QUICK_MODE" "REL_PATH" "REPORTABLE_SRCS" "SUBJECT" "TEST_EXIT_CODE" "TEST_RELATIVE_PATH"     
    "TEST_SOURCE_FILE" "TOTAL_FAILED" "TOTAL_PASSED" "TRIMMED_ELAPSED_TIME" "UNITY_BUILD_DIR"     
    "UNITY_BUILD_OUTPUT" "UNITY_C_INCLUDES" "UNITY_ERRORS" "UNITY_SRC_DIR" "UNUSED_SRCS" "USED_OBJS"
    "USED_SRCS" "VERSION_FULL" "TEMP_DIR" "HYDROGEN_DIR" "PROJECT_ROOT"
    # First found in tests/artifacts
    "SQL_FILE" "TEMP_CREATE" "TEMP_TEST" 
    # First found in payloads/payload-generate.sh
    "BLUE" "BOLD" "COMPRESSED_TAR_FILE" "CYAN" "EXTRACTED_DIR" "FAIL" "GREEN" "INFO" "MAGENTA" "PASS"
    "PATH1" "PATH2" "PATH3" "RED" "SWAGGERUI_DIR" "SWAGGERUI_VERSION" "TAR_FILE" "WARN" "XTERMJS_DIR" "YELLOW" 
    # First found in payloads/swagger-generate.sh
    "API_DIR" "OUTPUT_FILE" "PATH4" "PATHS_FILE" "RED" "TAGS_FILE"     
    # First found in payloads/terminal-generate.sh
    "ARTIFACTS_DIR" "XTERMJS_DIR" "XTERMJS_VERSION_FILE" "XTERM_ATTACH_VERSION" "XTERM_FIT_VERSION" "XTERM_JS_VERSION" 
    # First found in extras/hm_browser.sh
    "DEFAULT_CONFIG" "DEFAULT_OUTPUT" "OUTPUT_DIR"

)

# Environment variables array
# Format: name|tests|brief_desc|long_desc
declare -a ENV_VARS=(
    # Needed for project build
    "PHILEMENT_ROOT|all|Hydrogen project root|Root directory path of the Hydrogen project. Required for all test scripts and build processes."
    "HYDROGEN_ROOT|all|Hydrogen project root|Root directory path of the Hydrogen project. Required for all test scripts and build processes."
    "HYDROGEN_DOCS_ROOT|all|Hydrogen project root|Root directory path of the Hydrogen project. Required for all test scripts and build processes."
    "HELIUM_ROOT|all|Helium project root|Root directory path of the Helium project. Required for payload generation and cross-project operations."
    "HELIUM_DOCS_ROOT|all|Helium project root|Root directory path of the Helium project. Required for payload generation and cross-project operations."
                              
    # Key project parameters
    "HYDROGEN_DEV_EMAIL|02,12|Developer email|The email address of the developer responsible for this Hydrogen instance. Used for notifications and support contact."
                            
    # Security keys
    "HYDROGEN_INSTALLER_LOCK|02|Payload encryption key|RSA private key used for encrypting payload data. Must be base64 encoded and valid RSA format."
    "HYDROGEN_INSTALLER_KEY|02|Payload encryption key|RSA private key used for encrypting payload data. Must be base64 encoded and valid RSA format."
    "PAYLOAD_LOCK|02,12|Payload encryption key|RSA private key used for encrypting payload data. Must be base64 encoded and valid RSA format."
    "PAYLOAD_KEY|02,12|Payload encryption key|RSA private key used for encrypting payload data. Must be base64 encoded and valid RSA format."
    "WEBSOCKET_KEY|02,12|WebSocket authentication key|Key used for WebSocket connections and authentication."
    "JWT_SECRET|12|JWT signing secret|Secret key used for signing JSON Web Tokens in API authentication."    
                   
    # MySQL demo
    "CANVAS_DB_HOST|30-35|Canvas database host|Hostname or IP address of the Canvas database server."
    "CANVAS_DB_NAME|30-35|Canvas database name|Name of the Canvas database."
    "CANVAS_DB_USER|30-35|Canvas database user|Username for connecting to the Canvas database."
    "CANVAS_DB_PASS|30-35|Canvas database password|Password for connecting to the Canvas database."
    "CANVAS_DB_PORT|30-35|Canvas database port|Port number for the Canvas database server."
    "CANVAS_DB_TYPE|30-35|Canvas database type|Type of the Canvas database (e.g., sqlite)."

    # PostgreSQL demo
    "ACURANZO_DB_HOST|30-35|Acuranzo database host|Hostname or IP address of the Acuranzo database server."
    "ACURANZO_DB_NAME|30-35|Acuranzo database name|Name of the Acuranzo database."
    "ACURANZO_DB_USER|30-35|Acuranzo database user|Username for connecting to the Acuranzo database."
    "ACURANZO_DB_PASS|30-35|Acuranzo database password|Password for connecting to the Acuranzo database."
    "ACURANZO_DB_PORT|30-35|Acuranzo database port|Port number for the Acuranzo database server."
    "ACURANZO_DB_TYPE|30-35|Acuranzo database type|Type of the Acuranzo database (e.g., postgres)."

    # DB2 demo
    "HYDROTST_DB_NAME|30-35|Acuranzo database name|Name of the Acuranzo database."
    "HYDROTST_DB_USER|30-35|Acuranzo database user|Username for connecting to the Acuranzo database."
    "HYDROTST_DB_PASS|30-35|Acuranzo database password|Password for connecting to the Acuranzo database."
    "HYDROTST_DB_TYPE|30-35|Acuranzo database type|Type of the Acuranzo database (e.g., postgres)."

)

# Function to check if environment variable is set and not empty
check_env_var() {
    local var_name="$1"
    local var_value="${!var_name:-}"
    [[ -n "${var_value}" ]]
}

# Function to get env var info from array
get_env_info() {
    local var_name="$1"
    local info_type="$2"  # tests, brief, long
    for entry in "${ENV_VARS[@]}"; do
        IFS='|' read -r name tests brief long <<< "${entry}"
        if [[ "${name}" == "${var_name}" ]]; then
            case "${info_type}" in
                "tests") echo "${tests}" ;;
                "brief") echo "${brief}" ;;
                "long") echo "${long}" ;;
                *) return 1 ;;
            esac
            return 0
        fi
    done
    return 1
}

# Check defined environment variables
for entry in "${ENV_VARS[@]}"; do
    IFS='|' read -r var_name tests brief_desc long_desc <<< "${entry}"
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${brief_desc} (${var_name})"
    
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if check_env_var "${var_name}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${var_name} is set"
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${long_desc}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${var_name} is not set or is empty"
        EXIT_CODE=1
    fi
done

# Function to check if variable is in whitelist
is_whitelisted() {
    local var_name="$1"
    for wl_var in "${ENV_WHITELIST[@]}"; do
        if [[ "${wl_var}" == "${var_name}" ]]; then
            return 0
        fi
    done
    return 1
}


# Search for additional environment variables in project configs and scripts
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Search for additional environment variables in project"

# Get all env vars from project files using the shared get_vars functions
CONFIG_VARS=$(extract_env_vars_from_files "${PROJECT_DIR}/tests/configs" "json_only")    # JSON only for configs
SCRIPT_VARS=$(extract_env_vars_from_files "${PROJECT_DIR}/tests" "shell_only")           # Shell only for tests
EXTRAS_VARS=$(extract_env_vars_from_files "${PROJECT_DIR}/extras" "shell_only")         # Shell only for extras
PAYLOAD_VARS=$(extract_env_vars_from_files "${PROJECT_DIR}/payloads" "shell_only")      # Shell only for payloads

# Combine and deduplicate
ALL_FOUND_VARS=$(echo -e "${CONFIG_VARS}\n${SCRIPT_VARS}\n${EXTRAS_VARS}\n${PAYLOAD_VARS}" | grep -v '^$' | sort | uniq)

# Check each found var
for var_name in ${ALL_FOUND_VARS}; do
    # Skip if already in our defined list
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if get_env_info "${var_name}" "tests" >/dev/null 2>&1; then
        continue
    fi

    # Skip if whitelisted
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if is_whitelisted "${var_name}"; then
        continue
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Additional env var: ${var_name}"

    # Find which file contains this var (could be config or script)
    found_line=$(grep -rn "\${env\.${var_name}}" "${PROJECT_DIR}/tests/configs" 2>/dev/null | head -1 || true)
    if [[ -z "${found_line}" ]]; then
        found_line=$(grep -rn "\${${var_name}}" "${PROJECT_DIR}/tests" "${PROJECT_DIR}/extras" "${PROJECT_DIR}/payloads" --include="*.sh" 2>/dev/null | head -1 || true)
    fi
    if [[ -n "${found_line}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found at ${found_line}"
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Environment variable ${var_name} is referenced in project files."
    fi
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${var_name} is not documented in ENV_VARS array"
    EXIT_CODE=1
done

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Search complete"

# Count whitelist variables
WHITELIST_COUNT=${#ENV_WHITELIST[@]}

# Update test name to include whitelist count
if [[ "${WHITELIST_COUNT}" -gt 0 ]]; then
    TEST_NAME="Shell Variables  {BLUE}whitelist: $("${PRINTF}" "%'d" "${WHITELIST_COUNT}" || true) vars{RESET}"
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone                
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"

