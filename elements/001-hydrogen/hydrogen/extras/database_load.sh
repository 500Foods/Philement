#!/usr/bin/env bash
# shellcheck disable=SC2310 # Each engine reports its own status and the wave continues.
# database_load.sh — apply embedded payload migrations to the build schemas.
#
# Wave 1 starts the eight test schemas (tests 32-39) together. Wave 2 starts
# the eight demo schemas (test 40) after wave 1 has stopped. Each engine gets
# one Hydrogen process and the JSON config it already uses. TestMigration is
# forced off in a temporary copy so the REVERSE phase does not undo the load.
# The process is stopped once the lead queue logs that migration has finished.
#
# The binary's embedded payload is what gets applied. Rebuild that payload
# after editing Helium migrations, then rebuild Hydrogen, before loading.
#
# CHANGELOG
# 1.0.0 - 2026-10-07 - Load test schemas in parallel, then demo schemas
# TEST_VERSION: 1.0.0

set -euo pipefail
set +H

GROUP_FILTER=""
ENGINE_FILTER=""
DRY_RUN=0
TIMEOUT=1800
FAILS=0
HYDROGEN_BIN_OVERRIDE="${HYDROGEN_BIN:-}"
HYDROGEN_BIN=""
LOG_DIR=""
PIDS=()

TARGETS=(
    "test|32|postgresql|PostgreSQL|hydrogen_test_32_postgres.json"
    "test|33|mysql|MySQL|hydrogen_test_33_mysql.json"
    "test|34|sqlite|SQLite|hydrogen_test_34_sqlite.json"
    "test|35|db2|DB2|hydrogen_test_35_db2.json"
    "test|36|mariadb|MariaDB|hydrogen_test_36_mariadb.json"
    "test|37|firebird|Firebird|hydrogen_test_37_firebird.json"
    "test|38|yugabytedb|YugabyteDB|hydrogen_test_38_yugabytedb.json"
    "test|39|mssql|MSSQL|hydrogen_test_39_mssql.json"
    "demo|40|postgresql|PostgreSQL|hydrogen_test_40_postgres.json"
    "demo|40|mysql|MySQL|hydrogen_test_40_mysql.json"
    "demo|40|sqlite|SQLite|hydrogen_test_40_sqlite.json"
    "demo|40|db2|DB2|hydrogen_test_40_db2.json"
    "demo|40|mariadb|MariaDB|hydrogen_test_40_mariadb.json"
    "demo|40|firebird|Firebird|hydrogen_test_40_firebird.json"
    "demo|40|yugabytedb|YugabyteDB|hydrogen_test_40_yugabytedb.json"
    "demo|40|mssql|MSSQL|hydrogen_test_40_mssql.json"
)

usage() {
    cat <<'EOF'
Usage: database_load.sh [--dry-run] [--group test|demo] [--engine name] [--timeout seconds]

Apply the embedded payload migrations to the build schemas. Test schemas
(tests 32-39) run together, then demo schemas (test 40) run together.
Hydrogen is stopped when the load finishes. The databases stay.

  --dry-run           Show the binary and the configs. Start nothing.
  --group test        Only the eight migration-test schemas.
  --group demo        Only the eight test 40 schemas.
  --engine name       One engine: postgresql, mysql, sqlite, db2, mariadb,
                      firebird, yugabytedb, mssql.
  --timeout seconds   How long one wave may run. Default 1800.

HYDROGEN_BIN overrides the binary. Otherwise the script uses hydrogen,
hydrogen_release, hydrogen_coverage, then hydrogen_debug in the project
root. Run from a shell that already has the test environment loaded.
EOF
}

warn() {
    printf '%s\n' "$*" >&2
}

report() {
    local status="$1"
    local group="$2"
    local number="$3"
    local label="$4"
    local detail="$5"
    printf '%-8s %-4s %2s  %-12s %s\n' "${status}" "${group^^}" "${number}" "${label}" "${detail}"
}

stop_all() {
    local pid start
    if [[ "${#PIDS[@]}" -eq 0 ]]; then
        return 0
    fi
    for pid in "${PIDS[@]}"; do
        if kill -0 "${pid}" 2>/dev/null; then
            kill -INT "${pid}" 2>/dev/null || true
        fi
    done
    start="${SECONDS}"
    for pid in "${PIDS[@]}"; do
        while kill -0 "${pid}" 2>/dev/null; do
            if [[ $((SECONDS - start)) -ge 20 ]]; then
                kill -KILL "${pid}" 2>/dev/null || true
                break
            fi
            sleep 0.2
        done
        wait "${pid}" 2>/dev/null || true
    done
}
trap stop_all EXIT
trap 'stop_all; exit 130' INT
trap 'stop_all; exit 143' TERM

