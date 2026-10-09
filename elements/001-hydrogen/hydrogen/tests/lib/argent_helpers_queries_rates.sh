#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, ARGENT_CASE_N, etc.) are set by argent_mcp_exercise
# shellcheck disable=SC2312 # diagnostic command substitutions intentionally swallow exit codes

# Argent query and rate exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_ok, argent_fail) and the
# exercise-local variables from argent_mcp_exercise:
# token, org_id, bank_id

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_queries_rates() {
    local args

    args=$(jq -n '{from:"2026-10-01",to:"nope"}')
    argent_fail due_date Argent.QueryDue "${args}" to || true
    args=$(jq -n '{from:"2026-10-01",to:"2026-10-31"}')
    argent_ok due_default Argent.QueryDue "${args}" \
        '(.result.structuredContent.schedules|type) == "array" and (.result.structuredContent.transactions|type) == "array"' || true
    args=$(jq -n '{from:"2026-10-01",to:"2026-10-31",status:[1]}')
    argent_ok due_status Argent.QueryDue "${args}" \
        '(.result.structuredContent.schedules|type) == "array" and (.result.structuredContent.transactions|type) == "array"' || true

    args=$(jq -n '{}')
    argent_ok recon_ok Argent.QueryReconciliationStatus "${args}" \
        '(.result.structuredContent.rows|type) == "array"' || true

    args=$(jq -n '{from:"2026-10-01",to:"2026-10-31"}')
    argent_fail history_ledger Argent.QueryLedgerHistory "${args}" ledger_id || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id,from:"2026-10-01",to:"2026-10-31"}')
    argent_ok history_ok Argent.QueryLedgerHistory "${args}" \
        '(.result.structuredContent.rows|type) == "array"' \
        "${bank_id}" || true

    args=$(jq -n '{from:"2026-10-01",to:"2026-10-31"}')
    argent_fail tax_org Argent.QueryTaxSummary "${args}" organization_id || true
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,from:"2026-10-01",to:"2026-10-31"}')
    argent_ok tax_ok Argent.QueryTaxSummary "${args}" \
        '(.result.structuredContent.rows|type) == "array"' \
        "${org_id}" || true

    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,from:"2026-10-01",to:"2026-10-31"}')
    argent_ok income_ok Argent.QueryIncomeExpense "${args}" \
        '(.result.structuredContent.rows|type) == "array" and ((.result.structuredContent.warnings|type) == "array" or .result.structuredContent.warnings == {}) and .result.structuredContent.currency == "cad" and (.result.structuredContent.rate_source|tonumber) == 1' \
        "${org_id}" || true

    args=$(jq -n '{from:"2026-10-01",to:"2026-10-31"}')
    argent_ok calendar_ok Argent.QueryCalendarView "${args}" \
        '(.result.structuredContent.rows|type) == "array"' || true

    args=$(jq -n '{from:"2026-10-01",to:"2026-10-31",base_currency:"usd",quote_currency:"cad",compare_source:1}')
    argent_fail fx_source Argent.QueryFxPremium "${args}" compare_source || true

    args=$(jq -n '{}')
    argent_ok sync_ok Argent.QuerySyncProblems "${args}" \
        '(.result.structuredContent.rows|type) == "array" and ([.result.structuredContent.rows[].calendar_state_a2011 | tonumber] | all(. == 2 or . == 4))' || true

    args=$(jq -n '{}')
    argent_fail search_q Argent.Search "${args}" q || true
    args=$(jq -n --arg q "${token}" '{q:$q}')
    argent_ok search_ok Argent.Search "${args}" \
        '(.result.structuredContent.rows|type) == "array" and (.result.structuredContent.truncated|type) == "boolean"' || true

    args=$(jq -n '{base_currency:"usd",quote_currency:"cad",as_of:"2026-10-08",rate_n:1324434,rate_d:1000000,source:1}')
    argent_fail rate_source Argent.UpsertRate "${args}" source || true
    args=$(jq -n '{base_currency:"usd",quote_currency:"cad",as_of:"2026-10-08",rate_n:1324434,rate_d:1000000}')
    argent_ok rate_manual Argent.UpsertRate "${args}" \
        '(.result.structuredContent.created == true or .result.structuredContent.updated == true) and (.result.structuredContent.source_a2012|tonumber) == 5 and (.result.structuredContent.rate_n|tonumber) == 1324434 and (.result.structuredContent.rate_d|tonumber) == 1000000' || true
    argent_ok rate_again Argent.UpsertRate "${args}" \
        '.result.structuredContent.updated == true and (.result.structuredContent.source_a2012|tonumber) == 5 and (.result.structuredContent.rate_n|tonumber) == 1324434' || true

    args=$(jq -n '{base_currency:"usd",quote_currency:"cad",source:5,from:"2026-10-01",to:"2026-10-31"}')
    argent_ok rate_list Argent.ListRates "${args}" \
        '(.result.structuredContent.rows|type) == "array" and (.result.structuredContent.rows|length) >= 1' || true

    args=$(jq -n '{from:"2026-10-08",to:"2026-10-08",base_currency:"usd",quote_currency:"cad",compare_source:5}')
    argent_ok fx_manual Argent.QueryFxPremium "${args}" \
        '([.result.structuredContent.rows[].compare_rate_n | tonumber] | index(1324434)) != null' || true

    args=$(jq -n '{base_currency:"xxx",quote_currency:"cad",as_of:"2026-10-08"}')
    argent_fail boc_pair Argent.GetBocRate "${args}" pair || true
    args=$(jq -n '{base_currency:"usd",quote_currency:"cad",as_of:"not-a-date"}')
    argent_fail boc_date Argent.GetBocRate "${args}" as_of || true

    echo "EXPECTED_TOOL_CASES=${ARGENT_CASE_N}" >> "${result_file}"
}
