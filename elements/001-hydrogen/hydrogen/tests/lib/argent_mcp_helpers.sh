#!/usr/bin/env bash

# Argent MCP tool blackbox helpers for tests/test_73_argent_mcp.sh.
# One case per tool call. Errors are structuredContent.code, HTTP 200.
# Local variable names stay lowercase so test_03 does not require them.

# shellcheck disable=SC2154 # TEST_NUMBER and the MCP helpers come from the caller
# shellcheck disable=SC2312 # jq and curl status are checked by the caller

# CHANGELOG
# 1.0.0 - 2026-10-07 - Argent tool calls and validation variants
# 1.0.1 - 2026-10-07 - Quote the contact_not_found arguments so jq accepts them
# 1.0.2 - 2026-10-07 - One tool call, root error text, tax rate in force, parent rate check

[[ -n "${ARGENT_MCP_HELPERS_GUARD:-}" ]] && return 0
export ARGENT_MCP_HELPERS_GUARD="true"

ARGENT_MCP_HELPERS_NAME="Argent MCP Helpers"
ARGENT_MCP_HELPERS_VERSION="1.0.2"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
    "${ARGENT_MCP_HELPERS_NAME} ${ARGENT_MCP_HELPERS_VERSION}" "info"

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
    # One call. A wrong tool result is not a slow response, so this does not
    # retry or print the body as an INFO delay line.
    http_st=$(mcp_http "POST" "${ARGENT_URL}" "${payload}" "${out}" \
        "${ARGENT_HDR}" "${ARGENT_JWT}" "${ARGENT_SESSION}" "" "${HTTP_TIMEOUT}")
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

    tools=(
        ListOrganizations UpsertOrganization
        ListLedgers GetLedger UpsertLedger
        UpsertLedgerTerms UpsertContact
        PostTransaction
        AddTags RemoveTags AddAttachment
        UpsertTaxCode UpsertTaxRate
        GetTransaction ListTransactions QueryBalances
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

    args=$(jq -n --arg name "bank${token}" '{name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_fail ledger_org_required Argent.UpsertLedger "${args}" organization_id_required || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:9,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_fail ledger_type Argent.UpsertLedger "${args}" ledger_type "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,status_a2002:4,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_fail ledger_status Argent.UpsertLedger "${args}" status "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"c",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_fail ledger_currency_shape Argent.UpsertLedger "${args}" currency "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"zzz",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_fail ledger_currency_unknown Argent.UpsertLedger "${args}" currency "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07"}')
    argent_fail ledger_opening_required Argent.UpsertLedger "${args}" opening_balance "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:100}')
    argent_fail ledger_offset_required Argent.UpsertLedger "${args}" offset_required "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" --arg mask "$(printf '%51s' '' | tr ' ' 'm')" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0,mask:$mask}')
    argent_fail ledger_mask Argent.UpsertLedger "${args}" mask "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" --arg ref "$(printf '%101s' '' | tr ' ' 'r')" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0,external_ref:$ref}')
    argent_fail ledger_external_ref Argent.UpsertLedger "${args}" external_ref "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" --arg key 'bad"key' \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0,idempotency_key:$key}')
    argent_fail ledger_idem_quote Argent.UpsertLedger "${args}" idempotency_key "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" --arg key 'bad\key' \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0,idempotency_key:$key}')
    argent_fail ledger_idem_slash Argent.UpsertLedger "${args}" idempotency_key "${org_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bad${token}" --arg key "${long_key}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0,idempotency_key:$key}')
    argent_fail ledger_idem_long Argent.UpsertLedger "${args}" idempotency_key "${org_id}" || true
    args=$(jq -n --arg name "bad${token}" \
        '{organization_id:999999999,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_fail ledger_org_missing Argent.UpsertLedger "${args}" not_found || true

    args=$(jq -n --argjson org "${org_id:-0}" --arg name "bank${token}" --arg key "bank${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,is_posting:true,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0,idempotency_key:$key}')
    argent_ok ledger_bank Argent.UpsertLedger "${args}" \
        '.result.structuredContent.created == true and ((.result.structuredContent.opening_txn_id|tonumber) > 0)' \
        "${org_id}" || true
    bank_id=$(argent_grab ledger_bank '.result.structuredContent.ledger_id | tonumber')

    args=$(jq -n --argjson org "${org_id:-0}" --argjson offset "${bank_id:-0}" --arg name "card${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,is_posting:true,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:500,offset_ledger_id:$offset}')
    argent_ok ledger_card Argent.UpsertLedger "${args}" \
        '.result.structuredContent.created == true and ((.result.structuredContent.opening_txn_id|tonumber) > 0)' \
        "${org_id}" "${bank_id}" || true
    card_id=$(argent_grab ledger_card '.result.structuredContent.ledger_id | tonumber')

    args=$(jq -n --argjson org "${org_id:-0}" --arg name "exp${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:5,is_posting:true,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_ok ledger_exp Argent.UpsertLedger "${args}" '.result.structuredContent.created == true' \
        "${org_id}" || true
    exp_id=$(argent_grab ledger_exp '.result.structuredContent.ledger_id | tonumber')

    args=$(jq -n --argjson org "${org_id:-0}" --arg name "tax${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:2,is_posting:true,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_ok ledger_tax Argent.UpsertLedger "${args}" '.result.structuredContent.created == true' \
        "${org_id}" || true
    tax_id=$(argent_grab ledger_tax '.result.structuredContent.ledger_id | tonumber')

    args=$(jq -n --argjson org "${org_id:-0}" --arg name "usd${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,is_posting:true,currency:"usd",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_ok ledger_usd Argent.UpsertLedger "${args}" '.result.structuredContent.created == true' \
        "${org_id}" || true
    usd_id=$(argent_grab ledger_usd '.result.structuredContent.ledger_id | tonumber')

    args=$(jq -n --argjson org "${org_id:-0}" --arg name "parent${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,is_posting:0,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_ok ledger_parent Argent.UpsertLedger "${args}" \
        '.result.structuredContent.created == true and .result.structuredContent.opening_txn_id == null' \
        "${org_id}" || true
    parent_id=$(argent_grab ledger_parent '.result.structuredContent.ledger_id | tonumber')

    args=$(jq -n --argjson org "${org_id:-0}" --argjson parent "${parent_id:-0}" --arg name "child${token}" \
        '{organization_id:$org,name:$name,parent_id:$parent,ledger_type_a2001:1,is_posting:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_ok ledger_child Argent.UpsertLedger "${args}" '.result.structuredContent.created == true' \
        "${org_id}" "${parent_id}" || true
    child_id=$(argent_grab ledger_child '.result.structuredContent.ledger_id | tonumber')

    args=$(jq -n --argjson id "${bank_id:-0}" --arg day "${day}" '{ledger_id:$id,as_of:$day}')
    argent_ok ledger_get_bank Argent.GetLedger "${args}" \
        '(.result.structuredContent.balance.balance_cents | tonumber) == -500' \
        "${bank_id}" || true
    args=$(jq -n --argjson id "${card_id:-0}" --arg day "${day}" '{ledger_id:$id,as_of:$day}')
    argent_ok ledger_get_card Argent.GetLedger "${args}" \
        '(.result.structuredContent.balance.balance_cents | tonumber) == 500' \
        "${card_id}" || true
    args=$(jq -n --argjson id "${exp_id:-0}" --arg day "${day}" '{ledger_id:$id,as_of:$day}')
    argent_ok ledger_get_exp Argent.GetLedger "${args}" \
        '(.result.structuredContent.balance.balance_cents | tonumber) == 0' \
        "${exp_id}" || true
    args=$(jq -n --argjson id "${parent_id:-0}" --arg day "${day}" '{ledger_id:$id,as_of:$day}')
    argent_ok ledger_get_parent Argent.GetLedger "${args}" '.result.structuredContent.balance == null' \
        "${parent_id}" || true
    args=$(jq -n --argjson id "${child_id:-0}" '{ledger_id:$id}')
    argent_ok ledger_get_as_of Argent.GetLedger "${args}" \
        '(.result.structuredContent.as_of | type) == "string" and (.result.structuredContent.as_of | test("^[0-9]{4}-[0-9]{2}-[0-9]{2}"))' \
        "${child_id}" || true
    argent_fail ledger_get_missing Argent.GetLedger '{}' ledger_id_required || true
    argent_fail ledger_get_unknown Argent.GetLedger '{"ledger_id":999999999}' not_found || true
    args=$(jq -n --argjson id "${bank_id:-0}" --arg day "${day}" '{ledger_id:$id,as_of:$day,status:[1,2,3,4,5,1]}')
    argent_fail ledger_get_status_six Argent.GetLedger "${args}" status "${bank_id}" || true

    args=$(jq -n --argjson id "${bank_id:-0}" --arg name "bankb${token}" '{ledger_id:$id,name:$name}')
    argent_ok ledger_rename Argent.UpsertLedger "${args}" \
        '.result.structuredContent.updated == true and .result.structuredContent.created == false' \
        "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id,currency:"usd"}')
    argent_fail ledger_currency_fixed Argent.UpsertLedger "${args}" currency_fixed "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id,organization_id:999999999}')
    argent_fail ledger_org_fixed Argent.UpsertLedger "${args}" organization_fixed "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id,parent_id:$id}')
    argent_fail ledger_self_parent Argent.UpsertLedger "${args}" parent "${bank_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "other${token}" --arg key "bank${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0,idempotency_key:$key}')
    if [[ "${bank_id}" =~ ^[0-9]+$ ]]; then
        argent_ok ledger_idem Argent.UpsertLedger "${args}" \
            ".result.structuredContent.idempotent == true and .result.structuredContent.created == false and ((.result.structuredContent.ledger_id|tonumber) == ${bank_id})" \
            "${org_id}" || true
        argent_ok ledger_idem_name Argent.GetLedger "{\"ledger_id\":${bank_id}}" \
            ".result.structuredContent.ledger.name == \"bankb${token}\"" \
            "${bank_id}" || true
    else
        argent_miss ledger_idem
        argent_miss ledger_idem_name
    fi

    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org}')
    if [[ "${parent_id}" =~ ^[0-9]+$ && "${bank_id}" =~ ^[0-9]+$ ]]; then
        argent_ok ledger_list_posting Argent.ListLedgers "${args}" \
            "([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${parent_id})) == null and ([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${bank_id})) != null" \
            "${org_id}" || true
    else
        argent_miss ledger_list_posting
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,include_non_posting:true}')
    if [[ "${parent_id}" =~ ^[0-9]+$ ]]; then
        argent_ok ledger_list_parent Argent.ListLedgers "${args}" \
            "([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${parent_id})) != null" \
            "${org_id}" || true
    else
        argent_miss ledger_list_parent
    fi
    args=$(jq -n --argjson org "${org_id:-0}" '{organization_id:$org,type:5}')
    if [[ "${exp_id}" =~ ^[0-9]+$ && "${bank_id}" =~ ^[0-9]+$ ]]; then
        argent_ok ledger_list_type Argent.ListLedgers "${args}" \
            "([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${exp_id})) != null and ([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${bank_id})) == null" \
            "${org_id}" || true
    else
        argent_miss ledger_list_type
    fi

    args=$(jq -n --argjson org "${org_id:-0}" --argjson offset "${usd_id:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:100,offset_ledger_id:$offset}')
    argent_fail ledger_offset_currency Argent.UpsertLedger "${args}" offset "${org_id}" "${usd_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --argjson offset "${parent_id:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:100,offset_ledger_id:$offset}')
    argent_fail ledger_offset_nonposting Argent.UpsertLedger "${args}" offset "${org_id}" "${parent_id}" || true

    args=$(jq -n --arg name "org2${token}" '{name:$name,fiscal_year_start_month:1,fiscal_year_start_day:1,default_currency:"cad"}')
    argent_ok org2_create Argent.UpsertOrganization "${args}" '.result.structuredContent.created == true' || true
    org2_id=$(argent_grab org2_create '.result.structuredContent.organization_id | tonumber')
    args=$(jq -n --argjson org "${org2_id:-0}" --arg name "o2${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,is_posting:true,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_ok ledger_org2 Argent.UpsertLedger "${args}" '.result.structuredContent.created == true' \
        "${org2_id}" || true
    org2_ledger=$(argent_grab ledger_org2 '.result.structuredContent.ledger_id | tonumber')
    args=$(jq -n --argjson org "${org_id:-0}" --argjson parent "${org2_ledger:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,parent_id:$parent,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:0}')
    argent_fail ledger_parent_other_org Argent.UpsertLedger "${args}" parent "${org_id}" "${org2_ledger}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --argjson offset "${org2_ledger:-0}" --arg name "bad${token}" \
        '{organization_id:$org,name:$name,ledger_type_a2001:1,currency:"cad",opening_on:"2026-10-07",opening_balance_cents:100,offset_ledger_id:$offset}')
    argent_fail ledger_offset_other_org Argent.UpsertLedger "${args}" offset "${org_id}" "${org2_ledger}" || true

    argent_fail terms_ledger_required Argent.UpsertLedgerTerms '{"effective_on":"2026-01-01"}' ledger_id_required || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id,effective_on:"bad"}')
    argent_fail terms_date Argent.UpsertLedgerTerms "${args}" effective_on "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id,effective_on:"2026-01-01",statement_close_day:32}')
    argent_fail terms_close_day Argent.UpsertLedgerTerms "${args}" statement_close_day "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" \
        '{ledger_id:$id,effective_on:"2026-01-01",credit_limit_cents:100000,statement_close_day:15}')
    argent_ok terms_create Argent.UpsertLedgerTerms "${args}" '.result.structuredContent.created == true' \
        "${bank_id}" || true
    term_id=$(argent_grab terms_create '.result.structuredContent.ledger_term_id | tonumber')
    args=$(jq -n --argjson id "${term_id:-0}" '{ledger_term_id:$id,credit_limit_cents:200000}')
    argent_ok terms_update Argent.UpsertLedgerTerms "${args}" '.result.structuredContent.updated == true' \
        "${term_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id,effective_on:"2026-06-01",credit_limit_cents:1}')
    argent_ok terms_second Argent.UpsertLedgerTerms "${args}" '.result.structuredContent.created == true' \
        "${bank_id}" || true
    term2_id=$(argent_grab terms_second '.result.structuredContent.ledger_term_id | tonumber')
    args=$(jq -n --argjson id "${term2_id:-0}" '{ledger_term_id:$id,effective_on:"2026-01-01"}')
    argent_fail terms_duplicate Argent.UpsertLedgerTerms "${args}" duplicate "${term2_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id,as_of:"2026-03-01"}')
    if [[ "${term_id}" =~ ^[0-9]+$ ]]; then
        argent_ok terms_get Argent.GetLedger "${args}" \
            "(.result.structuredContent.terms.ledger_term_id | tonumber) == ${term_id} and (.result.structuredContent.terms.credit_limit_cents | tonumber) == 200000" \
            "${bank_id}" || true
    else
        argent_miss terms_get
    fi
    argent_fail terms_not_found Argent.UpsertLedgerTerms '{"ledger_term_id":999999999}' not_found || true

    args=$(jq -n --argjson id "${bank_id:-0}" '{ledger_id:$id}')
    argent_fail contact_name Argent.UpsertContact "${args}" name_required "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" --arg name "pat${token}" '{ledger_id:$id,name:$name,role_a2005:9}')
    argent_fail contact_role Argent.UpsertContact "${args}" role "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" --arg name "pat${token}" '{ledger_id:$id,name:$name,address:1}')
    argent_fail contact_address Argent.UpsertContact "${args}" address "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" --arg name "pat${token}" --arg address "1 Main" \
        '{ledger_id:$id,name:$name,email:"pat@example.com",phone:"555",address:$address}')
    argent_ok contact_create Argent.UpsertContact "${args}" '.result.structuredContent.created == true' \
        "${bank_id}" || true
    contact_id=$(argent_grab contact_create '.result.structuredContent.contact_id | tonumber')
    args=$(jq -n --argjson id "${bank_id:-0}" --arg day "${day}" '{ledger_id:$id,as_of:$day}')
    if [[ "${contact_id}" =~ ^[0-9]+$ ]]; then
        argent_ok contact_get_role Argent.GetLedger "${args}" \
            "[.result.structuredContent.contacts[] | select((.contact_id|tonumber)==${contact_id}) | .role_a2005 | tonumber] | .[0] == 4" \
            "${bank_id}" || true
    else
        argent_miss contact_get_role
    fi
    args=$(jq -n --argjson id "${contact_id:-0}" '{contact_id:$id,role_a2005:1}')
    argent_ok contact_update Argent.UpsertContact "${args}" '.result.structuredContent.updated == true' \
        "${contact_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" --arg day "${day}" '{ledger_id:$id,as_of:$day}')
    if [[ "${contact_id}" =~ ^[0-9]+$ ]]; then
        argent_ok contact_get_role_1 Argent.GetLedger "${args}" \
            "[.result.structuredContent.contacts[] | select((.contact_id|tonumber)==${contact_id}) | .role_a2005 | tonumber] | .[0] == 1" \
            "${bank_id}" || true
    else
        argent_miss contact_get_role_1
    fi
    args=$(jq -n '{contact_id:999999999,name:"x"}')
    argent_fail contact_not_found Argent.UpsertContact "${args}" not_found || true

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

    argent_fail tags_type Argent.AddTags '{"entity_type":0,"entity_id":1,"tags":[{"name":"x"}]}' entity_type || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tags:[]}')
    argent_fail tags_empty Argent.AddTags "${args}" tags_required "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tags:[1]}')
    argent_fail tags_item Argent.AddTags "${args}" tags "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tags:[{}]}')
    argent_fail tags_name Argent.AddTags "${args}" name_required "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" --arg name "tag${token}" \
        '{entity_type:2,entity_id:$id,tags:[{name:$name}]}')
    argent_ok tags_create Argent.AddTags "${args}" \
        '(.result.structuredContent.links|length) == 1 and .result.structuredContent.links[0].created == true' \
        "${bank_id}" || true
    tag_id=$(argent_grab tags_create '.result.structuredContent.links[0].tag_id | tonumber')
    args=$(jq -n --argjson id "${bank_id:-0}" --argjson tag "${tag_id:-0}" \
        '{entity_type:2,entity_id:$id,tags:[{tag_id:$tag}]}')
    argent_ok tags_again Argent.AddTags "${args}" \
        '.result.structuredContent.links[0].created == false' \
        "${bank_id}" "${tag_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tags:[{tag_id:-1}]}')
    argent_fail tags_unknown Argent.AddTags "${args}" not_found "${bank_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "tag${token}" '{organization_id:$org,tag:$name}')
    if [[ "${bank_id}" =~ ^[0-9]+$ ]]; then
        argent_ok tags_list_hit Argent.ListLedgers "${args}" \
            "([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${bank_id})) != null" \
            "${org_id}" || true
    else
        argent_miss tags_list_hit
    fi
    args=$(jq -n --argjson id "${bank_id:-0}" --argjson tag "${tag_id:-0}" \
        '{entity_type:2,entity_id:$id,tag_id:$tag}')
    argent_ok tags_remove Argent.RemoveTags "${args}" \
        '(.result.structuredContent.removed[0].removed == true)' \
        "${bank_id}" "${tag_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "tag${token}" '{organization_id:$org,tag:$name}')
    if [[ "${bank_id}" =~ ^[0-9]+$ ]]; then
        argent_ok tags_list_miss Argent.ListLedgers "${args}" \
            "([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${bank_id})) == null" \
            "${org_id}" || true
    else
        argent_miss tags_list_miss
    fi
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tag_id:-1}')
    argent_ok tags_remove_absent Argent.RemoveTags "${args}" \
        '.result.structuredContent.removed[0].removed == false' \
        "${bank_id}" || true
    argent_fail tags_remove_required Argent.RemoveTags '{"entity_type":2,"entity_id":1}' tag_id_required || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tag_ids:["x"]}')
    argent_fail tags_ids_bad Argent.RemoveTags "${args}" tag_id "${bank_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" --arg name "txntag${token}" \
        '{entity_type:3,entity_id:$id,tags:[{name:$name}]}')
    argent_ok tags_txn Argent.AddTags "${args}" '.result.structuredContent.links[0].created == true' \
        "${txn_id}" || true
    args=$(jq -n --argjson id "${line_id:-0}" --arg name "linetag${token}" \
        '{entity_type:4,entity_id:$id,tags:[{name:$name}]}')
    argent_ok tags_line Argent.AddTags "${args}" '.result.structuredContent.links[0].created == true' \
        "${line_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id}')
    if [[ "${txn_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_get_tags Argent.GetTransaction "${args}" \
            "([.result.structuredContent.tags[]?.name] | index(\"txntag${token}\")) != null and ([.result.structuredContent.tags[]?.name] | index(\"linetag${token}\")) == null" \
            "${txn_id}" || true
    else
        argent_miss txn_get_tags
    fi

    args=$(jq -n --argjson id "${txn_id:-0}" '{entity_type:3,entity_id:$id,name:"note",att_type:9}')
    argent_fail att_type Argent.AddAttachment "${args}" att_type "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{entity_type:3,entity_id:$id,name:"note",file_text:"hello",byte_len:-1}')
    argent_fail att_byte Argent.AddAttachment "${args}" byte_len "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" --arg text "hello note" \
        '{entity_type:3,entity_id:$id,name:"Note",file_text:$text}')
    argent_ok att_note Argent.AddAttachment "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.rev_id | tonumber) == 1' \
        "${txn_id}" || true
    att_id=$(argent_grab att_note '.result.structuredContent.attachment_id | tonumber')
    args=$(jq -n --argjson entity "${txn_id:-0}" --argjson id "${att_id:-0}" --arg text "hello note 2" \
        '{attachment_id:$id,entity_type:3,entity_id:$entity,name:"Note",file_text:$text}')
    argent_ok att_note_rev Argent.AddAttachment "${args}" \
        '(.result.structuredContent.rev_id | tonumber) == 2 and (.result.structuredContent.attachment_id | tonumber) == '"${att_id:-0}" \
        "${txn_id}" "${att_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{entity_type:3,entity_id:$id,name:"File",file_data:"abc",file_name:"a.txt"}')
    argent_ok att_file Argent.AddAttachment "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.rev_id | tonumber) == 1' \
        "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id}')
    if [[ "${att_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_get_note Argent.GetTransaction "${args}" \
            "(( [.result.structuredContent.attachments[] | select(((.attachment_id|tonumber)==${att_id}) and ((.rev_id|tonumber)==1)) | .att_type_a2010 | tonumber] | .[0] ) == 1) and (( [.result.structuredContent.attachments[] | select(((.attachment_id|tonumber)==${att_id}) and ((.rev_id|tonumber)==1)) | .mime_type] | .[0] ) == \"text/plain\")" \
            "${txn_id}" || true
        argent_ok txn_get_file Argent.GetTransaction "${args}" \
            '([.result.structuredContent.attachments[] | select(.name == "File") | .att_type_a2010 | tonumber] | .[0]) == 5 and ([.result.structuredContent.attachments[] | has("file_data") or has("file_text")] | any | not)' \
            "${txn_id}" || true
    else
        argent_miss txn_get_note
        argent_miss txn_get_file
    fi

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

    echo "EXPECTED_TOOL_CASES=${ARGENT_CASE_N}" >> "${result_file}"
}
