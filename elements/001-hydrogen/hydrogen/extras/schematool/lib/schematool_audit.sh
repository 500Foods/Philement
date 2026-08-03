#!/usr/bin/env bash
# SchemaTool audit orchestration — mode dispatch and audit runners
#
# This library is sourced by schematool.sh. It wraps the procedural
# audit blocks (early-exit dump/emit modes, full metadata audit,
# dry-disk discovery, and catalog audit) into callable functions.
#
# Expected globals (set by schematool.sh before invocation):
#   DUMP_DB, DUMP_DB_PATH, DUMP_CATALOG, DUMP_CATALOG_PATH,
#   EMIT_EXPECTED, EMIT_EXPECTED_PATH, DRY_DISK, FULL_AUDIT,
#   CATALOG_AUDIT, DB_JSON, CAT_LIVE_JSON, EXPECTED_JSON,
#   CAT_EXPECTED_JSON, CAT_DATA_JSON, CAT_FINDINGS_JSON,
#   DATA_JSON, LAYOUT_JSON, DISK_JSON, FINDINGS_JSON,
#   DB_DIR, LUA_DIR, MIGRATIONS, DESIGN, ENGINE, SCHEMA, DATABASE,
#   HOST, PORT, USER_NAME, PASSWORD_ENV, FROM_REF, TO_REF,
#   ONLY_TABLES, ONLY_FAILURES, INCLUDE_REVERSE, INCLUDE_DIAGRAM,
#   INCLUDE_OK, NORMALIZE, NO_SQL, OUT_DIR, SQL_OUT, MIG_OUT,
#   UTC_STAMP, DISPLAY_STAMP, VERSION,
#   DB_LABEL, SCHEMA_LABEL, AUDIT_EXIT, CATALOG_EXIT, RENDER_MODE
#
# Globals set by this library (consumed by schematool_render.sh):
#   AUDIT_EXIT, SUBTITLE, FOOTER, DATA_JSON,
#   CATALOG_EXIT, CAT_OK, CAT_MT, CAT_MC, CAT_NULL, CAT_CHK,
#   CAT_ROWS, CAT_EXIT_LABEL, RENDER_MODE, SUBTITLE, FOOTER
#
# CHANGELOG
# 1.0.0 - 2026-08-02 - Split from schematool.sh

# shellcheck disable=SC2154,SC2034 # globals shared with schematool.sh and render lib

set -euo pipefail

# --- Phase 3 only: dump DB (early-exit mode) ---
schematool_run_dump_db_only() {
    run_dump_db "${DB_JSON}"
    local dump_out="${DUMP_DB_PATH}"
    if [[ -z "${dump_out}" ]]; then
        if [[ -n "${OUT_DIR}" ]]; then
            dump_out="${OUT_DIR}/db_metadata.json"
        else
            dump_out=""
        fi
    fi
    local row_n
    row_n="$("${JQ}" 'length' "${DB_JSON}")"
    if [[ -n "${dump_out}" ]]; then
        local dump_parent
        dump_parent="$(dirname "${dump_out}")"
        mkdir -p "${dump_parent}"
        cp "${DB_JSON}" "${dump_out}"
        echo "DB metadata: ${dump_out} (${row_n} rows)" >&2
    else
        "${JQ}" '.' "${DB_JSON}"
        echo "DB metadata rows: ${row_n}" >&2
    fi
    if [[ "${EMIT_EXPECTED}" -eq 0 && "${DRY_DISK}" -eq 0 && "${DUMP_CATALOG}" -eq 0 ]]; then
        exit 0
    fi
}

# --- Phase 7 only: dump catalog (early-exit mode) ---
schematool_run_dump_catalog_only() {
    run_dump_catalog "${CAT_LIVE_JSON}"
    local cat_out="${DUMP_CATALOG_PATH}"
    if [[ -z "${cat_out}" ]]; then
        if [[ -n "${OUT_DIR}" ]]; then
            cat_out="${OUT_DIR}/catalog_live.json"
        else
            cat_out=""
        fi
    fi
    local tbl_n
    tbl_n="$("${JQ}" '.tables | length' "${CAT_LIVE_JSON}")"
    if [[ -n "${cat_out}" ]]; then
        local cat_parent
        cat_parent="$(dirname "${cat_out}")"
        mkdir -p "${cat_parent}"
        cp "${CAT_LIVE_JSON}" "${cat_out}"
        echo "Catalog: ${cat_out} (${tbl_n} tables)" >&2
    else
        "${JQ}" '.' "${CAT_LIVE_JSON}"
        echo "Catalog tables: ${tbl_n}" >&2
    fi
    exit 0
}

