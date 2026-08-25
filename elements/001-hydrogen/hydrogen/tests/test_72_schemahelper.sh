#!/usr/bin/env bash

# Test: SchemaHelper Phase 72 Integration
# Exercise schemahelper_queue.lua, schemahelper_packet.lua,
# schemahelper_apply.lua, schemahelper_connect.lua and
# schemahelper_qutil.lua against checked-in fixture findings
# (no live DB required; brotli-only paths guarded).

# CHANGELOG
# 1.1.0 - 2026-08-25 - Rewritten for framework contract: no manual
#   TEST_COUNTER increments, no orphan print_subtest, exactly one
#   print_result per subtest, multi-line Lua output routed line-by-line
#   through print_output. Added pure-module subtests for qutil, packet,
#   apply, connect. Expanded luacheck to all lua/ submodules.
# 1.0.2 - 2026-08-25 - Added dashboard_content render regression test against
#   the fixture (headless terminal stubs); added lua/schemahelper_screens.lua
#   to the luacheck list. Guards the packet-vs-queue resolver after the
#   monolith split.
# 1.0.1 - 2026-08-24 - Renumbered from 98 to 72, abbr changed to SCH; added
#   decode_embedded dialect tests (SQLite, MySQL upper+lower, DB2, PostgreSQL);
#   fixed brotli C module path after luarocks LUA_CPATH overrides.
# 1.0.0 - 2026-08-23 - Initial version for Phase 98 fixture validation

set -euo pipefail

# Test configuration
TEST_NAME="SchemaHelper"
TEST_ABBR="SCH"
TEST_NUMBER="72"
TEST_COUNTER=0
TEST_VERSION="1.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

SCHEMAGUI="${HYDROGEN_ROOT}/extras/schematool"
FIXTURE_DIR="${SCHEMAGUI}/test/fixtures/sample_project"

# Set up lua path for luarocks rocks (provides terminal.lua, brotli.so, etc.)
if command -v luarocks > /dev/null 2>&1; then
    LUA_PATH_SETUP=$(luarocks --lua-version=5.5 path 2>/dev/null || true)
    if [[ -n "${LUA_PATH_SETUP}" ]]; then
        _orig_cpath="${LUA_CPATH:-}"
        eval "${LUA_PATH_SETUP}"
        if [[ -n "${_orig_cpath}" ]]; then
            LUA_CPATH="${LUA_CPATH};${_orig_cpath}"
        fi
    fi
fi

# ---------------------------------------------------------------------------
# Helper: run Lua code from stdin, print each output line through the
# framework's print_output, and set LUAP_OK / LUAP_OUT for assertion.
# ---------------------------------------------------------------------------
run_lua() {
    LUAP_OUT=$(lua 2>&1) || true
    LUAP_OK=0
    if echo "${LUAP_OUT}" | grep -q "^OK:"; then
        LUAP_OK=1
    fi
    print_multi_output "${LUAP_OUT}"
}

print_multi_output() {
    local text="$1"
    while IFS= read -r line; do
        [[ -z "${line}" ]] && continue
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
    done <<< "${text}"
}

# ---------------------------------------------------------------------------
# 1. Verify fixture files exist
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# 2. Verify Lua 5.5 + terminal.lua availability (single result)
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Lua 5.5 + terminal.lua availability"

lua_ok=false
LUA_VER=""
if command -v lua > /dev/null 2>&1; then
    LUA_VER=$(lua -v 2>/dev/null | grep -oP 'Lua \K[0-9]+\.[0-9]+' || true)
    if [[ "${LUA_VER}" == "5.5" ]]; then
        lua_ok=true
    fi
fi

terminal_ok=false
if lua -e 'require("terminal")' > /dev/null 2>&1; then
    terminal_ok=true
fi

