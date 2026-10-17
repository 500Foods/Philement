#!/usr/bin/env bash

# Test: QueryRef transaction blocks
# Drives QueryRefs 156, 157, and 158 through POST /api/conduit/alt_query
# on one server with eight demo connections (port 5640).
#
# Success responses put the statement array in "rows". Each entry has
# statement, block, rows, affected, success, and error. Statement numbers
# restart at 1 inside each block. Bare statements are blocks too, so the
# forced failure in #157 is "transaction block 4 failed at statement 3".
#
# A database error is HTTP 422. The body has success=false, error
# "Database error", and message set to the executor text. It does not
# include error_class or the statement array. #158's staging drop is the
# DDL observable: HTTP 422 block 3 statement 1 where the staging table
# rolled back, HTTP 200 with txn_id 1 still in the SELECT where DDL
# committed at the CREATE.
#
# FUNCTIONS
# txn_http()
# txn_payload()
# txn_check_156()
# txn_check_157_ok()
# txn_check_158_ok()
# txn_check_fail()
# txn_case()
# exercise_engine()

# CHANGELOG
# 1.0.3 - 2026-10-17 - Best-effort #158 before the cases, so a leftover table does not fail #156
# 1.0.2 - 2026-10-14 - Firebird parameter-free UPDATE reports affected 0
# 1.0.1 - 2026-10-14 - Sum statement affected counts; Yugabyte keeps staging after rollback
# 1.0.0 - 2026-10-14 - Initial blackbox for QueryRefs 156, 157, and 158

set -euo pipefail