# --- Phase 2 only: emit expected (early-exit mode) ---
schematool_run_emit_expected_only() {
    run_expect "${EXPECTED_JSON}"
    local expect_out="${EMIT_EXPECTED_PATH}"
    if [[ -z "${expect_out}" ]]; then
        if [[ -n "${OUT_DIR}" ]]; then
            expect_out="${OUT_DIR}/expected_payloads.json"
        else
            expect_out=""
        fi
    fi
    if [[ -n "${expect_out}" ]]; then
        local expect_parent
        expect_parent="$(dirname "${expect_out}")"
        mkdir -p "${expect_parent}"
        cp "${EXPECTED_JSON}" "${expect_out}"
        echo "Expected payloads: ${expect_out}" >&2
    else
        "${JQ}" '.' "${EXPECTED_JSON}"
    fi
    if [[ "${DRY_DISK}" -eq 0 && "${DUMP_DB}" -eq 0 ]]; then
        exit 0
    fi
}

# --- Phase 4: full metadata audit (LOAD/APPLY/text-fidelity) ---
schematool_run_metadata_audit() {
    local disk_all_json="${WORK_DIR}/disk_all.json"
    run_discover "${DISK_JSON}"
    if [[ -n "${FROM_REF}" || -n "${TO_REF}" ]]; then
        # Full discovery without range (orphan = in DB, not on any disk file)
        if ! "${LUA}" "${LUA_DIR}/schematool_discover.lua" \
            "${MIGRATIONS}" "${DESIGN}" "" "" \
            > "${disk_all_json}"; then
            echo "Error: full migration discovery failed" >&2
            exit 1
        fi
    else
        cp "${DISK_JSON}" "${disk_all_json}"
    fi
    run_expect "${EXPECTED_JSON}"
    # Dump all migration metadata (no --from/--to) so orphans outside range are visible
    local saved_from="${FROM_REF}"
    local saved_to="${TO_REF}"
    FROM_REF=""
    TO_REF=""
    run_dump_db "${DB_JSON}"
    FROM_REF="${saved_from}"
    TO_REF="${saved_to}"

    local COMPARE_ARGS=(
        --disk "${DISK_JSON}"
        --disk-all "${disk_all_json}"
        --expected "${EXPECTED_JSON}"
        --db "${DB_JSON}"
        --normalize "${NORMALIZE}"
        --checklist-out "${DATA_JSON}"
        --findings-out "${FINDINGS_JSON}"
    )
    if [[ "${ONLY_FAILURES}" -eq 1 ]]; then
        COMPARE_ARGS+=(--only-failures)
    fi
    if [[ "${INCLUDE_REVERSE}" -eq 1 ]]; then
        COMPARE_ARGS+=(--include-reverse)
    fi
    if [[ "${INCLUDE_DIAGRAM}" -eq 1 ]]; then
        COMPARE_ARGS+=(--include-diagram)
    fi

    set +e
    "${LUA}" "${LUA_DIR}/schematool_compare.lua" "${COMPARE_ARGS[@]}"
    local cmp_rc=$?
    set -e
    if [[ "${cmp_rc}" -ne 0 ]]; then
        echo "Error: compare failed" >&2
        exit 1
    fi

    AUDIT_EXIT="$("${JQ}" -r '.exit_code // 0' "${FINDINGS_JSON}")"
    MIN_REF="$("${JQ}" 'min_by(.ref).ref' "${DATA_JSON}")"
    MAX_REF="$("${JQ}" 'max_by(.ref).ref' "${DATA_JSON}")"
    ROW_COUNT="$("${JQ}" 'length' "${DATA_JSON}")"
    CNT_OK="$("${JQ}" -r '.counts.ok // 0' "${FINDINGS_JSON}")"
    CNT_DRIFT="$("${JQ}" -r '.counts.drift // 0' "${FINDINGS_JSON}")"
    CNT_MLOAD="$("${JQ}" -r '.counts.missing_load // 0' "${FINDINGS_JSON}")"
    CNT_MAPPLY="$("${JQ}" -r '.counts.missing_apply // 0' "${FINDINGS_JSON}")"
    CNT_ORPHAN="$("${JQ}" -r '.counts.orphans // 0' "${FINDINGS_JSON}")"
    DB_MAX_APPLY="$("${JQ}" '[.[] | select(.query_type==1003) | .query_ref] | if length>0 then max else 0 end' "${DB_JSON}")"
    DISK_MAX="$("${JQ}" 'max_by(.ref).ref' "${DISK_JSON}")"

    case "${AUDIT_EXIT}" in
        0) EXIT_LABEL="exit 0 clean" ;;
        2) EXIT_LABEL="exit 2 drift/missing" ;;
        3) EXIT_LABEL="exit 3 anomalies" ;;
        *) EXIT_LABEL="exit ${AUDIT_EXIT}" ;;
    esac

    SUBTITLE="{CYAN}Audit{WHITE} refs ${MIN_REF}-${MAX_REF} ({BOLD}${ROW_COUNT}{RESET}{CYAN} rows) · disk AVAIL ${DISK_MAX} · DB APPLY ${DB_MAX_APPLY}{RESET}"
    FOOTER="{CYAN}${DISPLAY_STAMP}{RESET} {RED}———{RESET} ok=${CNT_OK} drift=${CNT_DRIFT} missL=${CNT_MLOAD} missA=${CNT_MAPPLY} orphan=${CNT_ORPHAN} · ${EXIT_LABEL} · norm=${NORMALIZE}"

    # Remediation artifacts
    if [[ "${NO_SQL}" -eq 0 ]]; then
        if [[ -z "${SQL_OUT}" ]]; then
            if [[ -n "${OUT_DIR}" ]]; then
                SQL_OUT="${OUT_DIR}/schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.sql"
            else
                SQL_OUT="./schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.sql"
            fi
        fi
    fi

    MIG_PATH="${MIG_OUT}"
    if [[ -z "${MIG_PATH}" && -n "${OUT_DIR}" ]]; then
        local orphan_n
        orphan_n="$("${JQ}" '.counts.orphans // 0' "${FINDINGS_JSON}")"
        if [[ "${orphan_n}" -gt 0 ]]; then
            MIG_PATH="${OUT_DIR}/schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.mig"
        fi
    fi

    if [[ "${NO_SQL}" -eq 0 || -n "${MIG_PATH}" ]]; then
        local REM_ARGS=(
            --findings "${FINDINGS_JSON}"
            --design "${DESIGN}"
            --engine "${ENGINE}"
            --schema "${SCHEMA}"
            --database "${DATABASE}"
            --migrations "${MIGRATIONS}"
            --normalize "${NORMALIZE}"
            --version "${VERSION}"
            --stamp "${UTC_STAMP}"
        )
        if [[ "${NO_SQL}" -eq 0 ]]; then
            local sql_parent
            sql_parent="$(dirname "${SQL_OUT}")"
            mkdir -p "${sql_parent}"
            REM_ARGS+=(--sql-out "${SQL_OUT}")
        else
            # remediate requires --sql-out; write to work dir and discard
            REM_ARGS+=(--sql-out "${WORK_DIR}/discard.sql")
        fi
        if [[ -n "${MIG_PATH}" ]]; then
            local mig_parent
            mig_parent="$(dirname "${MIG_PATH}")"
            mkdir -p "${mig_parent}"
            REM_ARGS+=(--mig-out "${MIG_PATH}")
        fi
        if [[ "${INCLUDE_OK}" -eq 1 ]]; then
            REM_ARGS+=(--include-ok-comments)
        fi
        set +e
        "${LUA}" "${LUA_DIR}/schematool_remediate.lua" "${REM_ARGS[@]}"
        local rem_rc=$?
        set -e
        if [[ "${rem_rc}" -ne 0 ]]; then
            echo "Error: remediation generation failed" >&2
            exit 1
        fi
    fi

    if [[ -n "${OUT_DIR}" ]]; then
        cp "${DATA_JSON}" "${OUT_DIR}/checklist_data.json"
        cp "${FINDINGS_JSON}" "${OUT_DIR}/findings.json"
        cp "${DB_JSON}" "${OUT_DIR}/db_metadata.json"
        cp "${EXPECTED_JSON}" "${OUT_DIR}/expected_payloads.json"
        cp "${DISK_JSON}" "${OUT_DIR}/disk.json"
    fi
}

