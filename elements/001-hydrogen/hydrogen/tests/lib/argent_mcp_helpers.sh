#!/usr/bin/env bash

# Argent MCP tool blackbox helpers for tests/test_73_argent_mcp.sh.
# One case per tool call. Errors are structuredContent.code, HTTP 200.
# Local variable names stay lowercase so test_03 does not require them.
#
# This file is the coordinator: it defines the framework helpers
# (argent_prepare_sqlite, argent_miss, argent_rpc, argent_call, argent_ok,
# argent_fail, argent_confirm, argent_grab) and argent_mcp_exercise, which
# delegates the per-tool exercise blocks to the argent_helpers_*.sh files
# below. Each sub-file stays under the project's 1,000-line cap.

# shellcheck disable=SC2154 # TEST_NUMBER and the MCP helpers come from the caller
# shellcheck disable=SC2312 # jq and curl status are checked by the caller

# CHANGELOG
# 1.0.0 - 2026-10-07 - Argent tool calls and validation variants
# 1.0.1 - 2026-10-07 - Quote the contact_not_found arguments so jq accepts them
# 1.0.2 - 2026-10-07 - One tool call, root error text, tax rate in force, parent rate check
# 1.0.3 - 2026-10-08 - Confirm tokens, edits, rescinds, statements, and reconciliation
# 1.0.4 - 2026-10-08 - Retry one HTTP 503 (auth lookup timed out)
# 1.0.5 - 2026-10-08 - Schedules, reserved rows, and a down calendar host
# 1.0.6 - 2026-10-08 - Report queries, manual rates, and offline BoC checks
# 1.0.7 - 2026-10-08 - Accept an empty income warnings object
# 1.0.8 - 2026-10-09 - Split exercise blocks into argent_helpers_*.sh for code-size cap

[[ -n "${ARGENT_MCP_HELPERS_GUARD:-}" ]] && return 0
export ARGENT_MCP_HELPERS_GUARD="true"

ARGENT_MCP_HELPERS_NAME="Argent MCP Helpers"
ARGENT_MCP_HELPERS_VERSION="1.0.8"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
    "${ARGENT_MCP_HELPERS_NAME} ${ARGENT_MCP_HELPERS_VERSION}" "info"

# shellcheck source=tests/lib/argent_helpers_tools_orgs.sh # tools/list + org CRUD
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_tools_orgs.sh"
# shellcheck source=tests/lib/argent_helpers_ledgers.sh # Ledger upserts, gets, listings
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_ledgers.sh"
# shellcheck source=tests/lib/argent_helpers_terms_contacts.sh # Ledger terms and contacts
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_terms_contacts.sh"
# shellcheck source=tests/lib/argent_helpers_txn.sh # Transactions, tax codes/rates
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_txn.sh"
# shellcheck source=tests/lib/argent_helpers_tags_att.sh # Tags, attachments, txn-get
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_tags_att.sh"
# shellcheck source=tests/lib/argent_helpers_balances.sh # QueryBalances cases
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_balances.sh"
# shellcheck source=tests/lib/argent_helpers_recon.sh # Edit, rescind, statement, reconciliation
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_recon.sh"
# shellcheck source=tests/lib/argent_helpers_schedules.sh # Schedules, matches, retries
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_schedules.sh"
# shellcheck source=tests/lib/argent_helpers_queries_rates.sh # Query tools, rates, BoC
source "$(dirname "${BASH_SOURCE[0]}")/argent_helpers_queries_rates.sh"

