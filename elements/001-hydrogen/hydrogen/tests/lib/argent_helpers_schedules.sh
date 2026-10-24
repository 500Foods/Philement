#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, suffix, etc.) are set by argent_mcp_exercise

# Argent schedule and match-reserved exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_ok, argent_fail, argent_miss,
# argent_grab) and the exercise-local variables from argent_mcp_exercise:
# token, org_id, bank_id, exp_id, usd_id, recon_ledger, buy_txn, buy_line,
# buy_exp_line, future_txn, future_line, stmt_txn, exp_stmt, late_stmt,
# open_recon, later_recon, sched_id, rent_a, rent_b, rent_actual, rent_down

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_schedules() {
    local args

    # Phase 14. Generate runs before calendar_url is set, so those rows stay not_set.
    # The idempotent upsert sends a dead URL and must not store it.
    # The later match uses http://127.0.0.1:9/cal/ and must still save.
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org}')
    argent_fail sched_name_required Argent.UpsertSchedule "${args}" name_required "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "rent${token}" \
        '{organization_id:$org,name:$name,rrule:"FREQ=HOURLY"}')
    argent_fail sched_rrule Argent.UpsertSchedule "${args}" rrule "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --argjson from "${bank_id:-0}" --argjson to "${usd_id:-0}" \
        --arg name "mix${token}" \
        '{organization_id:$org,name:$name,from_ledger_id:$from,to_ledger_id:$to,amount_cents:100,currency:"cad",rrule:"FREQ=WEEKLY",anchor_on:"2026-10-01"}')
    argent_fail sched_currency Argent.UpsertSchedule "${args}" currency "${org_id}" "${bank_id}" "${usd_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "zero${token}" \
        '{organization_id:$org,name:$name,rrule:"FREQ=WEEKLY",amount_cents:0}')
    argent_fail sched_amount Argent.UpsertSchedule "${args}" amount "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --argjson from "${bank_id:-0}" --arg name "same${token}" \
        '{organization_id:$org,name:$name,from_ledger_id:$from,to_ledger_id:$from,amount_cents:100,currency:"cad",rrule:"FREQ=WEEKLY",anchor_on:"2026-10-01"}')
    argent_fail sched_same Argent.UpsertSchedule "${args}" ledger "${org_id}" "${bank_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "days${token}" \
        '{organization_id:$org,name:$name,from_ledger_id:1,to_ledger_id:2,amount_cents:100,currency:"cad",rrule:"FREQ=WEEKLY",anchor_on:"2026-10-01",horizon_mode_a2008:2}')
    argent_fail sched_horizon Argent.UpsertSchedule "${args}" horizon_days "${org_id}" || true

    args=$(jq -n --argjson org "${org_id:-0}" --argjson from "${bank_id:-0}" --argjson to "${exp_id:-0}" \
        --arg name "rent${token}" --arg key "rent${token}" \
        '{organization_id:$org,name:$name,from_ledger_id:$from,to_ledger_id:$to,amount_cents:150000,currency:"cad",rrule:"FREQ=WEEKLY",anchor_on:"2026-10-01",idempotency_key:$key}')
    argent_ok sched_create Argent.UpsertSchedule "${args}" \
        '.result.structuredContent.created == true and .result.structuredContent.idempotent == false and ((.result.structuredContent.schedule_id|tonumber) > 0)' \
        "${org_id}" "${bank_id}" "${exp_id}" || true
    sched_id=$(argent_grab sched_create '.result.structuredContent.schedule_id | tonumber')
    args=$(jq -n --argjson org "${org_id:-0}" --arg key "rent${token}" \
        '{organization_id:$org,idempotency_key:$key,calendar_url:"http://127.0.0.1:9/cal/"}')
    if [[ "${sched_id}" =~ ^[0-9]+$ ]]; then
        argent_ok sched_idem Argent.UpsertSchedule "${args}" \
            ".result.structuredContent.created == false and .result.structuredContent.idempotent == true and ((.result.structuredContent.schedule_id|tonumber) == ${sched_id})" \
            "${org_id}" || true
    else
        argent_miss sched_idem || true
    fi

    argent_fail gen_missing Argent.GenerateSchedule '{"schedule_id":999999999}' not_found || true
    args=$(jq -n --argjson id "${sched_id:-0}" '{schedule_id:$id,through:"2026-10-22"}')
    argent_ok gen_month Argent.GenerateSchedule "${args}" \
        '(.result.structuredContent.created|length) == 4 and (.result.structuredContent.skipped|tonumber) == 0 and ([.result.structuredContent.created[].status_a2003] | all(. == 1)) and ([.result.structuredContent.created[].calendar_state_a2011] | all(. == 1)) and ([.result.structuredContent.created[].calendar_attempts] | all(. == 0)) and (([.result.structuredContent.created[].txn_on] | index("2026-10-01")) != null) and (([.result.structuredContent.created[].txn_on] | index("2026-10-22")) != null)' \
        "${sched_id}" || true
    rent_a=$(argent_grab gen_month '.result.structuredContent.txn_ids[0] | tonumber')
    rent_b=$(argent_grab gen_month '.result.structuredContent.txn_ids[1] | tonumber')
    if [[ "${rent_a}" =~ ^[0-9]+$ ]]; then
        argent_ok gen_get Argent.GetTransaction "{\"txn_id\":${rent_a}}" \
            '(.result.structuredContent.transaction.status_a2003|tonumber) == 1 and (.result.structuredContent.transaction.calendar_state_a2011|tonumber) == 1' \
            "${rent_a}" || true
    else
        argent_miss gen_get || true
    fi
    args=$(jq -n --argjson id "${sched_id:-0}" '{schedule_id:$id,through:"2026-10-22"}')
    argent_ok gen_skip Argent.GenerateSchedule "${args}" \
        '(.result.structuredContent.created|length) == 0 and (.result.structuredContent.skipped|tonumber) == 4' \
        "${sched_id}" || true

    args=$(jq -n --argjson id "${rent_a:-0}" '{reserved_txn_id:$id,amount_window:0,amount_cents:1}')
    argent_fail match_amount Argent.MatchReserved "${args}" match_amount "${rent_a}" || true
    if [[ "${rent_a}" =~ ^[0-9]+$ ]]; then
        argent_ok match_still Argent.GetTransaction "{\"txn_id\":${rent_a}}" \
            '(.result.structuredContent.transaction.status_a2003|tonumber) == 1' \
            "${rent_a}" || true
    else
        argent_miss match_still || true
    fi
    args=$(jq -n --argjson id "${rent_a:-0}" '{reserved_txn_id:$id}')
    argent_ok match_one Argent.MatchReserved "${args}" \
        '(.result.structuredContent.status_a2003|tonumber) == 3 and (.result.structuredContent.reserved_status_a2003|tonumber) == 5 and (.result.structuredContent.calendar_state_a2011|tonumber) == 1 and (.result.structuredContent.calendar_attempts|tonumber) == 0 and ((.result.structuredContent.replaces_txn_id|tonumber) == (.result.structuredContent.reserved_txn_id|tonumber))' \
        "${rent_a}" || true
    rent_actual=$(argent_grab match_one '.result.structuredContent.txn_id | tonumber')
    if [[ "${rent_a}" =~ ^[0-9]+$ ]]; then
        argent_ok match_reserved_get Argent.GetTransaction "{\"txn_id\":${rent_a}}" \
            '(.result.structuredContent.transaction.status_a2003|tonumber) == 5' \
            "${rent_a}" || true
    else
        argent_miss match_reserved_get || true
    fi
    if [[ "${rent_actual}" =~ ^[0-9]+$ && "${rent_a}" =~ ^[0-9]+$ ]]; then
        argent_ok match_actual_get Argent.GetTransaction "{\"txn_id\":${rent_actual}}" \
            "(.result.structuredContent.transaction.status_a2003|tonumber) == 3 and (.result.structuredContent.transaction.calendar_state_a2011|tonumber) == 1 and ((.result.structuredContent.transaction.replaces_txn_id|tonumber) == ${rent_a})" \
            "${rent_actual}" || true
    else
        argent_miss match_actual_get || true
    fi

    argent_fail retry_missing Argent.RetryCalendar '{"txn_id":999999999}' not_found || true
    args=$(jq -n --argjson id "${rent_a:-0}" '{txn_id:$id}')
    argent_fail retry_not_pending Argent.RetryCalendar "${args}" not_pending "${rent_a}" || true

    args=$(jq -n --argjson id "${sched_id:-0}" '{schedule_id:$id,calendar_url:"http://127.0.0.1:9/cal/"}')
    if [[ "${sched_id}" =~ ^[0-9]+$ ]]; then
        argent_ok sched_url Argent.UpsertSchedule "${args}" \
            ".result.structuredContent.created == false and .result.structuredContent.idempotent == false and ((.result.structuredContent.schedule_id|tonumber) == ${sched_id})" \
            "${sched_id}" || true
    else
        argent_miss sched_url || true
    fi
    args=$(jq -n --argjson id "${rent_b:-0}" '{reserved_txn_id:$id}')
    argent_ok match_down Argent.MatchReserved "${args}" \
        '(.result.structuredContent.status_a2003|tonumber) == 3 and (.result.structuredContent.calendar_state_a2011|tonumber) == 4 and (.result.structuredContent.calendar_attempts|tonumber) >= 1 and (.result.structuredContent.calendar_error|type) == "string" and (.result.structuredContent.calendar_error|length) > 0' \
        "${rent_b}" || true
    rent_down=$(argent_grab match_down '.result.structuredContent.txn_id | tonumber')
    if [[ "${rent_down}" =~ ^[0-9]+$ ]]; then
        argent_ok match_down_get Argent.GetTransaction "{\"txn_id\":${rent_down}}" \
            '(.result.structuredContent.transaction.status_a2003|tonumber) == 3' \
            "${rent_down}" || true
    else
        argent_miss match_down_get || true
    fi
    args=$(jq -n --argjson id "${rent_down:-0}" '{txn_id:$id}')
    argent_ok retry_down Argent.RetryCalendar "${args}" \
        '(.result.structuredContent.tried|tonumber) == 1 and (.result.structuredContent.failed|tonumber) == 1 and (.result.structuredContent.set|tonumber) == 0 and (.result.structuredContent.calendar_state_a2011|tonumber) == 4 and (.result.structuredContent.calendar_attempts|tonumber) >= 2 and (.result.structuredContent.status_a2003|tonumber) == 3' \
        "${rent_down}" || true
    if [[ "${rent_down}" =~ ^[0-9]+$ ]]; then
        argent_ok retry_down_get Argent.GetTransaction "{\"txn_id\":${rent_down}}" \
            '(.result.structuredContent.transaction.status_a2003|tonumber) == 3' \
            "${rent_down}" || true
    else
        argent_miss retry_down_get || true
    fi
}
