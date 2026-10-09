#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, day, org_id, etc.) are set by argent_mcp_exercise
# shellcheck disable=SC2312 # diagnostic command substitutions intentionally swallow exit codes

# Argent balance query and ledger-idempotent-blocked exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_ok, argent_fail, argent_miss) and
# the exercise-local variables from argent_mcp_exercise:
# token, day, org_id, card_id, child_id, parent_id, txn_id

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_balances() {
    local args

    args=$(jq -n --arg day "${day}" '{as_of:$day}')
    argent_fail bal_org Argent.QueryBalances "${args}" organization_id_required || true
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org}')
    argent_fail bal_as_of Argent.QueryBalances "${args}" as_of "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,as_of:"bad"}')
    argent_fail bal_as_of_bad Argent.QueryBalances "${args}" as_of "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg day "${day}" \
        '{organization_id:$org,as_of:$day,status:[1,2,3,4,5,1]}')
    argent_fail bal_status_six Argent.QueryBalances "${args}" status "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg day "${day}" '{organization_id:$org,as_of:$day}')
    if [[ "${card_id}" =~ ^[0-9]+$ ]]; then
        argent_ok bal_card Argent.QueryBalances "${args}" \
            ".result.structuredContent.include_parents == false and .result.structuredContent.parents == null and ([.result.structuredContent.balances[] | select((.ledger_id|tonumber)==${card_id}) | .balance_cents | tonumber] | .[0]) == 500" \
            "${org_id}" || true
    else
        argent_miss bal_card
    fi
    args=$(jq -n --argjson org "${org_id:-0}" --arg day "${day}" \
        '{organization_id:$org,as_of:$day,include_parents:true,rate_source:9}')
    argent_fail bal_rate_bad Argent.QueryBalances "${args}" rate_source "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg day "${day}" \
        '{organization_id:$org,as_of:$day,include_parents:true,rate_source:"boc"}')
    argent_fail bal_rate_type Argent.QueryBalances "${args}" rate_source "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg day "${day}" \
        '{organization_id:$org,as_of:$day,include_parents:true,rate_source:1}')
    if [[ "${child_id}" =~ ^[0-9]+$ && "${parent_id}" =~ ^[0-9]+$ ]]; then
        argent_ok bal_parents Argent.QueryBalances "${args}" \
            ".result.structuredContent.include_parents == true and (([.result.structuredContent.parents[]? | select((.child_ledger_id|tonumber)==${child_id} and (.parent_ledger_id|tonumber)==${parent_id})] | length) >= 1)" \
            "${org_id}" || true
    else
        argent_miss bal_parents
    fi

    args=$(jq -n --argjson org "${org_id:-0}" --arg name "blocked${token}" --arg key "post${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0,idempotency_key:$key}')
    if [[ "${txn_id}" =~ ^[0-9]+$ ]]; then
        argent_ok ledger_idem_blocked Argent.UpsertLedger "${args}" \
            ".result.structuredContent.idempotent == true and .result.structuredContent.created == false and ((.result.structuredContent.txn_id|tonumber) == ${txn_id}) and .result.structuredContent.ledger_id == null" \
            "${org_id}" || true
    else
        argent_miss ledger_idem_blocked
    fi
}
