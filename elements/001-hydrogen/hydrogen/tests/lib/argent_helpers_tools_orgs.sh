#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, etc.) are set by argent_mcp_exercise

# Argent organization and tools-list exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_rpc, argent_call, argent_ok,
# argent_fail, argent_confirm, argent_grab, and the exercise-local variables
# set by argent_mcp_exercise).

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_tools_list_orgs() {
    local args payload tool list_filter
    local -a tools

    tools=(
        ListOrganizations UpsertOrganization
        ListLedgers GetLedger UpsertLedger
        UpsertLedgerTerms UpsertContact
        PostTransaction
        AddTags RemoveTags AddAttachment
        UpsertTaxCode UpsertTaxRate
        GetTransaction ListTransactions QueryBalances
        EditTransaction RescindTransaction
        PostStatement PostPeriodClose
        StartReconciliation ClearLines CompleteReconciliation
        UpsertSchedule GenerateSchedule MatchReserved RetryCalendar
        QueryDue QueryReconciliationStatus QueryLedgerHistory
        QueryTaxSummary QueryIncomeExpense QueryCalendarView
        QueryFxPremium QuerySyncProblems Search ListRates
        UpsertRate GetBocRate
    )
    list_filter='.result.error == null'
    for tool in "${tools[@]}"; do
        list_filter="${list_filter} and ([.result.tools[].name] | index(\"Argent.${tool}\") != null)"
    done
    payload=$(jq -nc --argjson id 3 \
        '{jsonrpc:"2.0",id:$id,method:"tools/list",params:{page_size:500}}')
    argent_rpc "tools_list" "${payload}" "${list_filter}" || true

    args=$(jq -n '{fiscal_year_start_month:1,fiscal_year_start_day:1,default_currency:"cad"}')
    argent_fail org_name_required Argent.UpsertOrganization "${args}" name_required || true
    args=$(jq -n --arg name "bad${token}" '{name:$name,fiscal_year_start_month:1,fiscal_year_start_day:1,default_currency:"cad",status_a2000:9}')
    argent_fail org_status Argent.UpsertOrganization "${args}" status || true
    args=$(jq -n --arg name "bad${token}" '{name:$name,fiscal_year_start_month:13,fiscal_year_start_day:1,default_currency:"cad"}')
    argent_fail org_fiscal_month Argent.UpsertOrganization "${args}" fiscal_month || true
    args=$(jq -n --arg name "bad${token}" '{name:$name,fiscal_year_start_month:1,fiscal_year_start_day:0,default_currency:"cad"}')
    argent_fail org_fiscal_day Argent.UpsertOrganization "${args}" fiscal_day || true
    args=$(jq -n --arg name "bad${token}" '{name:$name,fiscal_year_start_month:1.5,fiscal_year_start_day:1,default_currency:"cad"}')
    argent_fail org_fiscal_fraction Argent.UpsertOrganization "${args}" fiscal_month || true
    args=$(jq -n --arg name "bad${token}" '{name:$name,fiscal_year_start_month:1,fiscal_year_start_day:1,default_currency:"ca"}')
    argent_fail org_currency_shape Argent.UpsertOrganization "${args}" currency || true
    args=$(jq -n --arg name "bad${token}" '{name:$name,fiscal_year_start_month:1,fiscal_year_start_day:1,default_currency:"zzz"}')
    argent_fail org_currency_unknown Argent.UpsertOrganization "${args}" currency || true

    args=$(jq -n --arg name "org${token}" '{name:$name,fiscal_year_start_month:4,fiscal_year_start_day:1,default_currency:"CAD"}')
    argent_ok org_create Argent.UpsertOrganization "${args}" \
        '.result.structuredContent.created == true and ((.result.structuredContent.organization_id|tonumber) > 0)' || true
    org_id=$(argent_grab org_create '.result.structuredContent.organization_id | tonumber')

    args='{}'
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok org_list_has Argent.ListOrganizations "${args}" \
            "[.result.structuredContent.organizations[]?.organization_id | tonumber] | index(${org_id}) != null" \
            "${org_id}" || true
        argent_ok org_list_currency Argent.ListOrganizations "${args}" \
            "[.result.structuredContent.organizations[] | select((.organization_id|tonumber)==${org_id}) | .default_currency | gsub(\" \";\"\")] | .[0] == \"cad\"" \
            "${org_id}" || true
        argent_ok org_list_status Argent.ListOrganizations "${args}" \
            "[.result.structuredContent.organizations[] | select((.organization_id|tonumber)==${org_id}) | .status_a2000 | tonumber] | .[0] == 1" \
            "${org_id}" || true
        argent_ok org_list_month Argent.ListOrganizations "${args}" \
            "[.result.structuredContent.organizations[] | select((.organization_id|tonumber)==${org_id}) | .fiscal_year_start_month | tonumber] | .[0] == 4" \
            "${org_id}" || true
    else
        argent_miss org_list_has
        argent_miss org_list_currency
        argent_miss org_list_status
        argent_miss org_list_month
    fi

    args=$(jq -n --argjson id "${org_id:-0}" --arg name "orgb${token}" '{organization_id:$id,name:$name}')
    argent_ok org_rename Argent.UpsertOrganization "${args}" \
        '.result.structuredContent.updated == true and .result.structuredContent.created != true' \
        "${org_id}" || true
    args=$(jq -n --argjson id "${org_id:-0}" --arg summary "hello" '{organization_id:$id,summary:$summary}')
    argent_ok org_summary Argent.UpsertOrganization "${args}" '.result.structuredContent.updated == true' \
        "${org_id}" || true
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok org_summary_seen Argent.ListOrganizations '{}' \
            "[.result.structuredContent.organizations[] | select((.organization_id|tonumber)==${org_id}) | .summary] | .[0] == \"hello\"" \
            "${org_id}" || true
    else
        argent_miss org_summary_seen
    fi
    args=$(jq -n --argjson id "${org_id:-0}" '{organization_id:$id,summary:""}')
    argent_ok org_summary_clear Argent.UpsertOrganization "${args}" '.result.structuredContent.updated == true' \
        "${org_id}" || true
    if [[ "${org_id}" =~ ^[0-9]+$ ]]; then
        argent_ok org_summary_cleared Argent.ListOrganizations '{}' \
            "[.result.structuredContent.organizations[] | select((.organization_id|tonumber)==${org_id}) | .summary] | .[0] | (. == null or . == \"\")" \
            "${org_id}" || true
    else
        argent_miss org_summary_cleared
    fi
    args=$(jq -n --argjson id "${org_id:-0}" '{organization_id:$id,status_a2000:2}')
    argent_ok org_archive Argent.UpsertOrganization "${args}" '.result.structuredContent.updated == true' \
        "${org_id}" || true
    args=$(jq -n --argjson id "${org_id:-0}" '{organization_id:$id,status_a2000:9}')
    argent_fail org_status_keeps Argent.UpsertOrganization "${args}" status "${org_id}" || true
    args=$(jq -n '{organization_id:999999999,name:"missing"}')
    argent_fail org_not_found Argent.UpsertOrganization "${args}" not_found || true
}
