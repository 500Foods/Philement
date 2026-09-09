#!/usr/bin/env bash

# SchemaHelper blackbox helpers for tests/test_72_schemahelper.sh.
# Lua fixtures live in tests/lib/schemahelper/ (split for the 1000-line cap).

# shellcheck disable=SC2154 # Globals (TEST_NUMBER, TEST_COUNTER) come from the test script / framework.sh
# shellcheck disable=SC2312 # Diagnostic substitutions swallow inner status; callers use || true

# CHANGELOG
# 1.0.0 - 2026-09-09 - Split from test_72 to stay under the 1000-line cap

[[ -n "${SCHEMAHELPER_HELPERS_GUARD:-}" ]] && return 0
export SCHEMAHELPER_HELPERS_GUARD="true"
: "${EXIT_CODE:=0}"
export EXIT_CODE

SCHEMAHELPER_HELPERS_NAME="SchemaHelper Test Helpers"
SCHEMAHELPER_HELPERS_VERSION="1.0.0"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${SCHEMAHELPER_HELPERS_NAME} ${SCHEMAHELPER_HELPERS_VERSION}" "info"

SCHEMAHELPER_LUA_DIR="$(dirname "${BASH_SOURCE[0]}")/schemahelper"
SCHEMAGUI="${HYDROGEN_ROOT}/extras/schematool"
FIXTURE_DIR="${SCHEMAGUI}/test/fixtures/sample_project"
export SCHEMAHELPER_LUA_DIR SCHEMAGUI FIXTURE_DIR

schemahelper_setup_lua_path() {
    if command -v luarocks > /dev/null 2>&1; then
        local lua_path_setup orig_cpath
        lua_path_setup=$(luarocks --lua-version=5.5 path 2>/dev/null || true)
        if [[ -n "${lua_path_setup}" ]]; then
            orig_cpath="${LUA_CPATH:-}"
            eval "${lua_path_setup}"
            if [[ -n "${orig_cpath}" ]]; then
                LUA_CPATH="${LUA_CPATH};${orig_cpath}"
                export LUA_CPATH
            fi
        fi
    fi
}

print_multi_output() {
    local text="$1"
    while IFS= read -r line; do
        [[ -z "${line}" ]] && continue
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
    done <<< "${text}"
}

run_schemahelper_lua() {
    local script_name="$1"
    local script="${SCHEMAHELPER_LUA_DIR}/${script_name}"
    if [[ ! -f "${script}" ]]; then
        LUAP_OUT="ERR: missing lua fixture ${script}"
        LUAP_OK=0
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${LUAP_OUT}"
        return
    fi
    LUAP_OUT=$(lua -e "package.path = '${SCHEMAHELPER_LUA_DIR}/?.lua;' .. package.path" "${script}" 2>&1) || true
    LUAP_OK=0
    if echo "${LUAP_OUT}" | grep -q "^OK:"; then
        LUAP_OK=1
    fi
    print_multi_output "${LUAP_OUT}"
}

