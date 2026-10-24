#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, day, etc.) are set by argent_mcp_exercise
# shellcheck disable=SC2312 # diagnostic command substitutions intentionally swallow exit codes

# Argent edit, rescind, statement, and reconciliation exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_ok, argent_fail, argent_miss,
# argent_confirm, argent_call, argent_grab) and the exercise-local variables
# from argent_mcp_exercise:
# token, day, org_id, org2_id, bank_id, bank_id, card_id, exp_id, tax_id,
# usd_id, parent_id, child_id, org2_ledger, txn_id, line_id, tax_code_id,
# tax_rate_id, bare_code_id, usd_code_id, rate2_id, att_id, recon_ledger,
# buy_txn, buy_line, buy_exp_line, future_txn, future_line, stmt_txn,
# exp_stmt, late_stmt, open_recon, later_recon, edit_token, mismatch_token,
# stmt_token, close_txn, mid_txn, close_token, rescind_token, org2_stmt_id

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_edit_rescind_recon() {
    local args buy_base mismatch_base

    # Phase 12. A fresh posting ledger keeps the earlier balance cases stable.
    # Book balance through 2026-10-07 is the purchase of -250. The statement
    # line is amount 0. Complete is asked to match 0, so a bare override fails.
    argent_fail edit_txn_required Argent.EditTransaction '{}' txn_id_required || true
    argent_fail edit_unknown Argent.EditTransaction '{"txn_id":999999999}' not_found || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id,txn_on:"bad"}')
    argent_fail edit_date Argent.EditTransaction "${args}" txn_on "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id,description:""}')
    argent_fail edit_description Argent.EditTransaction "${args}" description_required "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id,memo:1}')
    argent_fail edit_memo Argent.EditTransaction "${args}" memo "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id,lines:1}')
    argent_fail edit_lines Argent.EditTransaction "${args}" lines "${txn_id}" || true
    argent_fail rescind_required Argent.RescindTransaction '{}' txn_id_required || true
    argent_fail rescind_unknown Argent.RescindTransaction '{"txn_id":999999999}' not_found || true
    argent_fail stmt_ledger_required Argent.PostStatement '{"txn_on":"2026-10-07","description":"x"}' ledger_id_required || true
    argent_fail stmt_date Argent.PostStatement '{"ledger_id":1,"txn_on":"bad","description":"x"}' txn_on || true
    argent_fail stmt_description Argent.PostStatement '{"ledger_id":1,"txn_on":"2026-10-07"}' description_required || true
    args=$(jq -n '{ledger_id:1,txn_on:"2026-10-07",description:"x",statement_balance_cents:1.5}')
    argent_fail stmt_balance_type Argent.PostStatement "${args}" statement_balance_cents || true
    args=$(jq -n --argjson id "${parent_id:-0}" '{ledger_id:$id,txn_on:"2026-10-07",description:"x"}')
    argent_fail stmt_nonposting Argent.PostStatement "${args}" ledger "${parent_id}" || true
    argent_fail close_ledger_required Argent.PostPeriodClose '{"txn_on":"2026-10-20","description":"x"}' ledger_id_required || true
    args=$(jq -n --argjson id "${parent_id:-0}" '{ledger_id:$id,txn_on:"2026-10-20",description:"x"}')
    argent_fail close_nonposting Argent.PostPeriodClose "${args}" ledger "${parent_id}" || true
    argent_fail start_ledger_required Argent.StartReconciliation '{"statement_txn_id":1,"reconciled_on":"2026-10-07","statement_balance_cents":0}' ledger_id_required || true
    argent_fail start_balance_required Argent.StartReconciliation '{"ledger_id":1,"statement_txn_id":1,"reconciled_on":"2026-10-07"}' statement_balance_cents || true
    argent_fail start_date Argent.StartReconciliation '{"ledger_id":1,"statement_txn_id":1,"reconciled_on":"bad","statement_balance_cents":0}' reconciled_on || true
    argent_fail done_required Argent.CompleteReconciliation '{}' reconciliation_id_required || true
    argent_fail done_reason_type Argent.CompleteReconciliation '{"reconciliation_id":1,"override_reason":1}' override_reason || true
    argent_fail clear_required Argent.ClearLines '{}' reconciliation_id_required || true
    argent_fail clear_ids Argent.ClearLines '{"reconciliation_id":1}' line_ids || true

    args=$(jq -n --argjson org "${org_id:-0}" --arg name "recon${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,is_posting:true,currency:"cad",opening_on:"2026-10-01",opening_balance_cents:0}')
    argent_ok recon_ledger_ok Argent.UpsertLedger "${args}" '.result.structuredContent.created == true' \
        "${org_id}" || true
    recon_ledger=$(argent_grab recon_ledger_ok '.result.structuredContent.ledger_id | tonumber')

    args=$(jq -n --arg desc "buy${token}" --argjson ledger "${recon_ledger:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-02",description:$desc,lines:[{ledger_id:$ledger,amount_cents:-250},{ledger_id:$exp,amount_cents:250}]}')
    argent_ok recon_buy Argent.PostTransaction "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.lines|length) == 2' \
        "${recon_ledger}" "${exp_id}" || true
    buy_txn=$(argent_grab recon_buy '.result.structuredContent.txn_id | tonumber')
    if [[ "${recon_ledger}" =~ ^[0-9]+$ ]]; then
        buy_line=$(argent_grab recon_buy "[.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${recon_ledger}) | .line_id | tonumber] | .[0]")
    fi
    if [[ "${exp_id}" =~ ^[0-9]+$ ]]; then
        buy_exp_line=$(argent_grab recon_buy "[.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${exp_id}) | .line_id | tonumber] | .[0]")
    fi

    args=$(jq -n --arg desc "future${token}" --argjson ledger "${recon_ledger:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-08",description:$desc,lines:[{ledger_id:$ledger,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_ok recon_future Argent.PostTransaction "${args}" '.result.structuredContent.created == true' \
        "${recon_ledger}" "${exp_id}" || true
    future_txn=$(argent_grab recon_future '.result.structuredContent.txn_id | tonumber')
    if [[ "${recon_ledger}" =~ ^[0-9]+$ ]]; then
        future_line=$(argent_grab recon_future "[.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${recon_ledger}) | .line_id | tonumber] | .[0]")
    fi
    args=$(jq -n --argjson id "${future_txn:-0}" --arg desc "nope${token}" \
        '{txn_id:$id,description:$desc,confirm_token:"not-a-token"}')
    argent_fail future_bogus Argent.EditTransaction "${args}" confirm_not_found "${future_txn}" || true
    args=$(jq -n --argjson id "${future_txn:-0}" '{txn_id:$id}')
    if [[ "${future_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok future_bogus_seen Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"future${token}\"" \
            "${future_txn}" || true
    else
        argent_miss future_bogus_seen || true
    fi

    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --arg desc "stmt${token}" --arg key "stmt${token}" \
        '{ledger_id:$ledger,txn_on:"2026-10-07",description:$desc,idempotency_key:$key}')
    argent_ok recon_stmt Argent.PostStatement "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.kind_a2004|tonumber) == 7' \
        "${recon_ledger}" || true
    stmt_txn=$(argent_grab recon_stmt '.result.structuredContent.txn_id | tonumber')
    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --arg desc "stmt-again${token}" --arg key "stmt${token}" \
        '{ledger_id:$ledger,txn_on:"2026-10-07",description:$desc,idempotency_key:$key}')
    if [[ "${stmt_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok recon_stmt_idem Argent.PostStatement "${args}" \
            ".result.structuredContent.idempotent == true and .result.structuredContent.created == false and ((.result.structuredContent.txn_id|tonumber) == ${stmt_txn})" \
            "${recon_ledger}" || true
        argent_ok recon_stmt_get Argent.GetTransaction "{\"txn_id\":${stmt_txn}}" \
            '(.result.structuredContent.transaction.kind_a2004|tonumber) == 7 and (.result.structuredContent.transaction.status_a2003|tonumber) == 3' \
            "${stmt_txn}" || true
    else
        argent_miss recon_stmt_idem || true
        argent_miss recon_stmt_get || true
    fi

    args=$(jq -n --argjson ledger "${exp_id:-0}" --arg desc "stmtexp${token}" \
        '{ledger_id:$ledger,txn_on:"2026-10-07",description:$desc}')
    argent_ok recon_stmt_exp Argent.PostStatement "${args}" '.result.structuredContent.created == true' \
        "${exp_id}" || true
    exp_stmt=$(argent_grab recon_stmt_exp '.result.structuredContent.txn_id | tonumber')

    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --arg desc "late${token}" \
        '{ledger_id:$ledger,txn_on:"2026-10-31",description:$desc}')
    argent_ok recon_stmt_late Argent.PostStatement "${args}" '.result.structuredContent.created == true' \
        "${recon_ledger}" || true
    late_stmt=$(argent_grab recon_stmt_late '.result.structuredContent.txn_id | tonumber')

    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    argent_fail edit_empty Argent.EditTransaction "${args}" edit_empty "${buy_txn}" || true
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id,lines:[{line_id:999999999,amount_cents:1}]}')
    argent_fail edit_line_missing Argent.EditTransaction "${args}" line_not_found "${buy_txn}" || true
    args=$(jq -n --argjson id "${buy_txn:-0}" --argjson line "${buy_line:-0}" \
        '{txn_id:$id,lines:[{line_id:$line,amount_cents:5}]}')
    argent_fail edit_unbalanced Argent.EditTransaction "${args}" unbalanced "${buy_txn}" "${buy_line}" || true
    args=$(jq -n --argjson id "${buy_txn:-0}" --arg desc "bought${token}" '{txn_id:$id,description:$desc}')
    argent_ok edit_safe Argent.EditTransaction "${args}" \
        '(.result.structuredContent.status_a2003|tonumber) == 3 and .result.structuredContent.knocked == false' \
        "${buy_txn}" || true
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok edit_safe_seen Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"bought${token}\" and (.result.structuredContent.transaction.status_a2003|tonumber) == 3" \
            "${buy_txn}" || true
    else
        argent_miss edit_safe_seen || true
    fi

    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --argjson stmt "${buy_txn:-0}" \
        '{ledger_id:$ledger,statement_txn_id:$stmt,reconciled_on:"2026-10-07",statement_balance_cents:0}')
    argent_fail start_kind Argent.StartReconciliation "${args}" statement_kind "${recon_ledger}" "${buy_txn}" || true
    args=$(jq -n --argjson ledger "${recon_ledger:-0}" \
        '{ledger_id:$ledger,statement_txn_id:999999999,reconciled_on:"2026-10-07",statement_balance_cents:0}')
    argent_fail start_missing Argent.StartReconciliation "${args}" statement_not_found "${recon_ledger}" || true
    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --argjson stmt "${exp_stmt:-0}" \
        '{ledger_id:$ledger,statement_txn_id:$stmt,reconciled_on:"2026-10-07",statement_balance_cents:0}')
    argent_fail start_other_ledger Argent.StartReconciliation "${args}" statement_ledger "${recon_ledger}" "${exp_stmt}" || true
    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --argjson stmt "${stmt_txn:-0}" --arg summary "$(printf '%201s' '' | tr ' ' 's')" \
        '{ledger_id:$ledger,statement_txn_id:$stmt,reconciled_on:"2026-10-07",statement_balance_cents:0,summary:$summary}')
    argent_fail start_summary Argent.StartReconciliation "${args}" summary "${recon_ledger}" "${stmt_txn}" || true
    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --argjson stmt "${stmt_txn:-0}" \
        '{ledger_id:$ledger,statement_txn_id:$stmt,reconciled_on:"2026-10-07",statement_balance_cents:0}')
    argent_ok start_open Argent.StartReconciliation "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.status_a2006|tonumber) == 1 and (.result.structuredContent.book_balance_cents|tonumber) == -250 and (.result.structuredContent.statement_balance_cents|tonumber) == 0' \
        "${recon_ledger}" "${stmt_txn}" || true
    open_recon=$(argent_grab start_open '.result.structuredContent.reconciliation_id | tonumber')
    argent_fail start_again Argent.StartReconciliation "${args}" reconciliation_open "${recon_ledger}" "${stmt_txn}" || true

    args=$(jq -n --argjson id "${open_recon:-0}" --argjson line "${buy_exp_line:-0}" '{reconciliation_id:$id,line_ids:[$line]}')
    argent_fail clear_mismatch Argent.ClearLines "${args}" ledger_mismatch "${open_recon}" "${buy_exp_line}" || true
    args=$(jq -n --argjson id "${open_recon:-0}" --argjson line "${future_line:-0}" '{reconciliation_id:$id,line_ids:[$line]}')
    argent_fail clear_future Argent.ClearLines "${args}" line_after_statement "${open_recon}" "${future_line}" || true
    args=$(jq -n --argjson id "${open_recon:-0}" '{reconciliation_id:$id,line_ids:[999999999]}')
    argent_fail clear_missing Argent.ClearLines "${args}" line_not_found "${open_recon}" || true
    args=$(jq -n --argjson id "${open_recon:-0}" '{reconciliation_id:$id,line_ids:[]}')
    argent_ok clear_empty Argent.ClearLines "${args}" '(.result.structuredContent.line_ids|length) == 0' \
        "${open_recon}" || true
    args=$(jq -n --argjson id "${open_recon:-0}" --argjson line "${buy_line:-0}" '{reconciliation_id:$id,line_ids:[$line]}')
    if [[ "${buy_line}" =~ ^[0-9]+$ ]]; then
        argent_ok clear_buy Argent.ClearLines "${args}" \
            "([.result.structuredContent.line_ids[]? | tonumber] | index(${buy_line})) != null" \
            "${open_recon}" "${buy_line}" || true
    else
        argent_miss clear_buy || true
    fi

    argent_fail done_unknown Argent.CompleteReconciliation '{"reconciliation_id":999999999}' not_found || true
    if [[ "${open_recon}" =~ ^[0-9]+$ ]]; then
        args=$(jq -n --argjson id "${open_recon}" '{reconciliation_id:$id}')
        argent_call done_override Argent.CompleteReconciliation "${args}" \
            '.result.error == null and .result.structuredContent.ok == false and .result.structuredContent.code == "override_reason_required" and (.result.structuredContent.statement_balance_cents|tonumber) == 0 and (.result.structuredContent.book_balance_cents|tonumber) == -250' \
            || true
    else
        argent_miss done_override || true
    fi
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok done_still_recorded Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"bought${token}\" and (.result.structuredContent.transaction.status_a2003|tonumber) == 3" \
            "${buy_txn}" || true
    else
        argent_miss done_still_recorded || true
    fi
    args=$(jq -n --argjson id "${open_recon:-0}" --arg reason "test73" '{reconciliation_id:$id,override_reason:$reason}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok done_reason Argent.CompleteReconciliation "${args}" \
            "(.result.structuredContent.status_a2006|tonumber) == 2 and .result.structuredContent.override == true and ((.result.structuredContent.book_balance_cents|tonumber) == -250) and (([.result.structuredContent.txn_ids[]? | tonumber] | index(${buy_txn})) != null)" \
            "${open_recon}" || true
    else
        argent_miss done_reason || true
    fi
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok done_reconciled Argent.GetTransaction "${args}" \
            '(.result.structuredContent.transaction.status_a2003|tonumber) == 4' \
            "${buy_txn}" || true
    else
        argent_miss done_reconciled || true
    fi
    args=$(jq -n --argjson id "${open_recon:-0}" '{reconciliation_id:$id}')
    argent_ok done_again Argent.CompleteReconciliation "${args}" \
        '.result.structuredContent.already == true and (.result.structuredContent.status_a2006|tonumber) == 2' \
        "${open_recon}" || true
    args=$(jq -n --argjson id "${open_recon:-0}" '{reconciliation_id:$id,line_ids:[]}')
    argent_fail clear_closed Argent.ClearLines "${args}" reconciliation_closed "${open_recon}" || true

    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --argjson stmt "${stmt_txn:-0}" \
        '{ledger_id:$ledger,statement_txn_id:$stmt,reconciled_on:"2026-10-07",statement_balance_cents:0}')
    argent_confirm start_reconciled_date Argent.StartReconciliation "${args}" "reconciled date" \
        "${recon_ledger}" "${stmt_txn}" || true
    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --argjson stmt "${late_stmt:-0}" \
        '{ledger_id:$ledger,statement_txn_id:$stmt,reconciled_on:"2026-10-31",statement_balance_cents:0}')
    argent_ok start_later Argent.StartReconciliation "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.status_a2006|tonumber) == 1' \
        "${recon_ledger}" "${late_stmt}" || true
    later_recon=$(argent_grab start_later '.result.structuredContent.reconciliation_id | tonumber')
    args=$(jq -n --argjson id "${later_recon:-0}" --argjson line "${buy_line:-0}" '{reconciliation_id:$id,line_ids:[$line]}')
    argent_fail clear_reconciled_line Argent.ClearLines "${args}" reconciled_line "${later_recon}" "${buy_line}" || true

    buy_base=$(jq -nc --argjson id "${buy_txn:-0}" --arg desc "knock${token}" '{txn_id:$id,description:$desc}')
    argent_confirm edit_warn Argent.EditTransaction "${buy_base}" "reconciled transaction" "${buy_txn}" || true
    edit_token=$(argent_grab edit_warn '.result.structuredContent.confirm_token')
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok edit_unchanged Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"bought${token}\" and (.result.structuredContent.transaction.status_a2003|tonumber) == 4" \
            "${buy_txn}" || true
    else
        argent_miss edit_unchanged || true
    fi
    args=$(jq -nc --argjson id "${buy_txn:-0}" --arg desc "$(printf '%4001s' '' | tr ' ' 'd')" \
        '{txn_id:$id,description:$desc}')
    argent_fail edit_body_long Argent.EditTransaction "${args}" body_too_long "${buy_txn}" || true
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok edit_body_unchanged Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"bought${token}\" and (.result.structuredContent.transaction.status_a2003|tonumber) == 4" \
            "${buy_txn}" || true
    else
        argent_miss edit_body_unchanged || true
    fi
    if [[ -n "${edit_token}" && "${buy_txn}" =~ ^[0-9]+$ ]]; then
        args=$(jq -nc --argjson id "${buy_txn}" --arg token "${edit_token}" '{txn_id:$id,confirm_token:$token}')
        argent_fail edit_token_tool Argent.RescindTransaction "${args}" confirm_tool "${buy_txn}" || true
    else
        argent_miss edit_token_tool || true
    fi
    if [[ -n "${edit_token}" && "${buy_txn}" =~ ^[0-9]+$ ]]; then
        args=$(jq -nc --argjson base "${buy_base}" --arg token "${edit_token}" '$base + {confirm_token:$token}')
        argent_ok edit_apply Argent.EditTransaction "${args}" \
            '(.result.structuredContent.status_a2003|tonumber) == 3 and .result.structuredContent.knocked == true' \
            "${buy_txn}" || true
    else
        argent_miss edit_apply || true
    fi
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok edit_knocked Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"knock${token}\" and (.result.structuredContent.transaction.status_a2003|tonumber) == 3" \
            "${buy_txn}" || true
    else
        argent_miss edit_knocked || true
    fi
    if [[ -n "${edit_token}" && "${buy_txn}" =~ ^[0-9]+$ ]]; then
        args=$(jq -nc --argjson id "${buy_txn}" --arg desc "again${token}" --arg token "${edit_token}" \
            '{txn_id:$id,description:$desc,confirm_token:$token}')
        argent_fail edit_token_used Argent.EditTransaction "${args}" confirm_used "${buy_txn}" || true
    else
        argent_miss edit_token_used || true
    fi
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok edit_still_knock Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"knock${token}\"" \
            "${buy_txn}" || true
    else
        argent_miss edit_still_knock || true
    fi
    mismatch_base=$(jq -nc --argjson id "${buy_txn:-0}" --arg desc "mismatch${token}" '{txn_id:$id,description:$desc}')
    argent_confirm edit_mismatch_warn Argent.EditTransaction "${mismatch_base}" "reconciled date" "${buy_txn}" || true
    mismatch_token=$(argent_grab edit_mismatch_warn '.result.structuredContent.confirm_token')
    if [[ -n "${mismatch_token}" && "${buy_txn}" =~ ^[0-9]+$ ]]; then
        args=$(jq -nc --argjson base "${mismatch_base}" --arg desc "other${token}" --arg token "${mismatch_token}" \
            '$base + {description:$desc,confirm_token:$token}')
        argent_fail edit_mismatch Argent.EditTransaction "${args}" confirm_mismatch "${buy_txn}" || true
    else
        argent_miss edit_mismatch || true
    fi
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok edit_still_after_mismatch Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"knock${token}\"" \
            "${buy_txn}" || true
    else
        argent_miss edit_still_after_mismatch || true
    fi
    args=$(jq -n --argjson id "${buy_txn:-0}" --arg desc "bogus${token}" \
        '{txn_id:$id,description:$desc,confirm_token:"not-a-token"}')
    argent_fail edit_bogus Argent.EditTransaction "${args}" confirm_not_found "${buy_txn}" || true
    args=$(jq -n --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    if [[ "${buy_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok edit_still_after_bogus Argent.GetTransaction "${args}" \
            ".result.structuredContent.transaction.description == \"knock${token}\"" \
            "${buy_txn}" || true
    else
        argent_miss edit_still_after_bogus || true
    fi

    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --arg desc "stmtr${token}" \
        '{ledger_id:$ledger,txn_on:"2026-10-07",description:$desc}')
    argent_confirm stmt_reconciled_warn Argent.PostStatement "${args}" "reconciled date" "${recon_ledger}" || true
    stmt_token=$(argent_grab stmt_reconciled_warn '.result.structuredContent.confirm_token')
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok stmt_reconciled_absent Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"stmtr${token}\") == null" \
            "${org_id}" || true
    else
        argent_miss stmt_reconciled_absent || true
    fi
    if [[ -n "${stmt_token}" && "${recon_ledger}" =~ ^[0-9]+$ ]]; then
        args=$(jq -nc --argjson ledger "${recon_ledger}" --arg desc "stmtr${token}" --arg token "${stmt_token}" \
            '{ledger_id:$ledger,txn_on:"2026-10-07",description:$desc,confirm_token:$token}')
        argent_ok stmt_reconciled_apply Argent.PostStatement "${args}" \
            '.result.structuredContent.created == true and (.result.structuredContent.kind_a2004|tonumber) == 7' \
            "${recon_ledger}" || true
        stmt_late_id=$(argent_grab stmt_reconciled_apply '.result.structuredContent.txn_id | tonumber')
        if [[ "${stmt_late_id}" =~ ^[0-9]+$ ]]; then
            argent_ok stmt_reconciled_get Argent.GetTransaction "{\"txn_id\":${stmt_late_id}}" \
                '(.result.structuredContent.transaction.kind_a2004|tonumber) == 7 and (.result.structuredContent.transaction.status_a2003|tonumber) == 3' \
                "${stmt_late_id}" || true
        else
            argent_miss stmt_reconciled_get || true
        fi
    else
        argent_miss stmt_reconciled_apply || true
        argent_miss stmt_reconciled_get || true
    fi

    args=$(jq -n --arg desc "mid${token}" --argjson ledger "${recon_ledger:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-10",description:$desc,lines:[{ledger_id:$ledger,amount_cents:-3},{ledger_id:$exp,amount_cents:3}]}')
    argent_ok mid_post Argent.PostTransaction "${args}" '.result.structuredContent.created == true' \
        "${recon_ledger}" "${exp_id}" || true
    mid_txn=$(argent_grab mid_post '.result.structuredContent.txn_id | tonumber')

    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --arg desc "close${token}" --arg key "close${token}" \
        '{ledger_id:$ledger,txn_on:"2026-10-20",description:$desc,idempotency_key:$key}')
    argent_ok close_create Argent.PostPeriodClose "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.kind_a2004|tonumber) == 8' \
        "${recon_ledger}" || true
    close_txn=$(argent_grab close_create '.result.structuredContent.txn_id | tonumber')
    if [[ "${close_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok close_get Argent.GetTransaction "{\"txn_id\":${close_txn}}" \
            '(.result.structuredContent.transaction.kind_a2004|tonumber) == 8 and (.result.structuredContent.transaction.status_a2003|tonumber) == 3' \
            "${close_txn}" || true
        args=$(jq -n --argjson ledger "${recon_ledger:-0}" --arg desc "close-again${token}" --arg key "close${token}" \
            '{ledger_id:$ledger,txn_on:"2026-10-20",description:$desc,idempotency_key:$key}')
        argent_ok close_idem Argent.PostPeriodClose "${args}" \
            ".result.structuredContent.idempotent == true and .result.structuredContent.created == false and ((.result.structuredContent.txn_id|tonumber) == ${close_txn})" \
            "${recon_ledger}" || true
    else
        argent_miss close_get || true
        argent_miss close_idem || true
    fi
    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --arg desc "close2${token}" \
        '{ledger_id:$ledger,txn_on:"2026-10-18",description:$desc}')
    argent_confirm close_boundary_warn Argent.PostPeriodClose "${args}" "period close boundary" "${recon_ledger}" || true
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok close_boundary_absent Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"close2${token}\") == null" \
            "${org_id}" || true
    else
        argent_miss close_boundary_absent || true
    fi
    args=$(jq -nc --argjson id "${close_txn:-0}" --arg desc "closed2${token}" '{txn_id:$id,description:$desc}')
    argent_confirm close_edit_warn Argent.EditTransaction "${args}" "period close" "${close_txn}" || true
    if [[ "${close_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok close_edit_unchanged Argent.GetTransaction "{\"txn_id\":${close_txn}}" \
            ".result.structuredContent.transaction.description == \"close${token}\"" \
            "${close_txn}" || true
    else
        argent_miss close_edit_unchanged || true
    fi
    args=$(jq -nc --argjson id "${mid_txn:-0}" --arg desc "mid2${token}" '{txn_id:$id,description:$desc}')
    argent_confirm mid_edit_warn Argent.EditTransaction "${args}" "period close boundary" "${mid_txn}" || true
    if [[ "${mid_txn}" =~ ^[0-9]+$ ]]; then
        argent_ok mid_edit_unchanged Argent.GetTransaction "{\"txn_id\":${mid_txn}}" \
            ".result.structuredContent.transaction.description == \"mid${token}\" and (.result.structuredContent.transaction.status_a2003|tonumber) == 3" \
            "${mid_txn}" || true
    else
        argent_miss mid_edit_unchanged || true
    fi

    args=$(jq -nc --argjson id "${close_txn:-0}" '{txn_id:$id}')
    argent_confirm close_rescind_warn Argent.RescindTransaction "${args}" "period close" "${close_txn}" || true
    close_token=$(argent_grab close_rescind_warn '.result.structuredContent.confirm_token')
    if [[ -n "${close_token}" && "${close_txn}" =~ ^[0-9]+$ ]]; then
        args=$(jq -nc --argjson id "${close_txn}" --arg token "${close_token}" '{txn_id:$id,confirm_token:$token}')
        argent_ok close_rescind_apply Argent.RescindTransaction "${args}" \
            '(.result.structuredContent.status_a2003|tonumber) == 5 and .result.structuredContent.knocked == true' \
            "${close_txn}" || true
    else
        argent_miss close_rescind_apply || true
    fi
    args=$(jq -nc --argjson id "${close_txn:-0}" '{txn_id:$id}')
    argent_ok close_rescind_again Argent.RescindTransaction "${args}" \
        '.result.structuredContent.already == true and (.result.structuredContent.status_a2003|tonumber) == 5' \
        "${close_txn}" || true
    args=$(jq -nc --argjson id "${mid_txn:-0}" '{txn_id:$id}')
    argent_ok mid_rescind Argent.RescindTransaction "${args}" \
        '(.result.structuredContent.status_a2003|tonumber) == 5 and .result.structuredContent.knocked == false' \
        "${mid_txn}" || true
    argent_ok mid_rescind_again Argent.RescindTransaction "${args}" \
        '.result.structuredContent.already == true and (.result.structuredContent.status_a2003|tonumber) == 5' \
        "${mid_txn}" || true

    args=$(jq -nc --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    argent_confirm buy_rescind_warn Argent.RescindTransaction "${args}" "reconciled date" "${buy_txn}" || true
    rescind_token=$(argent_grab buy_rescind_warn '.result.structuredContent.confirm_token')
    if [[ -n "${rescind_token}" && "${buy_txn}" =~ ^[0-9]+$ ]]; then
        args=$(jq -nc --argjson id "${buy_txn}" --arg token "${rescind_token}" '{txn_id:$id,confirm_token:$token}')
        argent_ok buy_rescind_apply Argent.RescindTransaction "${args}" \
            '(.result.structuredContent.status_a2003|tonumber) == 5 and .result.structuredContent.knocked == true' \
            "${buy_txn}" || true
    else
        argent_miss buy_rescind_apply || true
    fi
    args=$(jq -nc --argjson id "${buy_txn:-0}" '{txn_id:$id}')
    argent_ok buy_rescind_again Argent.RescindTransaction "${args}" \
        '.result.structuredContent.already == true and (.result.structuredContent.status_a2003|tonumber) == 5' \
        "${buy_txn}" || true

    args=$(jq -nc --argjson id "${exp_stmt:-0}" '{txn_id:$id}')
    argent_ok exp_stmt_rescind Argent.RescindTransaction "${args}" \
        '(.result.structuredContent.status_a2003|tonumber) == 5 and .result.structuredContent.knocked == false' \
        "${exp_stmt}" || true
    args=$(jq -n --argjson ledger "${exp_id:-0}" --argjson stmt "${exp_stmt:-0}" \
        '{ledger_id:$ledger,statement_txn_id:$stmt,reconciled_on:"2026-10-07",statement_balance_cents:0}')
    argent_fail start_rescinded Argent.StartReconciliation "${args}" statement_rescinded "${exp_id}" "${exp_stmt}" || true

    args=$(jq -n --argjson ledger "${org2_ledger:-0}" --arg desc "o2stmt${token}" \
        '{ledger_id:$ledger,txn_on:"2026-10-07",description:$desc}')
    argent_ok org2_stmt Argent.PostStatement "${args}" '.result.structuredContent.created == true' \
        "${org2_ledger}" || true
    org2_stmt_id=$(argent_grab org2_stmt '.result.structuredContent.txn_id | tonumber')
    args=$(jq -n --argjson ledger "${recon_ledger:-0}" --argjson stmt "${org2_stmt_id:-0}" \
        '{ledger_id:$ledger,statement_txn_id:$stmt,reconciled_on:"2026-10-07",statement_balance_cents:0}')
    argent_fail start_other_org Argent.StartReconciliation "${args}" organization_id "${recon_ledger}" "${org2_stmt_id}" || true
}
