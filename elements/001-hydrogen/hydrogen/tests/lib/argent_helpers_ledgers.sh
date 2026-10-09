#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, day, org_id, etc.) are set by argent_mcp_exercise
# shellcheck disable=SC2034 # tax_id, line_id etc. may be unused in some paths but needed for completeness
# shellcheck disable=SC2312 # diagnostic command substitutions intentionally swallow exit codes

# Argent ledger exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_ok, argent_fail, argent_grab) and
# the exercise-local variables set by argent_mcp_exercise:
# token, day, org_id, org2_id, bank_id, card_id, exp_id, tax_id, usd_id,
# parent_id, child_id, org2_ledger, long_key

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_ledgers() {
    local args

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
}