# --- Phase 1: dry-disk discovery only (no DB) ---
schematool_run_dry_disk() {
    run_discover "${DATA_JSON}"
    MIN_REF="$("${JQ}" 'min_by(.ref).ref' "${DATA_JSON}")"
    MAX_REF="$("${JQ}" 'max_by(.ref).ref' "${DATA_JSON}")"
    ROW_COUNT="$("${JQ}" 'length' "${DATA_JSON}")"
    SUBTITLE="{CYAN}Disk discovery{WHITE} refs ${MIN_REF}–${MAX_REF} ({BOLD}${ROW_COUNT}{RESET}{CYAN} files) · no DB audit{RESET}"
    FOOTER="{CYAN}Generated{WHITE} ${DISPLAY_STAMP}{RESET} {RED}———{RESET} {YELLOW}dry-disk{RESET}"

    if [[ "${NO_SQL}" -eq 0 ]]; then
        if [[ -z "${SQL_OUT}" ]]; then
            if [[ -n "${OUT_DIR}" ]]; then
                SQL_OUT="${OUT_DIR}/schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.sql"
            else
                SQL_OUT="./schematool_${DESIGN}_${ENGINE}_${UTC_STAMP}.sql"
            fi
        fi
        local sql_parent
        sql_parent="$(dirname "${SQL_OUT}")"
        mkdir -p "${sql_parent}"
        {
            echo "-- ============================================================================="
            echo "-- SchemaTool remediation (NOT EXECUTED)"
            echo "-- schematool ${VERSION} · dry-disk stub"
            echo "-- design=${DESIGN} engine=${ENGINE} schema=${SCHEMA_LABEL} database=${DB_LABEL}"
            echo "-- migrations=${MIGRATIONS}"
            echo "-- refs=${MIN_REF}-${MAX_REF} count=${ROW_COUNT}"
            echo "-- normalize=${NORMALIZE}"
            echo "-- Generated: ${UTC_STAMP}"
            echo "-- Rule: Uncomment deliberately. Prefer Hydrogen LOAD/APPLY when possible."
            echo "-- Full audit requires DB connection (not --dry-disk)."
            echo "-- ============================================================================="
            echo "--"
        } > "${SQL_OUT}"
        echo "SQL: ${SQL_OUT} (all statements commented)" >&2
    fi

    if [[ -n "${OUT_DIR}" ]]; then
        cp "${DATA_JSON}" "${OUT_DIR}/checklist_data.json"
    fi
}