normalize_engine() {
    local name="${1,,}"
    case "${name}" in
        postgres|postgresql) printf 'postgresql' ;;
        yugabyte|ydb|yugabytedb) printf 'yugabytedb' ;;
        mysql|mariadb|sqlite|db2|firebird|mssql) printf '%s' "${name}" ;;
        *)
            warn "Error: unknown engine '${1}'"
            return 1
            ;;
    esac
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --help|-h)
                usage
                exit 0
                ;;
            --dry-run)
                DRY_RUN=1
                shift
                ;;
            --group)
                if [[ $# -lt 2 ]]; then
                    warn "Error: --group needs test or demo"
                    exit 2
                fi
                GROUP_FILTER="${2,,}"
                if [[ "${GROUP_FILTER}" != "test" && "${GROUP_FILTER}" != "demo" ]]; then
                    warn "Error: --group must be test or demo"
                    exit 2
                fi
                shift 2
                ;;
            --engine)
                if [[ $# -lt 2 ]]; then
                    warn "Error: --engine needs a name"
                    exit 2
                fi
                ENGINE_FILTER="$(normalize_engine "${2}")" || exit 2
                shift 2
                ;;
            --timeout)
                if [[ $# -lt 2 || ! "${2}" =~ ^[0-9]+$ || "${2}" -lt 1 ]]; then
                    warn "Error: --timeout needs a positive number of seconds"
                    exit 2
                fi
                TIMEOUT="${2}"
                shift 2
                ;;
            *)
                warn "Error: unknown argument ${1}"
                usage >&2
                exit 2
                ;;
        esac
    done
}

find_binary() {
    local name candidate
    if [[ -n "${HYDROGEN_BIN_OVERRIDE:-}" ]]; then
        if [[ ! -x "${HYDROGEN_BIN_OVERRIDE}" ]]; then
            warn "Error: HYDROGEN_BIN is not executable: ${HYDROGEN_BIN_OVERRIDE}"
            return 1
        fi
        HYDROGEN_BIN="${HYDROGEN_BIN_OVERRIDE}"
        return 0
    fi
    for name in hydrogen hydrogen_release hydrogen_coverage hydrogen_debug; do
        candidate="${HYDROGEN_ROOT}/${name}"
        if [[ -x "${candidate}" ]]; then
            HYDROGEN_BIN="${candidate}"
            return 0
        fi
    done
    warn "Error: no Hydrogen binary in ${HYDROGEN_ROOT}"
    warn "Build with mkt, or set HYDROGEN_BIN."
    return 1
}

row_selected() {
    local row="$1"
    local group="$2"
    local row_group engine
    IFS='|' read -r row_group _ engine _ _ <<< "${row}"
    if [[ "${row_group}" != "${group}" ]]; then
        return 1
    fi
    if [[ -n "${ENGINE_FILTER}" && "${engine}" != "${ENGINE_FILTER}" ]]; then
        return 1
    fi
    return 0
}

log_has() {
    local file="$1"
    local text="$2"
    [[ -f "${file}" ]] && grep -q -F -- "${text}" "${file}"
}

classify_log() {
    local log="$1"
    if log_has "${log}" "Migration process failed"; then
        printf 'fail\n'
        return 0
    fi
    if log_has "${log}" "Migration test finished"; then
        printf 'ok\n'
        return 0
    fi
    if log_has "${log}" "Lead DQM initialization is complete"; then
        printf 'fail\n'
        return 0
    fi
    printf 'wait\n'
}

duration_text() {
    local log="$1"
    local line=""
    line="$(grep -m 1 -F "Migration completed in " "${log}" 2>/dev/null || true)"
    if [[ "${line}" =~ ([0-9]+\.[0-9]+)s ]]; then
        printf '%ss' "${BASH_REMATCH[1]}"
        return 0
    fi
    printf 'done'
}

show_tail() {
    local log="$1"
    if [[ ! -s "${log}" ]]; then
        warn "    (no log output)"
        return 0
    fi
    tail -n 12 "${log}" | while IFS= read -r line || [[ -n "${line}" ]]; do
        warn "    ${line}"
    done
}

prepare_config() {
    local src="$1"
    local dest="$2"
    jq '.Databases.Connections[].TestMigration = false' "${src}" > "${dest}" || return 1
    jq -e 'all(.Databases.Connections[]; .TestMigration == false)' "${dest}" >/dev/null || return 1
}

run_wave() {
    local group="$1"
    local row config src cfg log pid state detail
    local w_g w_n w_e w_l
    local -a w_group=() w_number=() w_label=() w_log=() w_pid=() w_done=()
    local i count start pending
    local launched=0

    for row in "${TARGETS[@]}"; do
        if ! row_selected "${row}" "${group}"; then
            continue
        fi
        IFS='|' read -r w_g w_n w_e w_l config <<< "${row}"
        src="${HYDROGEN_ROOT}/tests/configs/${config}"
        if [[ ! -f "${src}" ]]; then
            report "FAILED" "${w_g}" "${w_n}" "${w_l}" "missing ${config}"
            FAILS=$((FAILS + 1))
            continue
        fi
        if [[ "${DRY_RUN}" -eq 1 ]]; then
            report "PLAN" "${w_g}" "${w_n}" "${w_l}" "${config}"
            launched=$((launched + 1))
            continue
        fi
        cfg="${LOG_DIR}/${w_g}_${w_n}_${w_e}.json"
        log="${LOG_DIR}/${w_g}_${w_n}_${w_e}.log"
        if ! prepare_config "${src}" "${cfg}"; then
            report "FAILED" "${w_g}" "${w_n}" "${w_l}" "could not prepare config"
            FAILS=$((FAILS + 1))
            continue
        fi
        "${HYDROGEN_BIN}" "${cfg}" >"${log}" 2>&1 &
        pid="$!"
        PIDS+=("${pid}")
        w_group+=("${w_g}")
        w_number+=("${w_n}")
        w_label+=("${w_l}")
        w_log+=("${log}")
        w_pid+=("${pid}")
        w_done+=(0)
        report "START" "${w_g}" "${w_n}" "${w_l}" "pid ${pid}"
        launched=$((launched + 1))
    done

    if [[ "${launched}" -eq 0 ]]; then
        return 0
    fi
    if [[ "${DRY_RUN}" -eq 1 ]]; then
        return 0
    fi

    count="${#w_pid[@]}"
    pending="${count}"
    start="${SECONDS}"
    while [[ "${pending}" -gt 0 ]]; do
        if [[ $((SECONDS - start)) -ge "${TIMEOUT}" ]]; then
            break
        fi
        i=0
        while [[ "${i}" -lt "${count}" ]]; do
            if [[ "${w_done[${i}]}" -eq 1 ]]; then
                i=$((i + 1))
                continue
            fi
            if ! kill -0 "${w_pid[${i}]}" 2>/dev/null; then
                report "FAILED" "${w_group[${i}]}" "${w_number[${i}]}" "${w_label[${i}]}" "Hydrogen exited early"
                show_tail "${w_log[${i}]}"
                FAILS=$((FAILS + 1))
                w_done[i]=1
                pending=$((pending - 1))
                i=$((i + 1))
                continue
            fi
            state="$(classify_log "${w_log[${i}]}")"
            if [[ "${state}" == "wait" ]]; then
                i=$((i + 1))
                continue
            fi
            if [[ "${state}" == "ok" ]]; then
                detail="$(duration_text "${w_log[${i}]}")"
                report "OK" "${w_group[${i}]}" "${w_number[${i}]}" "${w_label[${i}]}" "${detail}"
            else
                report "FAILED" "${w_group[${i}]}" "${w_number[${i}]}" "${w_label[${i}]}" "migration failed"
                show_tail "${w_log[${i}]}"
                FAILS=$((FAILS + 1))
            fi
            kill -INT "${w_pid[${i}]}" 2>/dev/null || true
            w_done[i]=1
            pending=$((pending - 1))
            i=$((i + 1))
        done
        if [[ "${pending}" -gt 0 ]]; then
            sleep 0.5
        fi
    done

    i=0
    while [[ "${i}" -lt "${count}" ]]; do
        if [[ "${w_done[${i}]}" -eq 0 ]]; then
            report "FAILED" "${w_group[${i}]}" "${w_number[${i}]}" "${w_label[${i}]}" "timed out after ${TIMEOUT}s"
            show_tail "${w_log[${i}]}"
            FAILS=$((FAILS + 1))
            kill -INT "${w_pid[${i}]}" 2>/dev/null || true
        fi
        i=$((i + 1))
    done
    stop_all
    PIDS=()
}

main() {
    local script_dir group
    parse_args "$@"
    if ! command -v jq >/dev/null 2>&1; then
        warn "Error: jq not found"
        exit 2
    fi
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    if [[ -n "${HYDROGEN_ROOT:-}" ]]; then
        HYDROGEN_ROOT="$(cd "${HYDROGEN_ROOT}" && pwd)"
    else
        HYDROGEN_ROOT="$(cd "${script_dir}/.." && pwd)"
    fi
    find_binary || exit 2
    cd "${HYDROGEN_ROOT}"

    printf 'Binary %s\n' "${HYDROGEN_BIN}"
    printf 'Test schemas run together, then demo schemas. TestMigration stays off.\n\n'

    if [[ "${DRY_RUN}" -eq 0 ]]; then
        LOG_DIR="$(mktemp -d "${TMPDIR:-/tmp}/database_load.XXXXXX")" || {
            warn "Error: could not create a log directory"
            exit 2
        }
        printf 'Logs %s\n\n' "${LOG_DIR}"
    fi

    for group in test demo; do
        if [[ -n "${GROUP_FILTER}" && "${group}" != "${GROUP_FILTER}" ]]; then
            continue
        fi
        printf 'Wave %s\n' "${group}"
        run_wave "${group}"
        printf '\n'
    done

    if [[ "${DRY_RUN}" -eq 1 ]]; then
        printf 'Dry run. No servers started.\n'
        exit 0
    fi
    if [[ "${FAILS}" -ne 0 ]]; then
        printf '%s engine(s) failed. Logs: %s\n' "${FAILS}" "${LOG_DIR}"
        exit 1
    fi
    printf 'Load complete. Databases were left in place.\n'
    rm -rf "${LOG_DIR}"
    LOG_DIR=""
    exit 0
}

main "$@"
