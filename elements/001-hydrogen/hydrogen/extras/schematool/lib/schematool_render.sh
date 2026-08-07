#!/usr/bin/env bash
# SchemaTool rendering — tables output and checklist rendering
#
# This library is sourced by schematool.sh. It renders the metadata
# and/or catalog checklists using the Hydrogen `tables` binary, and
# dispatches rendering based on the current audit mode.
#
# Expected globals (set by schematool.sh / schematool_audit.sh):
#   AUDIT_EXIT, CATALOG_EXIT, FULL_AUDIT, RENDER_MODE,
#   DESIGN, ENGINE, SCHEMA_LABEL, DB_LABEL, TITLE_DB,
#   SUBTITLE, FOOTER, DISPLAY_STAMP,
#   DATA_JSON, LAYOUT_JSON, OUT_DIR, FORMAT, FINDINGS_JSON,
#   CAT_DATA_JSON, CAT_EXPECTED_JSON, CAT_LIVE_JSON, CAT_FINDINGS_JSON,
#   CAT_OK, CAT_MT, CAT_MC, CAT_NULL, CAT_CHK, CAT_ROWS, CAT_EXIT_LABEL,
#   ONLY_TABLES, TABLES, JQ, LUA, LUA_DIR, ROW_GROUP_SIZE, WORK_DIR,
#   SCHEMA, NO_DETAIL, DETAIL_MAX_LINES
#
# CHANGELOG
# 1.2.0 - 2026-08-06 - Post-table finding details (diff + commented remediation)
# 1.1.0 - 2026-08-02 - Row grouping: horizontal separators every ROW_GROUP_SIZE rows
# 1.0.0 - 2026-08-02 - Split from schematool.sh

# shellcheck disable=SC2154,SC2034 # globals shared with schematool.sh and audit lib

set -euo pipefail

schematool_group_data() {
    local input_json="$1"
    local output_json="$2"
    # shellcheck disable=SC2016 # jq filter — $size passed via --argjson, not shell
    "${JQ}" --argjson size "${ROW_GROUP_SIZE:-20}" \
        'to_entries | map(.value.group = (.key / $size | floor)) | map(.value)' \
        "${input_json}" > "${output_json}"
}

schematool_shorten_title_db() {
    TITLE_DB="${DB_LABEL}"
    if [[ "${ENGINE}" == "sqlite" && ${#TITLE_DB} -gt 48 ]]; then
        TITLE_DB="…/${TITLE_DB##*/}"
    fi
}

render_metadata_table() {
    local theme="Red"
    if [[ "${AUDIT_EXIT}" -eq 0 && "${FULL_AUDIT}" -eq 1 ]]; then
        theme="Blue"
    fi
    cat > "${LAYOUT_JSON}" <<EOF
{
    "title": "{BOLD}{WHITE}SchemaTool{RESET} — ${DESIGN} / ${ENGINE} / ${SCHEMA_LABEL} @ ${TITLE_DB}",
    "subtitle": "${SUBTITLE}",
    "footer": "${FOOTER}",
    "footer_position": "right",
    "theme": "${theme}",
    "columns": [
        {"header": "Group", "key": "group", "datatype": "int", "visible": false, "break": true},
        {"header": "Ref", "key": "ref", "datatype": "int", "justification": "right", "summary": "count"},
        {"header": "File", "key": "file", "datatype": "text", "justification": "left", "summary": "count"},
        {"header": "LOAD", "key": "load", "datatype": "text", "justification": "center"},
        {"header": "L.match", "key": "load_match", "datatype": "text", "justification": "center"},
        {"header": "APPLY", "key": "apply", "datatype": "text", "justification": "center"},
        {"header": "A.match", "key": "apply_match", "datatype": "text", "justification": "center"},
        {"header": "Notes", "key": "notes", "datatype": "text", "justification": "left"}
    ]
}
EOF
    if [[ -n "${OUT_DIR}" ]]; then
        cp "${LAYOUT_JSON}" "${OUT_DIR}/checklist_layout.json"
    fi
    case "${FORMAT}" in
        tables|both)
            if [[ "${ROW_GROUP_SIZE:-20}" -gt 0 ]]; then
                local grouped_json="${WORK_DIR}/checklist_grouped.json"
                schematool_group_data "${DATA_JSON}" "${grouped_json}"
                "${TABLES}" "${LAYOUT_JSON}" "${grouped_json}"
            else
                "${TABLES}" "${LAYOUT_JSON}" "${DATA_JSON}"
            fi
            ;;
        json)
            "${JQ}" '.' "${DATA_JSON}"
            ;;
        *)
            echo "Error: unsupported format ${FORMAT}" >&2
            exit 1
            ;;
    esac
    if [[ "${FORMAT}" == "both" && -n "${OUT_DIR}" ]]; then
        echo "JSON: ${OUT_DIR}/checklist_data.json" >&2
    elif [[ "${FORMAT}" == "both" ]]; then
        echo "--- checklist JSON ---"
        "${JQ}" '.' "${DATA_JSON}"
    fi

    schematool_render_metadata_detail
}