if ${lua_ok} && ${terminal_ok}; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Lua 5.5 + terminal.lua available"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Lua ${LUA_VER:-not found} or terminal.lua unavailable"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 3. Validate queue module functions exist and operate on fixture data
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Queue module functions and fixture validation"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local queue = require('schemahelper_queue')
local fixture = '${FIXTURE_DIR}'

local ok = true
local errs = {}

local required = {
    'build', 'load_state', 'create_state', 'default_state_path',
    'artifacts_present', 'load_metadata', 'load_detail_section',
    'note_for', 'explore_lines', 'save_decision', 'save_cursor',
    'jq_update_state', 'build_dashboard_lines', 'build_review_lines',
    'find_finding', 'json_subobj', 'payload_text',
}
for _, fn in ipairs(required) do
    if type(queue[fn]) ~= 'function' then
        errs[#errs+1] = 'queue.' .. fn .. ' is not a function'
        ok = false
    end
end

if #errs > 0 then
    for _, e in ipairs(errs) do print('ERR: ' .. e) end
    os.exit(1)
end
print('OK: all ' .. #required .. ' required functions present')

local present = queue.artifacts_present(fixture, 'both')
if not present then
    print('ERR: artifacts_present returned false')
    os.exit(1)
end
print('OK: artifacts_present=true')

local state = queue.load_state(fixture .. '/schemahelper_acuranzo_sqlite.json')
if state.design ~= 'acuranzo' then
    print('ERR: design=' .. tostring(state.design))
    os.exit(1)
end
print('OK: state design=' .. state.design .. ' engine=' .. state.engine)

if state.cursor_id ~= 'meta:drift:1148:1003:name' then
    print('ERR: cursor_id=' .. tostring(state.cursor_id))
    os.exit(1)
end
print('OK: cursor_id=' .. state.cursor_id)

if #state.decisions < 1 then
    print('ERR: no decisions loaded')
    os.exit(1)
end
print('OK: decisions loaded: ' .. #state.decisions)

local built = queue.build({
    out_dir = fixture,
    track = 'both',
    state = state,
})
print('OK: build totals total=' .. built.totals.total .. ' subject=' .. built.totals.subject)

local dash, built2 = queue.build_dashboard_lines({
    out_dir = fixture,
    track = 'both',
    state = state,
})
if not dash or #dash == 0 then
    print('ERR: dashboard lines empty')
    os.exit(1)
end
local dash_text = table.concat(dash, '\n')
if not dash_text:find('Findings for review', 1, true) then
    print('ERR: dashboard missing findings-for-review label')
    os.exit(1)
end
if dash_text:find('Subject for review', 1, true) then
    print('ERR: dashboard still labels the queue as migrations/subject')
    os.exit(1)
end
print('OK: dashboard produces ' .. #dash .. ' lines')

local review_lines = queue.build_review_lines(built.findings[1])
if not review_lines or #review_lines == 0 then
    print('ERR: review lines empty')
    os.exit(1)
end
print('OK: review produces ' .. #review_lines .. ' lines')

local explore = queue.explore_lines(fixture, 'orphan:1290', built.findings)
if not explore or #explore == 0 then
    print('ERR: explore lines empty')
    os.exit(1)
end
print('OK: explore produces ' .. #explore .. ' lines')

local note = queue.note_for('meta:drift:1148:1003:name', fixture, state)
if note ~= 'known drift' then
    print('ERR: note_for returned: ' .. tostring(note))
    os.exit(1)
end
print('OK: note_for returns: ' .. note)

local tmp_state = fixture .. '/.test_roundtrip_state.json'
local f = io.open(tmp_state, 'w')
f:write('{"version":1,"design":"acuranzo","engine":"sqlite","schema":"","updated_utc":"2026-08-23T12:00:00Z","cursor_id":"","decisions":[]}')
f:close()

queue.save_decision(tmp_state, 'cat:accounts:id:nullable', 'accepted', {hash='test', note='test note'})
queue.save_cursor(tmp_state, 'cat:accounts:id:nullable')

local h = io.open(tmp_state, 'r')
local content = h:read('*a')
h:close()
assert(content:find('"accepted"'), 'decision not persisted')
assert(content:find('"cursor_id"'), 'cursor not persisted')
assert(content:find('"cat:accounts:id:nullable"'), 'finding id not persisted')
print('OK: save_decision + save_cursor round-trip works')

os.remove(tmp_state)
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Queue module functions validate against fixture"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Queue module functions failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 4. Validate re-audit reload preserves decisions
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Re-audit reload preserves decisions"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local queue = require('schemahelper_queue')
local fixture = '${FIXTURE_DIR}'
local state = queue.load_state(fixture .. '/schemahelper_acuranzo_sqlite.json')

local built = queue.build({
    out_dir = fixture,
    track = 'both',
    state = state,
})

local found_accepted = false
for _, f in ipairs(built.findings) do
    if f.id == 'meta:drift:1148:1003:name' and f.action == 'accepted' then
        found_accepted = true
        break
    end
end

if not found_accepted then
    print('ERR: accepted finding not found in findings')
    os.exit(1)
end

if built.totals.accepted < 1 then
    print('ERR: accepted count too low')
    os.exit(1)
end

print('OK: accepted findings persist across rebuild, accepted=' .. built.totals.accepted)
print('OK: subject count=' .. built.totals.subject)
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Decisions persist across re-audit"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Decision persistence failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 5. Validate pure module: schemahelper_qutil (text/JSON helpers)
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "qutil pure module functions"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local qutil = require('schemahelper_qutil')

local ok = true
local errs = {}

local required = {
    'json_escape', 'file_exists', 'write_all', 'json_string_field',
    'json_num_field', 'json_subobj', 'listed_fields', 'payload_text',
    'payload_raw', 'payload_field_differs', 'has_embed', 'split_lines',
    'first_diff_at', 'pad_clip', 'wrap_hard', 'clip_text', 'jq_lines',
}
for _, fn in ipairs(required) do
    if type(qutil[fn]) ~= 'function' then
        errs[#errs+1] = 'qutil.' .. fn .. ' is not a function'
        ok = false
    end
end

-- has_embed: no false positive on plain SQL
if qutil.has_embed('SELECT 1 FROM users') then
    errs[#errs+1] = 'has_embed false positive on plain SQL'
    ok = false
end
-- has_embed: detects brotli pattern (lowercase)
if not qutil.has_embed("brotli_decompress(FROM_BASE64('abc'))") then
    errs[#errs+1] = 'has_embed failed to detect brotli'
    ok = false
end

-- json_escape: double-quote escaping
local esc = qutil.json_escape('hello "world"')
if not esc:find('\\"', 1, true) then
    errs[#errs+1] = 'json_escape did not escape quotes: ' .. esc
    ok = false
end

-- json_escape: backslash escaping (use string.char to avoid heredoc \\ -> \ conversion)
local bs_input = string.char(0x61, 0x5c, 0x62)
local bs_expected = string.char(0x61, 0x5c, 0x5c, 0x62)
if qutil.json_escape(bs_input) ~= bs_expected then
    errs[#errs+1] = 'json_escape did not double-escape backslash'
    ok = false
end

-- json_string_field on raw JSON
local state_json = '{"design":"acuranzo","engine":"sqlite","cursor_id":"meta:drift:1148:1003:name"}'
local design = qutil.json_string_field(state_json, 'design')
if design ~= 'acuranzo' then
    errs[#errs+1] = 'json_string_field design=' .. tostring(design)
    ok = false
end

-- json_num_field
local ver = qutil.json_num_field('{"version":1}', 'version')
if ver ~= 1 then
    errs[#errs+1] = 'json_num_field version=' .. tostring(ver)
    ok = false
end

-- file_exists
local fixture = '${FIXTURE_DIR}'
if not qutil.file_exists(fixture .. '/findings.json') then
    errs[#errs+1] = 'file_exists should be true for findings.json'
    ok = false
end
if qutil.file_exists(fixture .. '/nonexistent.json') then
    errs[#errs+1] = 'file_exists false positive'
    ok = false
end

-- split_lines
local lines = qutil.split_lines('a\nb\nc')
if #lines ~= 3 or lines[1] ~= 'a' or lines[3] ~= 'c' then
    errs[#errs+1] = 'split_lines failed'
    ok = false
end

-- first_diff_at (expects string arguments)
if qutil.first_diff_at('ab', 'ac') ~= 2 then
    errs[#errs+1] = 'first_diff_at expected 2'
    ok = false
end
if qutil.first_diff_at('abc', 'abc') ~= nil then
    errs[#errs+1] = 'first_diff_at expected nil for equal'
    ok = false
end
if qutil.first_diff_at('abc', 'ab') ~= 3 then
    errs[#errs+1] = 'first_diff_at expected 3 for length diff'
    ok = false
end

-- payload_raw
local raw = qutil.payload_raw('{"name":"x"}', 'name')
if raw ~= 'x' then
    errs[#errs+1] = 'payload_raw name=' .. tostring(raw)
    ok = false
end

-- listed_fields
local fields = qutil.listed_fields('{"fields":["code","name"]}')
if #fields ~= 2 or fields[1] ~= 'code' then
    errs[#errs+1] = 'listed_fields failed'
    ok = false
end

if #errs > 0 then
    for _, e in ipairs(errs) do print('ERR: ' .. e) end
    os.exit(1)
end
print('OK: all qutil function checks passed (' .. #required .. ' functions)')
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "qutil pure module functions validate"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "qutil module checks failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 6. Validate pure module: schemahelper_packet (ref reservation)
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "packet pure module functions"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local packet = require('schemahelper_packet')
local fixture = '${SCHEMAGUI}/test/fixtures/sample_project'
local migrations = fixture .. '/migrations'
local ok = true
local errs = {}

local required = {
    'packet_name', 'packet_path', 'in_git_tree', 'list_reserved',
    'scan_refs', 'next_ref', 'collision', 'suggested_sql', 'write', 'promote',
}
for _, fn in ipairs(required) do
    if type(packet[fn]) ~= 'function' then
        errs[#errs+1] = 'packet.' .. fn .. ' is not a function'
        ok = false
    end
end

-- packet_name format
local pname = packet.packet_name('acuranzo', 'sqlite', 1099)
if pname ~= 'schemahelper_acuranzo_sqlite_1099' then
    errs[#errs+1] = 'packet_name=' .. tostring(pname)
    ok = false
end

-- packet_path format
local ppath = packet.packet_path(fixture, 'acuranzo', 'sqlite', 1099)
if ppath ~= fixture .. '/schemahelper_acuranzo_sqlite_1099' then
    errs[#errs+1] = 'packet_path=' .. tostring(ppath)
    ok = false
end

-- scan_refs finds fixture migrations (max ref should be >= 1000)
local scan = packet.scan_refs({
    migrations = migrations,
    design = 'acuranzo',
    engine = 'sqlite',
    packet_dir = fixture,
})
if scan.max_ref < 1000 then
    errs[#errs+1] = 'scan_refs max_ref=' .. tostring(scan.max_ref)
    ok = false
end
print('OK: scan_refs max_ref=' .. scan.max_ref)

-- next_ref should be max+1
local nextRef = packet.next_ref({
    migrations = migrations,
    design = 'acuranzo',
    engine = 'sqlite',
    packet_dir = fixture,
})
if type(nextRef) ~= 'number' or nextRef <= scan.max_ref then
    errs[#errs+1] = 'next_ref=' .. tostring(nextRef) .. ' expected > ' .. scan.max_ref
    ok = false
end
print('OK: next_ref=' .. nextRef)

-- collision: unused ref returns nil + dest path
local collision_result, dest = packet.collision({
    migrations = migrations,
    design = 'acuranzo',
    engine = 'sqlite',
    packet_dir = fixture,
}, 99999)
if collision_result ~= nil then
    errs[#errs+1] = 'collision should be nil for unused ref, got ' .. tostring(collision_result)
    ok = false
end

-- collision: invalid ref
local inv = packet.collision({
    migrations = migrations,
    design = 'acuranzo',
    engine = 'sqlite',
    packet_dir = fixture,
}, 0)
if inv ~= 'invalid ref' then
    errs[#errs+1] = 'collision invalid ref expected, got ' .. tostring(inv)
    ok = false
end

-- list_reserved: fixture has no reserved dirs (returns table)
local reserved = packet.list_reserved(fixture, 'acuranzo', 'sqlite')
if type(reserved) ~= 'table' then
    errs[#errs+1] = 'list_reserved not table'
    ok = false
end
print('OK: list_reserved count=' .. #reserved)

if #errs > 0 then
    for _, e in ipairs(errs) do print('ERR: ' .. e) end
    os.exit(1)
end
print('OK: all packet function checks passed (' .. #required .. ' functions)')
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "packet pure module functions validate"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "packet module checks failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 7. Validate pure module: schemahelper_apply (apply logic)
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "apply pure module functions"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local apply = require('schemahelper_apply')
local ok = true
local errs = {}

local required = {
    'qualify_queries', 'qualify_table', 'confirm_token', 'refuse_reason',
    'can_apply', 'field_literal', 'build_catalog_sql', 'build_sql', 'write_log',
}
for _, fn in ipairs(required) do
    if type(apply[fn]) ~= 'function' then
        errs[#errs+1] = 'apply.' .. fn .. ' is not a function'
        ok = false
    end
end

-- refuse_reason: no allow_write
local finding = { id='cat:accounts:id:nullable', class='catalog', kind='nullable',
  object='accounts', column='id', ref=1148 }
local refuse = apply.refuse_reason(finding, false)
if refuse ~= 'need --allow-write' then
    errs[#errs+1] = 'refuse_reason no-write=' .. tostring(refuse)
    ok = false
end

-- can_apply: no allow_write => false
local can = apply.can_apply(finding, false)
if can then
    errs[#errs+1] = 'can_apply should be false without allow_write'
    ok = false
end

-- refuse_reason: non-metadata field with allow_write
local non_meta = { id='cat:accounts:foo', class='content drift', field='foo', ref=1148 }
local refuse2 = apply.refuse_reason(non_meta, true)
if refuse2 ~= 'not a metadata field' then
    errs[#errs+1] = 'refuse_reason non-meta=' .. tostring(refuse2)
    ok = false
end

-- can_apply: valid metadata field with allow_write (nullable kind)
local can2 = apply.can_apply(finding, true)
if type(can2) ~= 'boolean' then
    errs[#errs+1] = 'can_apply returned non-boolean'
    ok = false
end

-- refuse_reason: orphaned ref
local orphan = { id='cat:orphans:1290', kind='orphan', ref=1290 }
local refuse3 = apply.refuse_reason(orphan, true)
if refuse3 ~= nil then
    errs[#errs+1] = 'refuse_reason orphan should be nil, got ' .. tostring(refuse3)
    ok = false
end

-- confirm_token: catalog finding with column
local tok = apply.confirm_token(finding)
if tok ~= 'accounts.id' then
    errs[#errs+1] = 'confirm_token=' .. tostring(tok)
    ok = false
end

-- field_literal: sqlite
local fl = apply.field_literal('sqlite', 'hello')
if type(fl) ~= 'string' or fl == '' then
    errs[#errs+1] = 'field_literal sqlite empty'
    ok = false
end

-- qualify_table
local qt = apply.qualify_table('sqlite', '', 'accounts')
if qt ~= 'accounts' then
    errs[#errs+1] = 'qualify_table sqlite=' .. tostring(qt)
    ok = false
end

if #errs > 0 then
    for _, e in ipairs(errs) do print('ERR: ' .. e) end
    os.exit(1)
end
print('OK: all apply function checks passed (' .. #required .. ' functions)')
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "apply pure module functions validate"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "apply module checks failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 8. Validate pure module: schemahelper_connect (wrapper parsing)
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "connect pure module functions"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local connect = require('schemahelper_connect')
local fixture = '${FIXTURE_DIR}'
local ok = true
local errs = {}

local required = { 'parse_wrapper', 'resolve', 'probe', 'exec_sql' }
for _, fn in ipairs(required) do
    if type(connect[fn]) ~= 'function' then
        errs[#errs+1] = 'connect.' .. fn .. ' is not a function'
        ok = false
    end
end

-- parse_wrapper on fixture wrapper (uses env exports, not CLI flags)
local flags, exports = connect.parse_wrapper(fixture .. '/schematool_sqlite_fixture.sh')
if type(flags) ~= 'table' then
    errs[#errs+1] = 'parse_wrapper flags not table'
    ok = false
end
if type(exports) ~= 'table' then
    errs[#errs+1] = 'parse_wrapper exports not table'
    ok = false
end
if exports.SCHEMATOOL_DB_ENGINE ~= 'sqlite' then
    errs[#errs+1] = 'parse_wrapper exports.SCHEMATOOL_DB_ENGINE=' .. tostring(exports.SCHEMATOOL_DB_ENGINE)
    ok = false
end
print('OK: parse_wrapper exports engine=' .. tostring(exports.SCHEMATOOL_DB_ENGINE))

-- resolve returns conn table with expected fields
local conn = connect.resolve(fixture .. '/schematool_sqlite_fixture.sh')
if type(conn) ~= 'table' then
    errs[#errs+1] = 'resolve not table'
    ok = false
end
if type(conn.ok) ~= 'boolean' then
    errs[#errs+1] = 'resolve ok not boolean'
    ok = false
end
if not conn.engine or conn.engine == '' then
    errs[#errs+1] = 'resolve engine empty'
    ok = false
end
print('OK: resolve engine=' .. conn.engine .. ' family=' .. conn.family)

-- parse_wrapper on non-existent file returns empty tables
local flags2, exports2 = connect.parse_wrapper(fixture .. '/does_not_exist.sh')
if type(flags2) ~= 'table' or type(exports2) ~= 'table' then
    errs[#errs+1] = 'parse_wrapper missing file should return tables'
    ok = false
end

if #errs > 0 then
    for _, e in ipairs(errs) do print('ERR: ' .. e) end
    os.exit(1)
end
print('OK: all connect function checks passed (' .. #required .. ' functions)')
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "connect pure module functions validate"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "connect module checks failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 9. Validate luacheck passes on all schemahelper files
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "luacheck clean on schemahelper files"

LUACHECK_OK=1
LUACHECK_FILES=(
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

for f in "${LUACHECK_FILES[@]}"; do
    full="${SCHEMAGUI}/${f}"
    if [[ -f "${full}" ]]; then
        output=$(luacheck --std=max --max-line-length=120 --ignore 542,561 --no-self --no-unused-args --formatter=plain "${full}" 2>&1 || true)
        if [[ -n "${output}" ]] && ! echo "${output}" | grep -qi "^No errors"; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "luacheck issues in ${f}:"
            while IFS= read -r line; do
                [[ -z "${line}" ]] && continue
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${line}"
            done <<< "${output}"
            LUACHECK_OK=0
        fi
    else
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Cannot find ${f}"
        LUACHECK_OK=0
    fi
done

if [[ "${LUACHECK_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#LUACHECK_FILES[@]} schemahelper files pass luacheck"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "luacheck issues found"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 10. Validate decode_embedded across all dialects (SQLite, MySQL, DB2, PG)
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "decode_embedded all dialects"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
package.cpath = package.cpath .. ";/usr/local/lib/lua/5.5/?.so;/home/asimard/.luarocks/lib/lua/5.5/?.so"
local queue = require('schemahelper_queue')

local test_plain = 'hello world'
local b64_blob = 'DwWAaGVsbG8gd29ybGQD'
local plain_b64 = 'dGVzdA=='
local ok = true
local errs = {}

-- MySQL lowercase brotli
local r = queue.decode_embedded("brotli_decompress(FROM_BASE64('" .. b64_blob .. "'))")
if r ~= test_plain then errs[#errs+1] = 'MySQL lowercase brotli: got ' .. tostring(r); ok = false end

-- MySQL uppercase brotli
r = queue.decode_embedded("BROTLI_DECOMPRESS(FROM_BASE64('" .. b64_blob .. "'))")
if r ~= test_plain then errs[#errs+1] = 'MySQL uppercase brotli failed'; ok = false end

-- SQLite brotli
r = queue.decode_embedded("BROTLI_DECOMPRESS(CRYPTO_DECODE('" .. b64_blob .. "'))")
if r ~= test_plain then errs[#errs+1] = 'SQLite brotli failed'; ok = false end

-- DB2 brotli
r = queue.decode_embedded("myschema.BROTLI_DECOMPRESS(myschema.BASE64DECODEBINARY('" .. b64_blob .. "'))")
if r ~= test_plain then errs[#errs+1] = 'DB2 brotli failed'; ok = false end

-- PostgreSQL brotli
r = queue.decode_embedded("brotli_decompress(DECODE('" .. b64_blob .. "', 'base64'))")
if r ~= test_plain then errs[#errs+1] = 'PostgreSQL brotli failed'; ok = false end

-- PostgreSQL CONVERT_FROM
r = queue.decode_embedded("CONVERT_FROM(DECODE('" .. plain_b64 .. "', 'base64'), 'UTF8')")
if r ~= 'test' then errs[#errs+1] = 'PostgreSQL CONVERT_FROM failed'; ok = false end

-- Standalone base64
r = queue.decode_embedded("CRYPTO_DECODE('" .. plain_b64 .. "')")
if r ~= 'test' then errs[#errs+1] = 'SQLite CRYPTO_DECODE failed'; ok = false end
r = queue.decode_embedded("BASE64DECODE('" .. plain_b64 .. "')")
if r ~= 'test' then errs[#errs+1] = 'DB2 BASE64DECODE failed'; ok = false end
r = queue.decode_embedded("FROM_BASE64('" .. plain_b64 .. "')")
if r ~= 'test' then errs[#errs+1] = 'MySQL FROM_BASE64 failed'; ok = false end

-- has_embed detection
if not queue.has_embed("brotli_decompress(FROM_BASE64('" .. b64_blob .. "'))") then errs[#errs+1] = 'has_embed MySQL lowercase failed'; ok = false end
if not queue.has_embed("BROTLI_DECOMPRESS(BASE64DECODEBINARY('" .. b64_blob .. "'))") then errs[#errs+1] = 'has_embed DB2 failed'; ok = false end

-- No false positives
if queue.has_embed('SELECT 1 FROM users') then errs[#errs+1] = 'has_embed false positive on plain SQL'; ok = false end

if not ok then
    for _, e in ipairs(errs) do print('ERR: ' .. e) end
    os.exit(1)
end
print('OK: all decode patterns verified (SQLite, MySQL upper+lower, DB2, PostgreSQL)')
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All decode patterns work"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Decode pattern failures"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 11. Validate schemahelper.sh --help and --version (single result)
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "schemahelper.sh --help / --version"

SCHEMAGUI_SH="${SCHEMAGUI}/schemahelper.sh"
help_ok=false
SCH_HELP_OUT=$("${SCHEMAGUI_SH}" --help 2>&1) || true
if echo "${SCH_HELP_OUT}" | grep -qi "usage\|help\|schemahelper"; then
    help_ok=true
fi

VER_OUT=$("${SCHEMAGUI_SH}" --version 2>&1 || true)
ver_ok=false
if echo "${VER_OUT}" | grep -q "SchemaHelper"; then
    ver_ok=true
fi

if ${help_ok} && ${ver_ok}; then
    print_multi_output "${VER_OUT}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "schemahelper.sh --help/--version work"
else
    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "help_ok=${help_ok} ver_ok=${ver_ok}"
    print_multi_output "${VER_OUT}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "schemahelper.sh --help/version failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 12. Validate schemahelper.lua --version prints version info
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "schemahelper.lua --version"

SCHEMALUA="${SCHEMAGUI}/schemahelper.lua"
LUAV_OUT=$(lua "${SCHEMALUA}" --version 2>&1 || true)
if echo "${LUAV_OUT}" | grep -q "0.5"; then
    print_multi_output "${LUAV_OUT}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "SchemaHelper --version works"
else
    print_multi_output "${LUAV_OUT}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Version not found or wrong format"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 13. Validate terminal.lua TextPanel is loadable
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "terminal.lua TextPanel available"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local ok, TextPanel = pcall(require, 'terminal.ui.panel.text')
if not ok then
    print('ERR: cannot require terminal.ui.panel.text')
    os.exit(1)
end
print('OK: TextPanel loaded: ' .. tostring(TextPanel))
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "TextPanel loaded from terminal.lua"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "TextPanel not available"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# 14. Validate dashboard_content renderer against fixture (no TTY / no live DB)
# ---------------------------------------------------------------------------
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Dashboard renderer (fixture, headless)"

run_lua <<LUAEOF
package.path = "${SCHEMAGUI}/lua/?.lua;" .. package.path
local t = require('terminal')
t.width = 80
t.height = 24
t.output.write = function() end
t.cursor.position.set = function() end
t.text.push_seq = function() return nil end
t.text.pop_seq = function() return nil end
t.text.width.utf8swidth = function(s) return #tostring(s or '') end
t.text.width.truncate_ellipsis = function(w, s, _) return tostring(s or ''):sub(1, w) end

local S = require('schemahelper_screens')
local Q = require('schemahelper_queue')
local packet = require('schemahelper_packet')

assert(type(packet.list_reserved) == 'function', 'packet.list_reserved is not a function')
assert(type(Q.list_reserved) == 'nil', 'queue.list_reserved should be nil')

local fixture = '${SCHEMAGUI}/test/fixtures/sample_project'
local state = Q.load_state(fixture .. '/schemahelper_acuranzo_sqlite.json')
local app = { conn = nil, log = '(none)', state = state, built = nil,
  warn_in_repo = false, catalog_degraded = false, show_mode_msg = '' }
local opts = { wrapper = '(none)', out_dir = fixture, work_dir = fixture, state_file = '(none)',
  track = 'both', migrations = fixture .. '/migrations', schematool = '(none)',
  design = 'acuranzo', engine = 'sqlite', packet_dir = fixture, allow_write = true,
  ref = 0, lua_version = '5.5' }
local self_panel = { opts = opts, app = app, inner_row = 1, inner_col = 1,
  inner_height = 24, inner_width = 80 }

local ok, err = pcall(S.dashboard_content, self_panel)
if ok then
  print('OK: dashboard_content rendered without error')
else
  print('ERR: ' .. tostring(err))
  os.exit(1)
end
os.exit(0)
LUAEOF

if [[ "${LUAP_OK}" -eq 1 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Dashboard renderer exercises packet.list_reserved"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Dashboard renderer failed (list_reserved regression?)"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# Cleanup fixture artifacts created during testing
# ---------------------------------------------------------------------------
if [[ -f "${FIXTURE_DIR}/.test_roundtrip_state.json" ]]; then
    rm -f "${FIXTURE_DIR}/.test_roundtrip_state.json"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