argent_prepare_sqlite() {
    local src_config="$1"
    local work_dir="$2"
    local out_config="${work_dir}/config.json"
    local db_copy="${work_dir}/hydrodemo.sqlite"
    local seed_n
    mkdir -p "${work_dir}"
    if declare -f sqlite_online_backup >/dev/null 2>&1; then
        sqlite_online_backup "${BASELINE_SQLITE}" "${db_copy}" || return 1
    else
        sqlite3 -batch -init /dev/null "${BASELINE_SQLITE}" ".backup '${db_copy}'" || return 1
    fi
    seed_n=$(sqlite3 -batch -init /dev/null "${db_copy}" \
        "SELECT COUNT(*) FROM scripts WHERE group_name='Mcp' AND script_name='Server' AND mcp_access<>0;" \
        2>/dev/null || echo 0)
    if [[ "${seed_n}" -lt 1 ]]; then
        return 1
    fi
    jq --arg db "${db_copy}" '
        .Databases.Connections |= map(
            if ((.Engine // "") | ascii_downcase) == "sqlite" then
                .Database = $db | .AutoMigration = true
            else . end
        )
    ' "${src_config}" > "${out_config}"
    printf '%s\n' "${out_config}"
}

argent_miss() {
    local case_name="$1"
    ARGENT_CASE_N=$(( ARGENT_CASE_N + 1 ))
    record_case "${ARGENT_RESULT}" "${case_name}" 0
    echo "FAIL_${case_name}=prereq" >> "${ARGENT_RESULT}"
}

argent_rpc() {
    local case_name="$1"
    local payload="$2"
    local filter="$3"
    local http_st out code msg
    ARGENT_CASE_N=$(( ARGENT_CASE_N + 1 ))
    out="${ARGENT_RESULT}.${case_name}.json"
    # A wrong tool result is not a slow response. Retry only HTTP 503, once.
    # Yugabyte tools/list on test_73_20261008_115801 was that code: QueryRef 18
    # exceeded the 20s auth budget, then the same lookup succeeded.
    http_st=$(mcp_http "POST" "${ARGENT_URL}" "${payload}" "${out}" \
        "${ARGENT_HDR}" "${ARGENT_JWT}" "${ARGENT_SESSION}" "" "${HTTP_TIMEOUT}")
    if [[ "${http_st}" == "503" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "INFO delay ${case_name}: HTTP 503, one retry"
        http_st=$(mcp_http "POST" "${ARGENT_URL}" "${payload}" "${out}" \
            "${ARGENT_HDR}" "${ARGENT_JWT}" "${ARGENT_SESSION}" "" "${HTTP_TIMEOUT}")
    fi
    if [[ "${http_st}" == "200" ]] && jq -e "${filter}" "${out}" >/dev/null 2>&1; then
        record_case "${ARGENT_RESULT}" "${case_name}" 1
        return 0
    fi
    record_case "${ARGENT_RESULT}" "${case_name}" 0
    code=$(jq -r '.result.structuredContent.code // empty' "${out}" 2>/dev/null || true)
    msg=$(jq -r '.result.structuredContent.message // empty' "${out}" 2>/dev/null \
        | tr '\n' ' ' | cut -c1-140 || true)
    if [[ -n "${code}" ]]; then
        echo "FAIL_${case_name}=${http_st}:${code}:${msg}" >> "${ARGENT_RESULT}"
    else
        echo "FAIL_${case_name}=${http_st}" >> "${ARGENT_RESULT}"
    fi
    return 1
}

argent_call() {
    local case_name="$1"
    local tool="$2"
    local args_json="$3"
    local filter="$4"
    local payload
    ARGENT_RPC_ID=$(( ARGENT_RPC_ID + 1 ))
    payload=$(jq -nc \
        --arg name "${tool}" \
        --argjson arguments "${args_json}" \
        --argjson id "${ARGENT_RPC_ID}" \
        '{jsonrpc:"2.0",id:$id,method:"tools/call",params:{name:$name,arguments:$arguments}}')
    argent_rpc "${case_name}" "${payload}" "${filter}"
}

argent_ok() {
    local case_name="$1"
    local tool="$2"
    local args_json="$3"
    local extra="$4"
    local v filter
    shift 4
    for v in "$@"; do
        if [[ ! "${v}" =~ ^[0-9]+$ ]]; then
            argent_miss "${case_name}"
            return 1
        fi
    done
    filter='.result.error == null and .result.structuredContent.ok == true'
    if [[ -n "${extra}" ]]; then
        filter="${filter} and (${extra})"
    fi
    argent_call "${case_name}" "${tool}" "${args_json}" "${filter}"
}

argent_fail() {
    local case_name="$1"
    local tool="$2"
    local args_json="$3"
    local code="$4"
    local v filter
    shift 4
    for v in "$@"; do
        if [[ ! "${v}" =~ ^[0-9]+$ ]]; then
            argent_miss "${case_name}"
            return 1
        fi
    done
    if [[ ! "${code}" =~ ^[a-z0-9_]+$ ]]; then
        argent_miss "${case_name}"
        return 1
    fi
    filter='.result.error == null and .result.structuredContent.ok == false and .result.structuredContent.code == "'"${code}"'"'
    argent_call "${case_name}" "${tool}" "${args_json}" "${filter}"
}

argent_confirm() {
    local case_name="$1"
    local tool="$2"
    local args_json="$3"
    local warning="$4"
    local v filter warn_re
    shift 4
    for v in "$@"; do
        if [[ ! "${v}" =~ ^[0-9]+$ ]]; then
            argent_miss "${case_name}"
            return 1
        fi
    done
    # A space inside an unquoted [[ =~ ]] class is split by the shell.
    warn_re='^[a-z ]+$'
    if [[ ! "${warning}" =~ ${warn_re} ]]; then
        argent_miss "${case_name}"
        return 1
    fi
    filter='.result.error == null and .result.structuredContent.ok == false'
    filter="${filter} and .result.structuredContent.code == \"needs_confirm\""
    filter="${filter} and .result.structuredContent.needs_confirm == true"
    filter="${filter} and .result.structuredContent.warning == \"${warning}\""
    filter="${filter} and (.result.structuredContent.confirm_token | type) == \"string\""
    filter="${filter} and (.result.structuredContent.confirm_token | length) > 0"
    argent_call "${case_name}" "${tool}" "${args_json}" "${filter}"
}

argent_grab() {
    local case_name="$1"
    local expr="$2"
    jq -er "${expr}" "${ARGENT_RESULT}.${case_name}.json" 2>/dev/null || true
}

argent_mcp_exercise() {
    local result_file="$1"
    local mcp_url="$2"
    local jwt="$3"
    local session="$4"
    local hdr="$5"
    local engine_key="$6"
    local suffix day token tool list_filter payload
    local -a tools
    local args org_id org2_id bank_id card_id exp_id tax_id usd_id parent_id child_id org2_ledger
    local term_id term2_id contact_id txn_id line_id tax_code_id tax_rate_id rate2_id
    local bare_code_id usd_code_id tag_id att_id long_key
    local recon_ledger buy_txn buy_line buy_exp_line future_txn future_line stmt_txn exp_stmt late_stmt
    local open_recon later_recon edit_token mismatch_token close_token rescind_token
    local close_txn mid_txn stmt_token buy_base mismatch_base stmt_late_id org2_stmt_id
    local sched_id rent_a rent_b rent_actual rent_down

    ARGENT_RESULT="${result_file}"
    ARGENT_URL="${mcp_url}"
    ARGENT_JWT="${jwt}"
    ARGENT_SESSION="${session}"
    ARGENT_HDR="${hdr}"
    ARGENT_CASE_N=0
    ARGENT_RPC_ID=100
    suffix="${engine_key}${BASHPID}"
    day="2026-10-07"
    token="t73${suffix}"
    long_key=$(printf '%81s' '' | tr ' ' 'k')

    argent_exercise_tools_list_orgs
    argent_exercise_ledgers
    argent_exercise_terms_contacts
    argent_exercise_txn
    argent_exercise_tags_att
    argent_exercise_balances
    argent_exercise_edit_rescind_recon
    argent_exercise_schedules
    argent_exercise_queries_rates
}