schemahelper_lua_result() {
    local pass_msg="$1"
    local fail_msg="$2"
    if [[ "${LUAP_OK}" -eq 1 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${pass_msg}"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${fail_msg}"
        EXIT_CODE=1
    fi
}

schemahelper_lua_subtest() {
    local title="$1"
    local script_name="$2"
    local pass_msg="$3"
    local fail_msg="$4"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${title}"
    run_schemahelper_lua "${script_name}"
    schemahelper_lua_result "${pass_msg}" "${fail_msg}"
}

schemahelper_check_fixtures() {
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Fixture files present"

    local expected_files=(
        "findings.json"
        "catalog_findings.json"
        "schemahelper_acuranzo_sqlite.json"
        "migrations/design_1000.lua"
        "migrations/design_0100.lua"
        "migrations/design_1148.lua"
        "migrations/design_1200.lua"
        "migrations/design_1290.lua"
        "schemas/queries.sql"
        "finding_detail_meta_drift_1148.txt"
        "catalog_finding_detail_nullable.txt"
        "schematool_sqlite_fixture.sh"
    )

    local missing=0
    local f
    for f in "${expected_files[@]}"; do
        if [[ ! -f "${FIXTURE_DIR}/${f}" ]]; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing: ${f}"
            missing=$((missing + 1))
        fi
    done

    if [[ "${missing}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#expected_files[@]} fixture files present"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${missing} fixture files missing"
        EXIT_CODE=1
    fi
}

schemahelper_check_lua() {
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Lua 5.5 + terminal.lua availability"

    local lua_ok=false
    local lua_ver=""
    if command -v lua > /dev/null 2>&1; then
        lua_ver=$(lua -v 2>/dev/null | grep -oP 'Lua \K[0-9]+\.[0-9]+' || true)
        if [[ "${lua_ver}" == "5.5" ]]; then
            lua_ok=true
        fi
    fi

    local terminal_ok=false
    if lua -e 'require("terminal")' > /dev/null 2>&1; then
        terminal_ok=true
    fi

    if ${lua_ok} && ${terminal_ok}; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Lua 5.5 + terminal.lua available"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Lua ${lua_ver:-not found} or terminal.lua unavailable"
        EXIT_CODE=1
    fi
}

schemahelper_luacheck() {
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "luacheck clean on schemahelper files"

    local luacheck_ok=1
    local luacheck_files=(
        "schemahelper.lua"
        "lua/schemahelper_qutil.lua"
        "lua/schemahelper_apply.lua"
        "lua/schemahelper_packet.lua"
        "lua/schemahelper_connect.lua"
        "lua/schemahelper_qstate.lua"
        "lua/schemahelper_qload.lua"
        "lua/schemahelper_qdecode.lua"
        "lua/schemahelper_queue.lua"
        "lua/schemahelper_screens.lua"
        "lua/schemahelper_decode_test.lua"
        "lua/schemahelper_actions.lua"
        "lua/schemahelper_explore.lua"
        "lua/schemahelper_invoke.lua"
        "lua/schemahelper_mouse.lua"
        "lua/schemahelper_paint.lua"
        "lua/schemahelper_smoke_queue.lua"
        "lua/schemahelper_ui.lua"
        "lua/schemahelper_wrappers.lua"
        "lua/schemahelper_const.lua"
    )

    local f full output
    for f in "${luacheck_files[@]}"; do
        full="${SCHEMAGUI}/${f}"
        if [[ -f "${full}" ]]; then
            output=$(luacheck --std=max --max-line-length=120 --ignore 542,561 --no-self --no-unused-args --formatter=plain "${full}" 2>&1 || true)
            if [[ -n "${output}" ]] && ! echo "${output}" | grep -qi "^No errors"; then
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "luacheck issues in ${f}:"
                while IFS= read -r line; do
                    [[ -z "${line}" ]] && continue
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${line}"
                done <<< "${output}"
                luacheck_ok=0
            fi
        else
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Cannot find ${f}"
            luacheck_ok=0
        fi
    done

    if [[ "${luacheck_ok}" -eq 1 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#luacheck_files[@]} schemahelper files pass luacheck"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "luacheck issues found"
        EXIT_CODE=1
    fi
}

schemahelper_cli_help() {
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "schemahelper.sh --help / --version"

    local schemagui_sh="${SCHEMAGUI}/schemahelper.sh"
    local help_ok=false
    local sch_help_out
    sch_help_out=$("${schemagui_sh}" --help 2>&1) || true
    if echo "${sch_help_out}" | grep -qi "usage\|help\|schemahelper"; then
        help_ok=true
    fi

    local ver_out
    ver_out=$("${schemagui_sh}" --version 2>&1 || true)
    local ver_ok=false
    if echo "${ver_out}" | grep -q "SchemaHelper"; then
        ver_ok=true
    fi

    if ${help_ok} && ${ver_ok}; then
        print_multi_output "${ver_out}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "schemahelper.sh --help/--version work"
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "help_ok=${help_ok} ver_ok=${ver_ok}"
        print_multi_output "${ver_out}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "schemahelper.sh --help/version failed"
        EXIT_CODE=1
    fi
}

schemahelper_lua_version() {
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "schemahelper.lua --version"

    local schemalua="${SCHEMAGUI}/schemahelper.lua"
    local luav_out
    luav_out=$(lua "${schemalua}" --version 2>&1 || true)
    if echo "${luav_out}" | grep -q "0.6"; then
        print_multi_output "${luav_out}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "SchemaHelper --version works"
    else
        print_multi_output "${luav_out}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Version not found or wrong format"
        EXIT_CODE=1
    fi
}

schemahelper_cleanup() {
    if [[ -f "${FIXTURE_DIR}/.test_roundtrip_state.json" ]]; then
        rm -f "${FIXTURE_DIR}/.test_roundtrip_state.json"
    fi
    if [[ -f "${FIXTURE_DIR}/.test_hash_state.json" ]]; then
        rm -f "${FIXTURE_DIR}/.test_hash_state.json"
    fi
}
