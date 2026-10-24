#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, suffix, etc.) are set by argent_mcp_exercise
# shellcheck disable=SC2034 # line_id may be unused in some paths but needed for completeness
# shellcheck disable=SC2312 # diagnostic command substitutions intentionally swallow exit codes

# Argent transaction, tax, and listing exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_ok, argent_fail, argent_miss,
# argent_grab) and the exercise-local variables from argent_mcp_exercise:
# token, suffix, day, org_id, org2_id, bank_id, card_id, exp_id, tax_id,
# org2_ledger, txn_id, line_id, tax_code_id, tax_rate_id, bare_code_id,
# usd_code_id, rate2_id

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_txn() {
    local args

    args=$(jq -n '{fiscal_year_start_month:1,fiscal_year_start_day:1,default_currency:"cad"}')
    argent_fail txn_lines_required Argent.PostTransaction '{"txn_on":"2026-10-07","description":"x"}' lines_required || true
    argent_fail txn_lines_empty Argent.PostTransaction '{"txn_on":"2026-10-07","description":"x","lines":[]}' lines_required || true
    argent_fail txn_line_shape Argent.PostTransaction '{"txn_on":"2026-10-07","description":"x","lines":[1]}' lines || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"bad",description:"x",lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_date Argent.PostTransaction "${args}" txn_on "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_description Argent.PostTransaction "${args}" description_required "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",kind_a2004:6,lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_kind_6 Argent.PostTransaction "${args}" kind "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",kind_a2004:7,lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_kind_7 Argent.PostTransaction "${args}" kind "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",kind_a2004:8,lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_kind_8 Argent.PostTransaction "${args}" kind "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",kind_a2004:0,lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_kind_0 Argent.PostTransaction "${args}" kind "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",kind_a2004:12,lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_kind_12 Argent.PostTransaction "${args}" kind "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",status_a2003:4,lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_status_4 Argent.PostTransaction "${args}" status "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",status_a2003:5,lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_status_5 Argent.PostTransaction "${args}" status "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",lines:[{ledger_id:999999999,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_ledger_unknown Argent.PostTransaction "${args}" ledger "${exp_id}" || true
    args=$(jq -n --argjson parent "${parent_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"k",lines:[{ledger_id:$parent,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_ledger_nonposting Argent.PostTransaction "${args}" ledger "${parent_id}" "${exp_id}" || true
    args=$(jq -n --argjson org "${org2_id:-0}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{organization_id:$org,txn_on:"2026-10-07",description:"k",lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1}]}')
    argent_fail txn_org_mismatch Argent.PostTransaction "${args}" organization "${org2_id}" "${bank_id}" "${exp_id}" || true
    args=$(jq -n --arg desc "unbal${token}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:$desc,lines:[{ledger_id:$bank,amount_cents:100},{ledger_id:$exp,amount_cents:50}]}')
    argent_fail txn_unbalanced Argent.PostTransaction "${args}" unbalanced "${bank_id}" "${exp_id}" || true

    args=$(jq -n --arg desc "bal${token}" --arg key "post${token}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:$desc,idempotency_key:$key,lines:[{ledger_id:$bank,amount_cents:-100},{ledger_id:$exp,amount_cents:100}]}')
    argent_ok txn_balanced Argent.PostTransaction "${args}" \
        '.result.structuredContent.created == true and ((.result.structuredContent.txn_id|tonumber) > 0) and (.result.structuredContent.lines|length) == 2' \
        "${bank_id}" "${exp_id}" || true
    txn_id=$(argent_grab txn_balanced '.result.structuredContent.txn_id | tonumber')
    line_id=$(argent_grab txn_balanced '.result.structuredContent.lines[0].line_id | tonumber')
    args=$(jq -n --arg desc "bal-again${token}" --arg key "post${token}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:$desc,idempotency_key:$key,lines:[{ledger_id:$bank,amount_cents:-100},{ledger_id:$exp,amount_cents:100}]}')
    if [[ "${txn_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_idem Argent.PostTransaction "${args}" \
            ".result.structuredContent.idempotent == true and .result.structuredContent.created == false and ((.result.structuredContent.txn_id|tonumber) == ${txn_id})" \
            "${bank_id}" "${exp_id}" || true
    else
        argent_miss txn_idem
    fi
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id}')
    argent_ok txn_get_defaults Argent.GetTransaction "${args}" \
        '(.result.structuredContent.transaction.kind_a2004 | tonumber) == 11 and (.result.structuredContent.transaction.status_a2003 | tonumber) == 3' \
        "${txn_id}" || true
    argent_fail txn_get_missing Argent.GetTransaction '{}' txn_id_required || true
    argent_fail txn_get_unknown Argent.GetTransaction '{"txn_id":999999999}' not_found || true

    argent_fail txn_list_filter Argent.ListTransactions '{}' filter_required || true
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,date_from:"bad"}')
    argent_fail txn_list_date Argent.ListTransactions "${args}" date_from "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_has Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"bal${token}\") != null" \
            "${org_id}" || true
        argent_ok txn_list_misses_unbal Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"unbal${token}\") == null" \
            "${org_id}" || true
    else
        argent_miss txn_list_has
        argent_miss txn_list_misses_unbal
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,kind:11}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_kind_11 Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"bal${token}\") != null and index(\"Opening balance\") == null" \
            "${org_id}" || true
    else
        argent_miss txn_list_kind_11
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,kind:6}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_kind_6 Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"Opening balance\") != null and index(\"bal${token}\") == null" \
            "${org_id}" || true
    else
        argent_miss txn_list_kind_6
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,status:[3]}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_status_3 Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"bal${token}\") != null" \
            "${org_id}" || true
    else
        argent_miss txn_list_status_3
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,status:[1]}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_status_1 Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"bal${token}\") == null" \
            "${org_id}" || true
    else
        argent_miss txn_list_status_1
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,status:[1,2,3,4,5]}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_status_five Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"bal${token}\") != null" \
            "${org_id}" || true
    else
        argent_miss txn_list_status_five
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,status:[1,2,3,4,5,1]}')
    argent_fail txn_list_status_six Argent.ListTransactions "${args}" status "${org_id}" || true
    args=$(jq -n --argjson ledger "${exp_id:-0}" '{ledger_id:$ledger}')
    if [[ "${exp_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_ledger_exp Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"bal${token}\") != null" \
            "${exp_id}" || true
    else
        argent_miss txn_list_ledger_exp
    fi
    args=$(jq -n --argjson ledger "${card_id:-0}" '{ledger_id:$ledger}')
    if [[ "${card_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_ledger_card Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"bal${token}\") == null" \
            "${card_id}" || true
    else
        argent_miss txn_list_ledger_card
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,date_from:"2099-01-01"}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_list_future Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"bal${token}\") == null" \
            "${org_id}" || true
    else
        argent_miss txn_list_future
    fi

    args=$(jq -n --argjson org "${org_id:-0}" --argjson target "${tax_id:-0}" \
        '{organization_id:$org,target_ledger_id:$target,name:"GST"}')
    argent_fail tax_code_required Argent.UpsertTaxCode "${args}" code_required "${org_id}" "${tax_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --argjson target "${tax_id:-0}" \
        '{organization_id:$org,target_ledger_id:$target,code:"GST"}')
    argent_fail tax_name_required Argent.UpsertTaxCode "${args}" name_required "${org_id}" "${tax_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,code:"GST",name:"GST"}')
    argent_fail tax_target_required Argent.UpsertTaxCode "${args}" target_required "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --argjson target "${tax_id:-0}" --arg code "$(printf '%51s' '' | tr ' ' 'c')" \
        '{organization_id:$org,target_ledger_id:$target,code:$code,name:"long"}')
    argent_fail tax_code_long Argent.UpsertTaxCode "${args}" code "${org_id}" "${tax_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --argjson target "${parent_id:-0}" \
        '{organization_id:$org,target_ledger_id:$target,code:"BAD",name:"bad"}')
    argent_fail tax_target_nonposting Argent.UpsertTaxCode "${args}" target "${org_id}" "${parent_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --argjson target "${org2_ledger:-0}" \
        '{organization_id:$org,target_ledger_id:$target,code:"OTH",name:"other"}')
    argent_fail tax_target_other_org Argent.UpsertTaxCode "${args}" target "${org_id}" "${org2_ledger}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg code "gst${suffix}" --argjson target "${tax_id:-0}" \
        '{organization_id:$org,code:$code,name:"GST",target_ledger_id:$target}')
    argent_ok tax_create Argent.UpsertTaxCode "${args}" '.result.structuredContent.created == true' \
        "${org_id}" "${tax_id}" || true
    tax_code_id=$(argent_grab tax_create '.result.structuredContent.tax_code_id | tonumber')
    args=$(jq -n --argjson id "${tax_code_id:-0}" --arg name "GST renamed" '{tax_code_id:$id,name:$name}')
    argent_ok tax_update Argent.UpsertTaxCode "${args}" '.result.structuredContent.updated == true' \
        "${tax_code_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg code "norate${suffix}" --argjson target "${tax_id:-0}" \
        '{organization_id:$org,code:$code,name:"No rate",target_ledger_id:$target}')
    argent_ok tax_bare Argent.UpsertTaxCode "${args}" '.result.structuredContent.created == true' \
        "${org_id}" "${tax_id}" || true
    bare_code_id=$(argent_grab tax_bare '.result.structuredContent.tax_code_id | tonumber')
    args=$(jq -n --argjson org "${org_id:-0}" --arg code "usd${suffix}" --argjson target "${usd_id:-0}" \
        '{organization_id:$org,code:$code,name:"USD tax",target_ledger_id:$target}')
    argent_ok tax_usd_code Argent.UpsertTaxCode "${args}" '.result.structuredContent.created == true' \
        "${org_id}" "${usd_id}" || true
    usd_code_id=$(argent_grab tax_usd_code '.result.structuredContent.tax_code_id | tonumber')

    argent_fail tax_rate_date Argent.UpsertTaxRate '{"tax_code_id":1,"effective_on":"bad","rate_bps":500}' effective_on || true
    args=$(jq -n --argjson id "${tax_code_id:-0}" '{tax_code_id:$id,effective_on:"2020-01-01"}')
    argent_fail tax_rate_required Argent.UpsertTaxRate "${args}" rate_bps_required "${tax_code_id}" || true
    args=$(jq -n --argjson id "${tax_code_id:-0}" '{tax_code_id:$id,effective_on:"2020-01-01",rate_bps:500}')
    argent_ok tax_rate_create Argent.UpsertTaxRate "${args}" '.result.structuredContent.created == true' \
        "${tax_code_id}" || true
    tax_rate_id=$(argent_grab tax_rate_create '.result.structuredContent.tax_rate_id | tonumber')
    args=$(jq -n --argjson id "${tax_rate_id:-0}" '{tax_rate_id:$id,rate_bps:500,summary:"five percent"}')
    argent_ok tax_rate_update Argent.UpsertTaxRate "${args}" '.result.structuredContent.updated == true' \
        "${tax_rate_id}" || true
    args=$(jq -n --argjson id "${tax_code_id:-0}" '{tax_code_id:$id,effective_on:"2019-06-01",rate_bps:0}')
    argent_ok tax_rate_second Argent.UpsertTaxRate "${args}" '.result.structuredContent.created == true' \
        "${tax_code_id}" || true
    rate2_id=$(argent_grab tax_rate_second '.result.structuredContent.tax_rate_id | tonumber')
    args=$(jq -n --argjson id "${rate2_id:-0}" '{tax_rate_id:$id,effective_on:"2020-01-01"}')
    argent_fail tax_rate_duplicate Argent.UpsertTaxRate "${args}" duplicate "${rate2_id}" || true
    args=$(jq -n --argjson id "${bare_code_id:-0}" '{tax_code_id:$id,effective_on:"2099-01-01",rate_bps:500}')
    argent_ok tax_rate_future Argent.UpsertTaxRate "${args}" '.result.structuredContent.created == true' \
        "${bare_code_id}" || true
    args=$(jq -n --argjson id "${usd_code_id:-0}" '{tax_code_id:$id,effective_on:"2020-01-01",rate_bps:500}')
    argent_ok tax_rate_usd Argent.UpsertTaxRate "${args}" '.result.structuredContent.created == true' \
        "${usd_code_id}" || true

    args=$(jq -n --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"taxcents",lines:[{ledger_id:$bank,amount_cents:-1},{ledger_id:$exp,amount_cents:1,tax_cents:1}]}')
    argent_fail txn_tax_cents Argent.PostTransaction "${args}" tax_cents "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson code "${bare_code_id:-0}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"norate",lines:[{ledger_id:$bank,amount_cents:-10000},{ledger_id:$exp,amount_cents:10000,tax_code_id:$code}]}')
    argent_fail txn_no_tax_rate Argent.PostTransaction "${args}" no_tax_rate "${bare_code_id}" "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson code "${usd_code_id:-0}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:"taxcur",lines:[{ledger_id:$bank,amount_cents:-10000},{ledger_id:$exp,amount_cents:10000,tax_code_id:$code}]}')
    argent_fail txn_tax_currency Argent.PostTransaction "${args}" tax_currency "${usd_code_id}" "${bank_id}" "${exp_id}" || true
    args=$(jq -n --arg desc "net${token}" --argjson code "${tax_code_id:-0}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:$desc,lines:[{ledger_id:$bank,amount_cents:-10500},{ledger_id:$exp,amount_cents:10000,tax_code_id:$code}]}')
    if [[ "${exp_id}" =~ ^[0-9]+$ && "${tax_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_tax_net Argent.PostTransaction "${args}" \
            "(.result.structuredContent.lines|length) == 3 and ([.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${exp_id}) | .amount_cents | tonumber] | .[0]) == 10000 and ([.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${tax_id}) | .amount_cents | tonumber] | .[0]) == 500 and ([.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${exp_id}) | .tax_manual | tonumber] | .[0]) == 0" \
            "${tax_code_id}" "${bank_id}" "${exp_id}" || true
    else
        argent_miss txn_tax_net
    fi
    args=$(jq -n --arg desc "gross${token}" --argjson code "${tax_code_id:-0}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:$desc,lines:[{ledger_id:$bank,amount_cents:-10500},{ledger_id:$exp,amount_cents:10500,tax_code_id:$code,tax_gross:true}]}')
    if [[ "${exp_id}" =~ ^[0-9]+$ && "${tax_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_tax_gross Argent.PostTransaction "${args}" \
            "([.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${exp_id}) | .amount_cents | tonumber] | .[0]) == 10000 and ([.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${tax_id}) | .amount_cents | tonumber] | .[0]) == 500" \
            "${tax_code_id}" "${bank_id}" "${exp_id}" || true
    else
        argent_miss txn_tax_gross
    fi
    args=$(jq -n --arg desc "manual${token}" --argjson code "${tax_code_id:-0}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:$desc,lines:[{ledger_id:$bank,amount_cents:-10501},{ledger_id:$exp,amount_cents:10000,tax_code_id:$code,tax_cents:501}]}')
    if [[ "${exp_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_tax_manual Argent.PostTransaction "${args}" \
            "([.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${exp_id}) | .tax_manual | tonumber] | .[0]) == 1 and ([.result.structuredContent.lines[] | select((.ledger_id|tonumber)==${exp_id}) | .tax_cents | tonumber] | .[0]) == 501" \
            "${tax_code_id}" "${bank_id}" "${exp_id}" || true
    else
        argent_miss txn_tax_manual
    fi
    args=$(jq -n --arg desc "confirm${token}" --argjson code "${tax_code_id:-0}" --argjson bank "${bank_id:-0}" --argjson exp "${exp_id:-0}" \
        '{txn_on:"2026-10-07",description:$desc,lines:[{ledger_id:$bank,amount_cents:-10500},{ledger_id:$exp,amount_cents:10000,tax_code_id:$code,tax_cents:100}]}')
    argent_fail txn_needs_confirm Argent.PostTransaction "${args}" needs_confirm "${tax_code_id}" "${bank_id}" "${exp_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org}')
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_confirm_absent Argent.ListTransactions "${args}" \
            "[.result.structuredContent.transactions[]?.description] | index(\"confirm${token}\") == null" \
            "${org_id}" || true
    else
        argent_miss txn_confirm_absent
    fi
}
