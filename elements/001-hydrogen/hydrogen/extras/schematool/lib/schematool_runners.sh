#!/usr/bin/env bash
# SchemaTool database/Lua runner wrappers
#
# This library is sourced by schematool.sh. It provides thin wrappers that
# invoke the engine-specific adapter scripts (db/) and Lua extractors (lua/),
# capturing their JSON output into work files and validating the result shape.
#
# Expected globals (set by schematool.sh before invocation):
#   ENGINE, DB_DIR, LUA_DIR, MIGRATIONS, SCHEMA, FROM_REF, TO_REF,
#   HOST, PORT, USER_NAME, DATABASE, PASSWORD_ENV, ONLY_TABLES,
#   JQ, LUA, WORK_DIR, DB_JSON, EXPECTED_JSON, CAT_LIVE_JSON,
#   CAT_EXPECTED_JSON, CAT_DATA_JSON, CAT_FINDINGS_JSON, ONLY_FAILURES
#
# CHANGELOG
# 1.1.0 - 2026-08-23 - Catalog fold/compare/probe return 1 (caller may degrade)
# 1.0.0 - 2026-08-02 - Split from schematool.sh

# shellcheck disable=SC2154,SC2034 # globals shared with schematool.sh and other lib modules

set -euo pipefail

run_dump_db() {
    local out_file="$1"
    local adapter
    case "${ENGINE}" in
        postgresql) adapter="${DB_DIR}/query_pg.sh" ;;
        mysql) adapter="${DB_DIR}/query_mysql.sh" ;;
        sqlite) adapter="${DB_DIR}/query_sqlite.sh" ;;
        db2) adapter="${DB_DIR}/query_db2.sh" ;;
        *)
            echo "Error: no dump adapter for engine ${ENGINE}" >&2
            exit 1
            ;;
    esac
    if [[ ! -x "${adapter}" && -f "${adapter}" ]]; then
        chmod +x "${adapter}" || true
    fi
    if [[ ! -f "${adapter}" ]]; then
        echo "Error: adapter not found: ${adapter}" >&2
        exit 1
    fi

    if [[ "${ENGINE}" == "sqlite" ]]; then
        if [[ -z "${DATABASE}" ]]; then
            echo "Error: --database (sqlite file path) is required" >&2
            exit 1
        fi
    elif [[ "${ENGINE}" == "db2" ]]; then
        if [[ -z "${DATABASE}" || -z "${USER_NAME}" ]]; then
            echo "Error: --database and --user required for db2 (or HYDROTST_DB_*)" >&2
            exit 1
        fi
        if [[ -z "${SCHEMA}" ]]; then
            echo "Error: --schema is required for db2 (e.g. DEMO)" >&2
            exit 1
        fi
    else
        if [[ -z "${HOST}" || -z "${USER_NAME}" || -z "${DATABASE}" ]]; then
            echo "Error: --host/--user/--database required (or engine env fallback)" >&2
            exit 1
        fi
        if [[ -z "${SCHEMA}" ]]; then
            echo "Error: --schema is required (e.g. demo)" >&2
            exit 1
        fi
    fi

    local dump_args=(
        --host "${HOST}"
        --port "${PORT}"
        --user "${USER_NAME}"
        --database "${DATABASE}"
        --schema "${SCHEMA}"
        --password-env "${PASSWORD_ENV}"
    )
    if [[ -n "${FROM_REF}" ]]; then
        dump_args+=(--from "${FROM_REF}")
    fi
    if [[ -n "${TO_REF}" ]]; then
        dump_args+=(--to "${TO_REF}")
    fi

    set +e
    "${adapter}" "${dump_args[@]}" > "${out_file}"
    local dump_rc=$?
    set -e
    if [[ "${dump_rc}" -ne 0 ]]; then
        echo "Error: database metadata dump failed" >&2
        exit 1
    fi
    if ! "${JQ}" -e 'type == "array"' "${out_file}" >/dev/null 2>&1; then
        echo "Error: dump did not produce a JSON array" >&2
        exit 1
    fi
}

run_expect() {
    local out_file="$1"
    if [[ -z "${SCHEMA}" && "${ENGINE}" != "sqlite" ]]; then
        echo "Error: --schema is required for expected extract (empty only for sqlite)" >&2
        exit 1
    fi
    local schema_arg="${SCHEMA}"
    set +e
    (
        cd "${MIGRATIONS}" || exit 1
        if [[ -n "${FROM_REF}" || -n "${TO_REF}" ]]; then
            "${LUA}" "${LUA_DIR}/schematool_expect.lua" \
                "${MIGRATIONS}" "${ENGINE}" "${DESIGN}" "${schema_arg}" \
                --all "${FROM_REF}" "${TO_REF}"
        else
            "${LUA}" "${LUA_DIR}/schematool_expect.lua" \
                "${MIGRATIONS}" "${ENGINE}" "${DESIGN}" "${schema_arg}" \
                --all
        fi
    ) > "${out_file}"
    local expect_rc=$?
    set -e
    if [[ "${expect_rc}" -ne 0 ]]; then
        echo "Error: expected payload extraction failed" >&2
        exit 1
    fi
    if ! "${JQ}" -e 'type == "array" or type == "object"' "${out_file}" >/dev/null 2>&1; then
        echo "Error: expected extraction did not produce JSON" >&2
        exit 1
    fi
}

