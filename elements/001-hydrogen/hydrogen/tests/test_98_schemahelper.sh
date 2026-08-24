#!/usr/bin/env bash

# Test: SchemaHelper Phase 98 Integration
# Exercise schemahelper_queue.lua and schemahelper.lua against
# checked-in fixture findings (no live DB required).

# CHANGELOG
# 1.0.0 - 2026-08-23 - Initial version for Phase 98 fixture validation

set -euo pipefail

# Test configuration
TEST_NAME="SchemaHelper Phase 98"
TEST_ABBR="SH98"
TEST_NUMBER="98"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

SCHEMAGUI="${HYDROGEN_ROOT}/extras/schematool"
FIXTURE_DIR="${SCHEMAGUI}/test/fixtures/sample_project"

# Also set up lua path for luarocks rocks
if command -v luarocks > /dev/null 2>&1; then
    LUA_PATH_SETUP=$(luarocks --lua-version=5.5 path 2>/dev/null || true)
    if [[ -n "${LUA_PATH_SETUP}" ]]; then
        eval "${LUA_PATH_SETUP}"
    fi
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "SchemaHelper Phase 98 fixture validation"

# ---------------------------------------------------------------------------
# 1. Verify fixture files exist
# ---------------------------------------------------------------------------
TEST_COUNTER=$((TEST_COUNTER + 1))
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Fixture files present"