# --- Phase 7: live catalog audit (object shape vs folded DDL) ---
schematool_run_catalog_audit() {
    run_catalog_audit
    CATALOG_EXIT="$("${JQ}" -r '.exit_code // 0' "${CAT_FINDINGS_JSON}")"
    CAT_OK="$("${JQ}" -r '.counts.ok // 0' "${CAT_FINDINGS_JSON}")"
    CAT_MT="$("${JQ}" -r '.counts.missing_table // 0' "${CAT_FINDINGS_JSON}")"
    CAT_MC="$("${JQ}" -r '.counts.missing_column // 0' "${CAT_FINDINGS_JSON}")"
    CAT_NULL="$("${JQ}" -r '.counts.nullability // 0' "${CAT_FINDINGS_JSON}")"
    CAT_CHK="$("${JQ}" -r '.counts.checked // 0' "${CAT_FINDINGS_JSON}")"
    CAT_ROWS="$("${JQ}" 'length' "${CAT_DATA_JSON}")"

    case "${CATALOG_EXIT}" in
        0) CAT_EXIT_LABEL="exit 0 clean" ;;
        2) CAT_EXIT_LABEL="exit 2 shape drift" ;;
        *) CAT_EXIT_LABEL="exit ${CATALOG_EXIT}" ;;
    esac

    if [[ -n "${OUT_DIR}" ]]; then
        cp "${CAT_EXPECTED_JSON}" "${OUT_DIR}/catalog_expected.json"
        cp "${CAT_LIVE_JSON}" "${OUT_DIR}/catalog_live.json"
        cp "${CAT_DATA_JSON}" "${OUT_DIR}/catalog_checklist.json"
        cp "${CAT_FINDINGS_JSON}" "${OUT_DIR}/catalog_findings.json"
        if [[ -f "${DB_JSON}" ]]; then
            cp "${DB_JSON}" "${OUT_DIR}/db_metadata.json"
        fi
    fi

    # Catalog-only render when metadata was skipped
    if [[ "${FULL_AUDIT}" -eq 0 ]]; then
        RENDER_MODE="catalog"
        DATA_JSON="${CAT_DATA_JSON}"
        SUBTITLE="{CYAN}Catalog{WHITE} hybrid-C · ${CAT_CHK} checks · tables filter=${ONLY_TABLES:-all}{RESET}"
        FOOTER="{CYAN}${DISPLAY_STAMP}{RESET} {RED}———{RESET} ok=${CAT_OK} missT=${CAT_MT} missC=${CAT_MC} null=${CAT_NULL} · ${CAT_EXIT_LABEL}"
    else
        # Print catalog table after metadata
        RENDER_MODE="both"
    fi
}