run_discover() {
    local out_file="$1"
    if ! "${LUA}" "${LUA_DIR}/schematool_discover.lua" \
        "${MIGRATIONS}" "${DESIGN}" "${FROM_REF}" "${TO_REF}" \
        > "${out_file}"; then
        echo "Error: migration discovery failed" >&2
        exit 1
    fi
    if ! "${JQ}" -e 'type == "array"' "${out_file}" >/dev/null 2>&1; then
        echo "Error: discovery did not produce a JSON array" >&2
        exit 1
    fi
    local row_count
    row_count="$("${JQ}" 'length' "${out_file}")"
    if [[ "${row_count}" -eq 0 ]]; then
        echo "Error: no migrations matched ${DESIGN}_NNNN.lua in ${MIGRATIONS}" >&2
        exit 1
    fi
}

run_dump_catalog() {
    local out_file="$1"
    local adapter
    case "${ENGINE}" in
        postgresql) adapter="${DB_DIR}/catalog_pg.sh" ;;
        mysql) adapter="${DB_DIR}/catalog_mysql.sh" ;;
        sqlite) adapter="${DB_DIR}/catalog_sqlite.sh" ;;
        db2) adapter="${DB_DIR}/catalog_db2.sh" ;;
        *)
            echo "Error: no catalog adapter for engine ${ENGINE}" >&2
            return 1
            ;;
    esac
    if [[ ! -x "${adapter}" && -f "${adapter}" ]]; then
        chmod +x "${adapter}" || true
    fi
    if [[ ! -f "${adapter}" ]]; then
        echo "Error: catalog adapter not found: ${adapter}" >&2
        return 1
    fi

    local cat_args=(
        --host "${HOST}"
        --port "${PORT}"
        --user "${USER_NAME}"
        --database "${DATABASE}"
        --schema "${SCHEMA}"
        --password-env "${PASSWORD_ENV}"
    )
    if [[ -n "${ONLY_TABLES}" ]]; then
        cat_args+=(--tables "${ONLY_TABLES}")
    fi

    set +e
    "${adapter}" "${cat_args[@]}" > "${out_file}"
    local cat_rc=$?
    set -e
    if [[ "${cat_rc}" -ne 0 ]]; then
        echo "Error: catalog probe failed" >&2
        return 1
    fi
    if ! "${JQ}" -e 'type == "object" and has("tables")' "${out_file}" >/dev/null 2>&1; then
        echo "Error: catalog probe did not produce catalog JSON object" >&2
        return 1
    fi
}

run_catalog_audit() {
    if [[ ! -f "${DB_JSON}" ]] || ! "${JQ}" -e 'type == "array"' "${DB_JSON}" >/dev/null 2>&1; then
        local saved_from="${FROM_REF}"
        local saved_to="${TO_REF}"
        FROM_REF=""
        TO_REF=""
        run_dump_db "${DB_JSON}"
        FROM_REF="${saved_from}"
        TO_REF="${saved_to}"
    fi

    local fold_args=(
        --db "${DB_JSON}"
        --schema "${SCHEMA}"
        --out "${CAT_EXPECTED_JSON}"
    )
    if [[ -n "${ONLY_TABLES}" ]]; then
        fold_args+=(--only-tables "${ONLY_TABLES}")
    fi
    set +e
    "${LUA}" "${LUA_DIR}/schematool_catalog_fold.lua" "${fold_args[@]}"
    local fold_rc=$?
    set -e
    if [[ "${fold_rc}" -ne 0 ]]; then
        echo "Error: catalog expected-shape fold failed" >&2
        return 1
    fi

    local probe_tables="${ONLY_TABLES}"
    if [[ -z "${probe_tables}" ]]; then
        probe_tables="$("${JQ}" -r '[.tables[].table] | join(",")' "${CAT_EXPECTED_JSON}")"
    fi
    local saved_only="${ONLY_TABLES}"
    ONLY_TABLES="${probe_tables}"
    set +e
    run_dump_catalog "${CAT_LIVE_JSON}"
    local probe_rc=$?
    set -e
    ONLY_TABLES="${saved_only}"
    if [[ "${probe_rc}" -ne 0 ]]; then
        return 1
    fi

    local cmp_args=(
        --expected "${CAT_EXPECTED_JSON}"
        --live "${CAT_LIVE_JSON}"
        --checklist-out "${CAT_DATA_JSON}"
        --findings-out "${CAT_FINDINGS_JSON}"
    )
    if [[ "${ONLY_FAILURES}" -eq 1 ]]; then
        cmp_args+=(--only-failures)
    fi
    set +e
    "${LUA}" "${LUA_DIR}/schematool_catalog_compare.lua" "${cmp_args[@]}"
    local ccmp_rc=$?
    set -e
    if [[ "${ccmp_rc}" -ne 0 ]]; then
        echo "Error: catalog compare failed" >&2
        return 1
    fi
}
