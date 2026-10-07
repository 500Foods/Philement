#!/usr/bin/env bash
# SchemaTool — Test 40 multi-engine catalog smoke (1190 accounts.password_hash)
#
# Runs --catalog --only-tables accounts against each Test 40 layout wrapper.
# Expect exit 0 and password_hash expected=true live=true on each engine.
#
# Usage (from anywhere, needs zsh env / DB credentials):
#   extras/schematool/smoke_test40_catalog.sh
#   extras/schematool/smoke_test40_catalog.sh --out-dir /tmp/st40
#
# CHANGELOG
# 1.1.3 - 2026-10-07 - Call schematool_<engine>_demo.sh. MSSQL stays in the list.
# 1.1.2 - 2026-10-07 - MariaDB is its own row. Expect log must name engine=mariadb.
# 1.1.1 - 2026-10-07 - Firebird query and catalog adapters exist. password_hash check unchanged.
# 1.1.0 - 2026-10-07 - Add mssql (schema demoms). Firebird still lacks dump adapters.
# 1.0.0 - 2026-08-06 - Initial multi-engine 1190 acceptance smoke

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${TMPDIR:-/tmp}/schematool-t40-smoke-$$"
ENGINES=(sqlite postgresql mysql mariadb db2 firebird yugabytedb mssql)

while [[ $# -gt 0 ]]; do
    case "$1" in
        --out-dir) OUT_DIR="${2:-}"; shift 2 ;;
        -h|--help)
            echo "Usage: $0 [--out-dir DIR]"
            exit 0
            ;;
        *)
            echo "Error: unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

mkdir -p "${OUT_DIR}"
SUMMARY="${OUT_DIR}/summary.txt"
: >"${SUMMARY}"

pass=0
fail=0

run_one() {
    local name="$1"
    local wrapper="${SCRIPT_DIR}/schematool_${name}_demo.sh"
    local odir="${OUT_DIR}/${name}"
    local log="${OUT_DIR}/${name}.log"
    mkdir -p "${odir}"

    if [[ ! -x "${wrapper}" ]]; then
        echo "${name}: FAIL (missing wrapper)" | tee -a "${SUMMARY}"
        fail=$((fail + 1))
        return 0
    fi

    set +e
    "${wrapper}" \
        --catalog --only-tables accounts \
        --out-dir "${odir}" --no-sql --format json \
        >"${log}" 2>&1
    local ec=$?
    set -e

    local expect_ok=1
    if [[ "${name}" == "mariadb" ]]; then
        set +e
        "${wrapper}" --emit-expected --from 1000 --to 1000 --no-sql --format json \
            >"${odir}/expect.json" 2>>"${log}"
        local erc=$?
        set -e
        expect_ok=0
        if [[ "${erc}" -eq 0 ]] && grep -q 'engine=mariadb' "${log}"; then
            expect_ok=1
        fi
    fi

    local ph_ok=0
    if [[ -f "${odir}/catalog_checklist.json" ]]; then
        if jq -e '
            map(select(.column == "password_hash" and .check == "nullable"))
            | length > 0
            and all(.status == "Y" and .expected == "true" and .live == "true")
          ' "${odir}/catalog_checklist.json" >/dev/null 2>&1; then
            ph_ok=1
        fi
    elif jq -e '
            map(select(.column == "password_hash" and .check == "nullable"))
            | length > 0
            and all(.status == "Y" and .expected == "true" and .live == "true")
          ' "${log}" >/dev/null 2>&1; then
        ph_ok=1
    fi

    if [[ "${ec}" -eq 0 && "${ph_ok}" -eq 1 && "${expect_ok}" -eq 1 ]]; then
        echo "${name}: PASS (exit=${ec}, password_hash nullable Y)" | tee -a "${SUMMARY}"
        pass=$((pass + 1))
    else
        echo "${name}: FAIL (exit=${ec}, password_hash_ok=${ph_ok}, expect_ok=${expect_ok}) log=${log}" | tee -a "${SUMMARY}"
        fail=$((fail + 1))
        tail -30 "${log}" >>"${SUMMARY}" || true
    fi
}

echo "SchemaTool Test 40 catalog smoke → ${OUT_DIR}"
for eng in "${ENGINES[@]}"; do
    run_one "${eng}"
done

echo "" | tee -a "${SUMMARY}"
echo "Result: ${pass} pass / ${fail} fail / ${#ENGINES[@]} engines" | tee -a "${SUMMARY}"

if [[ "${fail}" -gt 0 ]]; then
    exit 2
fi
exit 0