render_catalog_table() {
    local theme="Red"
    if [[ "${CATALOG_EXIT}" -eq 0 ]]; then
        theme="Blue"
    fi
    local cat_layout="${WORK_DIR}/catalog_layout.json"
    local only_label="${ONLY_TABLES:-all}"
    cat > "${cat_layout}" <<EOF
{
    "title": "{BOLD}{WHITE}SchemaTool Catalog{RESET} — ${DESIGN} / ${ENGINE} / ${SCHEMA_LABEL} @ ${TITLE_DB}",
    "subtitle": "{CYAN}Live object shape{WHITE} · hybrid-C · filter=${only_label} · ${CAT_CHK} checks ({BOLD}${CAT_ROWS}{RESET}{CYAN} rows){RESET}",
    "footer": "{CYAN}${DISPLAY_STAMP}{RESET} {RED}———{RESET} ok=${CAT_OK} missT=${CAT_MT} missC=${CAT_MC} null=${CAT_NULL} · ${CAT_EXIT_LABEL}",
    "footer_position": "right",
    "theme": "${theme}",
    "columns": [
        {"header": "Group", "key": "group", "datatype": "int", "visible": false, "break": true},
        {"header": "Object", "key": "object", "datatype": "text", "justification": "left", "summary": "count"},
        {"header": "Column", "key": "column", "datatype": "text", "justification": "left"},
        {"header": "Check", "key": "check", "datatype": "text", "justification": "center"},
        {"header": "OK", "key": "status", "datatype": "text", "justification": "center"},
        {"header": "Expected", "key": "expected", "datatype": "text", "justification": "center"},
        {"header": "Live", "key": "live", "datatype": "text", "justification": "center"},
        {"header": "Notes", "key": "notes", "datatype": "text", "justification": "left"}
    ]
}
EOF
    if [[ -n "${OUT_DIR}" ]]; then
        cp "${cat_layout}" "${OUT_DIR}/catalog_layout.json"
    fi
    case "${FORMAT}" in
        tables|both)
            if [[ "${ROW_GROUP_SIZE:-20}" -gt 0 ]]; then
                local cat_grouped="${WORK_DIR}/catalog_grouped.json"
                schematool_group_data "${CAT_DATA_JSON}" "${cat_grouped}"
                "${TABLES}" "${cat_layout}" "${cat_grouped}"
            else
                "${TABLES}" "${cat_layout}" "${CAT_DATA_JSON}"
            fi
            ;;
        json)
            "${JQ}" '.' "${CAT_DATA_JSON}"
            ;;
        *)
            echo "Error: unsupported format ${FORMAT}" >&2
            exit 1
            ;;
    esac
    if [[ "${FORMAT}" == "both" && -n "${OUT_DIR}" ]]; then
        echo "JSON: ${OUT_DIR}/catalog_checklist.json" >&2
    elif [[ "${FORMAT}" == "both" ]]; then
        echo "--- catalog JSON ---"
        "${JQ}" '.' "${CAT_DATA_JSON}"
    fi

    schematool_render_catalog_detail
}

# Post-table narrative: unified field diffs + commented UPDATE/guidance
schematool_render_metadata_detail() {
    if [[ "${NO_DETAIL:-0}" -eq 1 ]]; then
        return 0
    fi
    if [[ "${FORMAT}" == "json" ]]; then
        return 0
    fi
    if [[ -z "${FINDINGS_JSON:-}" || ! -f "${FINDINGS_JSON}" ]]; then
        return 0
    fi
    if [[ "${AUDIT_EXIT:-0}" -eq 0 ]]; then
        return 0
    fi
    local detail_script="${LUA_DIR}/schematool_detail.lua"
    if [[ ! -f "${detail_script}" ]]; then
        return 0
    fi
    local max_lines="${DETAIL_MAX_LINES:-80}"
    local detail_out="${WORK_DIR}/finding_detail.txt"
    set +e
    "${LUA}" "${detail_script}" \
        --findings "${FINDINGS_JSON}" \
        --engine "${ENGINE}" \
        --schema "${SCHEMA:-}" \
        --max-lines "${max_lines}" \
        >"${detail_out}" 2>/dev/null
    local drc=$?
    set -e
    if [[ "${drc}" -eq 0 && -s "${detail_out}" ]]; then
        cat "${detail_out}"
        if [[ -n "${OUT_DIR}" ]]; then
            cp "${detail_out}" "${OUT_DIR}/finding_detail.txt"
            echo "Detail: ${OUT_DIR}/finding_detail.txt" >&2
        fi
    fi
}

schematool_render_catalog_detail() {
    if [[ "${NO_DETAIL:-0}" -eq 1 ]]; then
        return 0
    fi
    if [[ "${FORMAT}" == "json" ]]; then
        return 0
    fi
    if [[ "${CATALOG_EXIT:-0}" -eq 0 ]]; then
        return 0
    fi
    # Checklist rows hold per-check expected/live; findings JSON is counts-only
    if [[ -z "${CAT_DATA_JSON:-}" || ! -f "${CAT_DATA_JSON}" ]]; then
        return 0
    fi
    local detail_script="${LUA_DIR}/schematool_detail.lua"
    if [[ ! -f "${detail_script}" ]]; then
        return 0
    fi
    local detail_out="${WORK_DIR}/catalog_finding_detail.txt"
    set +e
    "${LUA}" "${detail_script}" \
        --catalog-findings "${CAT_DATA_JSON}" \
        --max-lines "${DETAIL_MAX_LINES:-80}" \
        >"${detail_out}" 2>/dev/null
    local drc=$?
    set -e
    if [[ "${drc}" -eq 0 && -s "${detail_out}" ]]; then
        cat "${detail_out}"
        if [[ -n "${OUT_DIR}" ]]; then
            cp "${detail_out}" "${OUT_DIR}/catalog_finding_detail.txt"
            echo "Detail: ${OUT_DIR}/catalog_finding_detail.txt" >&2
        fi
    fi
}

schematool_render() {
    schematool_shorten_title_db
    case "${RENDER_MODE}" in
        catalog)
            render_catalog_table
            ;;
        both)
            render_metadata_table
            echo ""
            render_catalog_table
            ;;
        *)
            render_metadata_table
            ;;
    esac
}
