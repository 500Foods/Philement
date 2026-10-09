#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, day, etc.) are set by argent_mcp_exercise

# Argent ledger-terms and contact exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_ok, argent_fail, argent_grab) and
# the exercise-local variables: token, day, bank_id, term_id, term2_id,
# contact_id

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_terms_contacts() {
    local args

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
}
