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
#   DATA_JSON, LAYOUT_JSON, OUT_DIR, FORMAT,
#   CAT_DATA_JSON, CAT_EXPECTED_JSON, CAT_LIVE_JSON, CAT_FINDINGS_JSON,
#   CAT_OK, CAT_MT, CAT_MC, CAT_NULL, CAT_CHK, CAT_ROWS, CAT_EXIT_LABEL,
#   ONLY_TABLES, TABLES, JQ, ROW_GROUP_SIZE, WORK_DIR
#
# CHANGELOG
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
}

schematool_render() {
    schematool_shorten_title_db
    case "${RENDER_MODE}" in
        catalog)
            render_catalog_table
            ;;
        both)
            render_metadata_table
            echo "" >&2
            render_catalog_table
            ;;
        *)
            render_metadata_table
            ;;
    esac
}