# Test Configuration
TEST_NAME="QueryRef Transactions"
TEST_ABBR="QRT"
TEST_NUMBER="64"
TEST_COUNTER=0
TEST_VERSION="1.0.3"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
# shellcheck source=tests/lib/conduit_utils.sh # Conduit testing utilities
[[ -n "${CONDUIT_UTILS_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/conduit_utils.sh"
setup_test_environment

TXN_CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_64_queryref_transactions.json"
TXN_LOG_SUFFIX="queryref_transactions"
TXN_DESCRIPTION="QueryRef Transactions"
TXN_FAIL_157="transaction block 4 failed at statement 3 (rolled back)"
TXN_FAIL_158="transaction block 3 failed at statement 1 (rolled back)"
TXN_CASES_FAILED=0

# POST one alt_query call. Prints the HTTP status. Body lands in output_file.
txn_http() {
    local endpoint="$1"
    local jwt_token="$2"
    local payload="$3"
    local output_file="$4"
    local http_status

    http_status=$(curl -s -X POST \
        -H "Content-Type: application/json" \
        -H "Authorization: Bearer ${jwt_token}" \
        -d "${payload}" \
        -w "%{http_code}" \
        -o "${output_file}" \
        --compressed \
        --max-time 120 \
        "${endpoint}")
    printf '%s' "${http_status}"
}

# Build the alt_query JSON. params_json is a JSON object.
txn_payload() {
    local jwt_token="$1"
    local database="$2"
    local query_ref="$3"
    local params_json="$4"

    jq -n \
        --arg token "${jwt_token}" \
        --arg database "${database}" \
        --argjson query_ref "${query_ref}" \
        --argjson params "${params_json}" \
        '{token: $token, database: $database, query_ref: $query_ref, params: $params}'
}

# #156 success: CREATE, then one block (INSERT, UPDATE), then SELECT.
# The UPDATE has no parameters. Firebird execute_immediate reports
# affected 0 for it; the SELECT note is the proof it ran.
txn_check_156() {
    local body_file="$1"
    local run_label="$2"

    jq -e --arg label "${run_label}" '
        .success == true
        and (.rows | length) == 4
        and .rows[0].block == 1 and .rows[0].statement == 1 and .rows[0].success == true
        and .rows[1].block == 2 and .rows[1].statement == 1 and .rows[1].success == true and .rows[1].affected == 1
        and .rows[2].block == 2 and .rows[2].statement == 2 and .rows[2].success == true
        and .rows[3].block == 3 and .rows[3].statement == 1 and .rows[3].success == true
        and .row_count == 1
        and .affected_rows == ([.rows[].affected] | add)
        and ([.rows[3].rows[] | to_entries[] | select(.key | ascii_downcase == "txn_id") | (.value | tostring)] == ["1"])
        and ([.rows[3].rows[] | to_entries[] | select(.key | ascii_downcase == "note") | .value]
            == ["updated inside the same transaction block"])
        and ([.rows[3].rows[] | to_entries[] | select(.key | ascii_downcase == "run_label") | .value] == [$label])
    ' "${body_file}" >/dev/null
}

# #157 with SHOULD_FAIL=0: two transaction blocks, DDL between them, two committed rows.
txn_check_157_ok() {
    local body_file="$1"
    local run_label="$2"

    jq -e --arg label "${run_label}" '
        .success == true
        and (.rows | length) == 7
        and .rows[0].block == 1 and .rows[0].statement == 1 and .rows[0].success == true
        and .rows[1].block == 2 and .rows[1].statement == 1 and .rows[1].success == true and .rows[1].affected == 1
        and .rows[2].block == 3 and .rows[2].statement == 1 and .rows[2].success == true
        and .rows[3].block == 4 and .rows[3].statement == 1 and .rows[3].success == true
        and .rows[4].block == 4 and .rows[4].statement == 2 and .rows[4].success == true and .rows[4].affected == 1
        and .rows[5].block == 4 and .rows[5].statement == 3 and .rows[5].success == true and .rows[5].affected == 1
        and .rows[6].block == 5 and .rows[6].statement == 1 and .rows[6].success == true
        and .row_count == 2
        and ([.rows[6].rows[] | to_entries[] | select(.key | ascii_downcase == "txn_id") | (.value | tostring)] == ["1", "2"])
        and ([.rows[6].rows[] | to_entries[] | select(.key | ascii_downcase == "note") | .value]
            == ["row from block one", "row from block two"])
        and ([.rows[6].rows[] | to_entries[] | select(.key | ascii_downcase == "run_label") | .value] == [$label, $label])
    ' "${body_file}" >/dev/null
}

# #158 success. select_notes is a JSON array of the expected note strings, in txn_id order.
txn_check_158_ok() {
    local body_file="$1"
    local select_notes="$2"

    jq -e --argjson notes "${select_notes}" '
        .success == true
        and (.rows | length) == 3
        and .rows[0].block == 1 and .rows[0].statement == 1 and .rows[0].success == true
        and .rows[1].block == 2 and .rows[1].statement == 1 and .rows[1].success == true
        and .rows[2].block == 3 and .rows[2].statement == 1 and .rows[2].success == true
        and ([.rows[0].rows[] | to_entries[] | select(.key | ascii_downcase == "note") | .value] == $notes)
    ' "${body_file}" >/dev/null
}

# HTTP 422 body: executor message only. The statement array is not in this body.
txn_check_fail() {
    local body_file="$1"
    local expected_message="$2"

    jq -e --arg message "${expected_message}" '
        .success == false
        and .error == "Database error"
        and .message == $message
    ' "${body_file}" >/dev/null
}

# One subtest: post, require an HTTP status, then run the named checker.
# checker is 156, 157ok, 158ok, or fail.
txn_case() {
    local description="$1"
    local endpoint="$2"
    local jwt_token="$3"
    local database="$4"
    local query_ref="$5"
    local params_json="$6"
    local expected_status="$7"
    local checker="$8"
    local checker_arg="$9"
    local body_file="${10}"
    local payload http_status

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}"
    payload=$(txn_payload "${jwt_token}" "${database}" "${query_ref}" "${params_json}")
    http_status=$(txn_http "${endpoint}" "${jwt_token}" "${payload}" "${body_file}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP response code: ${http_status}"

    if [[ ! -f "${body_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - no response body"
        TXN_CASES_FAILED=$((TXN_CASES_FAILED + 1))
        return 1
    fi

    local message
    message=$(jq -r '.message // empty' "${body_file}" 2>/dev/null || true)
    if [[ -n "${message}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Message: ${message}"
    fi

    if [[ "${http_status}" != "${expected_status}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - expected HTTP ${expected_status}, got ${http_status}"
        TXN_CASES_FAILED=$((TXN_CASES_FAILED + 1))
        return 1
    fi

    local check_ok=0
    case "${checker}" in
        156)
            # shellcheck disable=SC2310 # Assertion result decides the single PASS/FAIL
            txn_check_156 "${body_file}" "${checker_arg}" || check_ok=1
            ;;
        157ok)
            # shellcheck disable=SC2310 # Assertion result decides the single PASS/FAIL
            txn_check_157_ok "${body_file}" "${checker_arg}" || check_ok=1
            ;;
        158ok)
            # shellcheck disable=SC2310 # Assertion result decides the single PASS/FAIL
            txn_check_158_ok "${body_file}" "${checker_arg}" || check_ok=1
            ;;
        fail)
            # shellcheck disable=SC2310 # Assertion result decides the single PASS/FAIL
            txn_check_fail "${body_file}" "${checker_arg}" || check_ok=1
            ;;
        *)
            check_ok=1
            ;;
    esac

    if [[ "${check_ok}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}"
        return 0
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} - response did not match the contract"
    TXN_CASES_FAILED=$((TXN_CASES_FAILED + 1))
    return 1
}

# Six cases on one engine. A not-ready engine is a failure: this test
# requires every connection in the config.
exercise_engine() {
    local db_engine="$1"
    local base_url="$2"
    local result_file="$3"
    local database="${DATABASE_NAMES[${db_engine}]}"
    local endpoint="${base_url}/api/conduit/alt_query"
    local token_map_name="JWT_TOKENS_BY_DATABASE_${TEST_NUMBER}"
    local jwt_token=""
    local ddl_mode="rollback"

    case "${db_engine}" in
        MySQL|MariaDB|Firebird|YugabyteDB) ddl_mode="commit" ;;
        *) ddl_mode="rollback" ;;
    esac

    if ! "${GREP}" -q "DATABASE_READY_${db_engine}=true" "${result_file}" 2>/dev/null; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} ready"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${db_engine} is not ready"
        TXN_CASES_FAILED=$((TXN_CASES_FAILED + 1))
        return 1
    fi

    # shellcheck disable=SC2086 # Token map name is a fixed TEST_NUMBER suffix
    eval "jwt_token=\${${token_map_name}[\"${db_engine}\"]:-}"
    if [[ -z "${jwt_token}" ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${db_engine} JWT"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${db_engine} has no JWT"
        TXN_CASES_FAILED=$((TXN_CASES_FAILED + 1))
        return 1
    fi

    local label_156="t64-${db_engine}-smoke"
    local label_ok="t64-${db_engine}-commit"
    local label_fail="t64-${db_engine}-rollback"
    local params_156 params_ok params_fail preclean_payload
    params_156=$(jq -n --arg label "${label_156}" '{STRING: {RUN_LABEL: $label}}')
    params_ok=$(jq -n --arg label "${label_ok}" '{STRING: {RUN_LABEL: $label}, INTEGER: {SHOULD_FAIL: 0}}')
    params_fail=$(jq -n --arg label "${label_fail}" '{STRING: {RUN_LABEL: $label}, INTEGER: {SHOULD_FAIL: 1}}')

    # DDL that commits on its own (YugabyteDB, and MySQL-family CREATE)
    # can leave TEST_TRANSACTIONS behind when a run stops early. #158
    # drops that table before it attempts TEST_TXN_STAGING, so one
    # ignored call gives #156 an empty slate. A missing table is fine.
    preclean_payload=$(txn_payload "${jwt_token}" "${database}" 158 '{}')
    # shellcheck disable=SC2310 # HTTP 422 from a missing table is a clean slate, not a case failure
    txn_http "${endpoint}" "${jwt_token}" "${preclean_payload}" \
        "${result_file}.${db_engine}.preclean.json" >/dev/null || true

    # shellcheck disable=SC2310 # A failed case is recorded; later cases still run so #158 can clean up
    txn_case "${db_engine} #156 smoke" "${endpoint}" "${jwt_token}" "${database}" 156 \
        "${params_156}" 200 156 "${label_156}" \
        "${result_file}.${db_engine}.156.json" || true

    # shellcheck disable=SC2310 # Cleanup still runs when the smoke case failed
    txn_case "${db_engine} #158 after #156" "${endpoint}" "${jwt_token}" "${database}" 158 \
        '{}' 422 fail "${TXN_FAIL_158}" \
        "${result_file}.${db_engine}.158a.json" || true

    # shellcheck disable=SC2310 # A failed case is recorded; the matching cleanup still runs
    txn_case "${db_engine} #157 commit" "${endpoint}" "${jwt_token}" "${database}" 157 \
        "${params_ok}" 200 157ok "${label_ok}" \
        "${result_file}.${db_engine}.157ok.json" || true

    # shellcheck disable=SC2310 # Cleanup still runs when the commit case failed
    txn_case "${db_engine} #158 after commit" "${endpoint}" "${jwt_token}" "${database}" 158 \
        '{}' 200 158ok '["row from block one","row from block two"]' \
        "${result_file}.${db_engine}.158b.json" || true

    # shellcheck disable=SC2310 # A failed case is recorded; the matching cleanup still runs
    txn_case "${db_engine} #157 rollback" "${endpoint}" "${jwt_token}" "${database}" 157 \
        "${params_fail}" 422 fail "${TXN_FAIL_157}" \
        "${result_file}.${db_engine}.157fail.json" || true

    if [[ "${ddl_mode}" == "commit" ]]; then
        # shellcheck disable=SC2310 # Last case of this engine; the result is already recorded
        txn_case "${db_engine} #158 after rollback (DDL committed)" "${endpoint}" "${jwt_token}" "${database}" 158 \
            '{}' 200 158ok '["row from block one"]' \
            "${result_file}.${db_engine}.158c.json" || true
    else
        # shellcheck disable=SC2310 # Last case of this engine; the result is already recorded
        txn_case "${db_engine} #158 after rollback (DDL rolled back)" "${endpoint}" "${jwt_token}" "${database}" 158 \
            '{}' 422 fail "${TXN_FAIL_158}" \
            "${result_file}.${db_engine}.158c.json" || true
    fi
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Environment Variables"
env_vars_valid=true
if [[ -z "${HYDROGEN_DEMO_USER_NAME:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_USER_NAME is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_USER_PASS:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_USER_PASS is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_API_KEY:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_API_KEY is not set"
    env_vars_valid=false
fi
if ! command -v jq >/dev/null 2>&1; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: jq is not installed"
    env_vars_valid=false
fi

if [[ "${env_vars_valid}" == true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Required environment variables are set"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing jq or demo credentials"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${TXN_CONFIG_FILE}"; then
    port=$(get_webserver_port "${TXN_CONFIG_FILE}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${TXN_DESCRIPTION} configuration will use port: ${port}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    EXIT_CODE=1
fi

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    result_file="${LOG_PREFIX}${TIMESTAMP}_${TXN_LOG_SUFFIX}.result"
    server_info=$(run_conduit_server "${TXN_CONFIG_FILE}" "${TXN_LOG_SUFFIX}" "${TXN_DESCRIPTION}" "${result_file}")

    if [[ "${server_info}" == "FAILED:0" ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen did not become ready"
        EXIT_CODE=1
    else
        server_line=$(printf '%s\n' "${server_info}" | tail -n 1)
        base_url=$(printf '%s\n' "${server_line}" | awk -F: '{print $1":"$2":"$3}')
        hydrogen_pid=$(printf '%s\n' "${server_line}" | awk -F: '{print $4}')
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server log location: build/tests/logs/test_64_${TIMESTAMP}_${TXN_LOG_SUFFIX}.log"

        acquire_jwt_tokens "${base_url}" "${result_file}"

        for db_engine in PostgreSQL MySQL SQLite DB2 MariaDB Firebird YugabyteDB MSSQL; do
            # shellcheck disable=SC2310 # One engine's failure must not skip the remaining engines
            exercise_engine "${db_engine}" "${base_url}" "${result_file}" || true
        done

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${TXN_DESCRIPTION}: All engines"
        if [[ "${TXN_CASES_FAILED}" -eq 0 ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${TXN_DESCRIPTION}: all engine cases passed"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${TXN_DESCRIPTION}: ${TXN_CASES_FAILED} case(s) failed"
            EXIT_CODE=1
        fi

        shutdown_conduit_server "${hydrogen_pid}" "${result_file}"
    fi
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping QueryRef transaction cases because a prerequisite failed"
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