EXPECTED_FILES=(
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

missing=0
for f in "${EXPECTED_FILES[@]}"; do
    if [[ ! -f "${FIXTURE_DIR}/${f}" ]]; then
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing: ${f}"
        missing=$((missing + 1))
    fi
done

if [[ "${missing}" -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#EXPECTED_FILES[@]} fixture files present"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${missing} fixture files missing"
    EXIT_CODE=1
fi

if [[ "${EXIT_CODE:-0}" -ne 0 ]]; then
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi

# ---------------------------------------------------------------------------
# 2. Verify Lua 5.5 + terminal.lua availability
# ---------------------------------------------------------------------------
TEST_COUNTER=$((TEST_COUNTER + 1))
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Lua 5.5 + terminal.lua availability"

if ! command -v lua > /dev/null 2>&1; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "lua not found on PATH"
    EXIT_CODE=1
else
    LUA_VER=$(lua -v 2>&1 | grep -oP 'Lua \K[0-9]+\.[0-9]+')
    if [[ "${LUA_VER}" != "5.5" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Lua ${LUA_VER} found, need 5.5"
        EXIT_CODE=1
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Lua 5.5 available"
    fi
fi

if ! lua -e 'require("terminal")' 2>/dev/null; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "terminal.lua not loadable via system lua"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "terminal.lua not available"
    EXIT_CODE=1
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "terminal.lua available"
fi

# ---------------------------------------------------------------------------
# 3. Validate queue module functions exist and operate on fixture data
# ---------------------------------------------------------------------------
TEST_COUNTER=$((TEST_COUNTER + 1))
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Queue module functions and fixture validation"

QUEUE_TEST=$(lua 2>&1 <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local queue = require("schemahelper_queue")

local fixture = "${FIXTURE_DIR}"

-- Verify required functions exist
local ok = true
local errs = {}

local required = {
    "build",
    "load_state",
    "create_state",
    "default_state_path",
    "artifacts_present",
    "load_metadata",
    "load_detail_section",
    "note_for",
    "explore_lines",
    "save_decision",
    "save_cursor",
    "jq_update_state",
    "build_dashboard_lines",
    "build_review_lines",
    "find_finding",
    "json_subobj",
    "payload_text",
}
for _, fn in ipairs(required) do
    if type(queue[fn]) ~= "function" then
        errs[#errs+1] = "queue." .. fn .. " is not a function"
        ok = false
    end
end

if #errs > 0 then
    for _, e in ipairs(errs) do
        print("ERR: " .. e)
    end
    os.exit(1)
else
    print("OK: all required functions present")
end

-- Test artifacts_present
local present = queue.artifacts_present(fixture, "both")
if not present then
    print("ERR: artifacts_present returned false")
    os.exit(1)
end
print("OK: artifacts_present=true")

-- Test load_state
local state = queue.load_state(fixture .. "/schemahelper_acuranzo_sqlite.json")
if state.design ~= "acuranzo" then
    print("ERR: design=" .. tostring(state.design))
    os.exit(1)
end
print("OK: state design=" .. state.design .. " engine=" .. state.engine)

-- Test cursor_id persistence
if state.cursor_id ~= "meta:drift:1148:1003:name" then
    print("ERR: cursor_id=" .. tostring(state.cursor_id))
    os.exit(1)
end
print("OK: cursor_id=" .. state.cursor_id)

-- Test decisions load
if #state.decisions < 1 then
    print("ERR: no decisions loaded")
    os.exit(1)
end
print("OK: decisions loaded: " .. #state.decisions)

-- Test build produces findings
local built = queue.build({
    out_dir = fixture,
    track = "both",
    state = state,
})
print("OK: build totals total=" .. built.totals.total .. " subject=" .. built.totals.subject)

-- Test build_dashboard_lines
local dash, built2 = queue.build_dashboard_lines({
    out_dir = fixture,
    track = "both",
    state = state,
})
if not dash or #dash == 0 then
    print("ERR: dashboard lines empty")
    os.exit(1)
end
local dash_text = table.concat(dash, "\n")
if not dash_text:find("Findings for review", 1, true) then
    print("ERR: dashboard missing findings-for-review label")
    os.exit(1)
end
if dash_text:find("Subject for review", 1, true) then
    print("ERR: dashboard still labels the queue as migrations/subject")
    os.exit(1)
end
print("OK: dashboard produces " .. #dash .. " lines")

-- Test build_review_lines
local review_lines = queue.build_review_lines(built.findings[1])
if not review_lines or #review_lines == 0 then
    print("ERR: review lines empty")
    os.exit(1)
end
print("OK: review produces " .. #review_lines .. " lines")

-- Test explore_lines
local explore = queue.explore_lines(fixture, "orphan:1290", built.findings)
if not explore or #explore == 0 then
    print("ERR: explore lines empty")
    os.exit(1)
end
print("OK: explore produces " .. #explore .. " lines")

-- Test note_for
local note = queue.note_for("meta:drift:1148:1003:name", fixture, state)
if note ~= "known drift" then
    print("ERR: note_for returned: " .. tostring(note))
    os.exit(1)
end
print("OK: note_for returns: " .. note)

-- Test save_decision + save_cursor round-trip
local tmp_state = fixture .. "/.test_roundtrip_state.json"
local f = io.open(tmp_state, "w")
f:write('{"version":1,"design":"acuranzo","engine":"sqlite","schema":"","updated_utc":"2026-08-23T12:00:00Z","cursor_id":"","decisions":[]}')
f:close()

queue.save_decision(tmp_state, "cat:accounts:id:nullable", "accepted", {hash="test", note="test note"})
queue.save_cursor(tmp_state, "cat:accounts:id:nullable")

local h = io.open(tmp_state, "r")
local content = h:read("*a")
h:close()
assert(content:find('"accepted"'), "decision not persisted")
assert(content:find('"cursor_id"'), "cursor not persisted")
assert(content:find('"cat:accounts:id:nullable"'), "finding id not persisted")
print("OK: save_decision + save_cursor round-trip works")

os.remove(tmp_state)
os.exit(0)
LUAEOF
) 2>&1 || true

if echo "${QUEUE_TEST}" | grep -q "^OK:"; then
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${QUEUE_TEST}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Queue module functions validate against fixture"
else
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${QUEUE_TEST}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Queue module functions failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 4. Validate re-audit reload preserves decisions
# ---------------------------------------------------------------------------
TEST_COUNTER=$((TEST_COUNTER + 1))
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Re-audit reload preserves decisions"

REAUDIT_TEST=$(lua 2>&1 <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local queue = require("schemahelper_queue")

local fixture = "${FIXTURE_DIR}"
local state = queue.load_state(fixture .. "/schemahelper_acuranzo_sqlite.json")

local built = queue.build({
    out_dir = fixture,
    track = "both",
    state = state,
})

local found_accepted = false
for _, f in ipairs(built.findings) do
    if f.id == "meta:drift:1148:1003:name" and f.action == "accepted" then
        found_accepted = true
        break
    end
end

if not found_accepted then
    print("ERR: accepted finding not found in findings")
    os.exit(1)
end

if built.totals.accepted < 1 then
    print("ERR: accepted count too low")
    os.exit(1)
end

print("OK: accepted findings persist across rebuild, accepted=" .. built.totals.accepted)
print("OK: subject count=" .. built.totals.subject)
os.exit(0)
LUAEOF
) 2>&1 || true

if echo "${REAUDIT_TEST}" | grep -q "^OK:"; then
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${REAUDIT_TEST}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Decisions persist across re-audit"
else
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${REAUDIT_TEST}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Decision persistence failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 5. Validate luacheck passes on schemahelper files
# ---------------------------------------------------------------------------
TEST_COUNTER=$((TEST_COUNTER + 1))
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "luacheck clean on schemahelper files"

LUACHECK_OK=1

for f in "schemahelper.lua" "lua/schemahelper_queue.lua" "lua/schemahelper_connect.lua"; do
    full="${SCHEMAGUI}/${f}"
    if [[ -f "${full}" ]]; then
        output=$(luacheck --std=max --max-line-length=120 --ignore 542,561 --no-self --no-unused-args --formatter=plain "${full}" 2>&1 || true)
        if [[ -n "${output}" ]] && ! echo "${output}" | grep -qi "^No errors"; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "luacheck issues in ${f}:"
            echo "${output}" | while read -r line; do
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${line}"
            done
            LUACHECK_OK=0
        else
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "luacheck clean: ${f}"
        fi
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Cannot find ${f}"
        LUACHECK_OK=0
    fi
done

if [[ "${LUACHECK_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All schemahelper files pass luacheck"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "luacheck issues found"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 6. Validate schemahelper.sh --help and --version
# ---------------------------------------------------------------------------
TEST_COUNTER=$((TEST_COUNTER + 1))
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "schemahelper.sh --help / --version"

SCHEMAGUI_SH="${SCHEMAGUI}/schemahelper.sh"
if "${SCHEMAGUI_SH}" --help > /dev/null 2>&1; then
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "schemahelper.sh --help exits 0"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "schemahelper.sh --help failed"
    EXIT_CODE=1
fi

VER_OUT=$("${SCHEMAGUI_SH}" --version 2>&1 || true)
if echo "${VER_OUT}" | grep -q "SchemaHelper"; then
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${VER_OUT}"
else
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${VER_OUT}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Version not reported"
    EXIT_CODE=1
fi

if [[ "${EXIT_CODE:-0}" -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "schemahelper.sh --help/--version work"
fi

# ---------------------------------------------------------------------------
# 7. Validate schemahelper.lua --version prints version info
# ---------------------------------------------------------------------------
TEST_COUNTER=$((TEST_COUNTER + 1))
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "schemahelper.lua --version"

SCHEMALUA="${SCHEMAGUI}/schemahelper.lua"
VER_OUT=$(lua "${SCHEMALUA}" --version 2>&1 || true)
if echo "${VER_OUT}" | grep -q "0.5"; then
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${VER_OUT}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "SchemaHelper --version works"
else
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${VER_OUT}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Version not found or wrong format"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 8. Validate TextPanel is loadable (terminal.lua integration)
# ---------------------------------------------------------------------------
TEST_COUNTER=$((TEST_COUNTER + 1))
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "terminal.lua TextPanel available"

TP_TEST=$(lua 2>&1 <<'LUAEOF'
local ok, TextPanel = pcall(require, "terminal.ui.panel.text")
if not ok then
    print("ERR: cannot require terminal.ui.panel.text")
    os.exit(1)
end
print("OK: TextPanel loaded: " .. tostring(TextPanel))
os.exit(0)
LUAEOF
) 2>&1 || true

if echo "${TP_TEST}" | grep -q "^OK:"; then
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${TP_TEST}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "TextPanel loaded from terminal.lua"
else
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${TP_TEST}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "TextPanel not available"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
