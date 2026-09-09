#!/usr/bin/env bash

# Test: SchemaHelper Phase 72 Integration
# Exercise schemahelper_queue.lua, schemahelper_packet.lua,
# schemahelper_apply.lua, schemahelper_connect.lua and
# schemahelper_qutil.lua against checked-in fixture findings
# (no live DB required; brotli-only paths guarded).
# Lua fixtures live in tests/lib/schemahelper/.

# FUNCTIONS
# (Helpers live in tests/lib/schemahelper_helpers.sh)

# CHANGELOG
# 1.2.0 - 2026-09-09 - Split Lua fixtures + helpers under tests/lib for 1000-line cap
# 1.1.6 - 2026-09-09 - Accept hash invalidation + un-accept
# 1.1.5 - 2026-09-08 - Progress bar dark-grey background
# 1.1.4 - 2026-09-08 - Eighths width + fake-log issue tint
# 1.1.3 - 2026-09-08 - Picker env-name blurbs (no FAMILY_*)
# 1.1.2 - 2026-09-08 - Instance label/value split; 0.6.1
# 1.1.1 - 2026-09-08 - Instance block headless check; version 0.6
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
TEST_VERSION="1.2.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/schemahelper_helpers.sh # Split for the 1000-line cap
source "$(dirname "${BASH_SOURCE[0]}")/lib/schemahelper_helpers.sh"

schemahelper_setup_lua_path

schemahelper_check_fixtures
schemahelper_check_lua

schemahelper_lua_subtest \
    "Queue module functions and fixture validation" \
    "queue.lua" \
    "Queue module functions validate against fixture" \
    "Queue module functions failed"

schemahelper_lua_subtest \
    "Re-audit reload preserves decisions" \
    "reaudit.lua" \
    "Decisions persist across re-audit" \
    "Decision persistence failed"

schemahelper_lua_subtest \
    "qutil pure module functions" \
    "qutil.lua" \
    "qutil pure module functions validate" \
    "qutil module checks failed"

schemahelper_lua_subtest \
    "packet pure module functions" \
    "packet.lua" \
    "packet pure module functions validate" \
    "packet module checks failed"

schemahelper_lua_subtest \
    "apply pure module functions" \
    "apply.lua" \
    "apply pure module functions validate" \
    "apply module checks failed"

schemahelper_lua_subtest \
    "connect pure module functions" \
    "connect.lua" \
    "connect pure module functions validate" \
    "connect module checks failed"

schemahelper_luacheck

schemahelper_lua_subtest \
    "decode_embedded all dialects" \
    "decode.lua" \
    "All decode patterns work" \
    "Decode pattern failures"

schemahelper_cli_help
schemahelper_lua_version

schemahelper_lua_subtest \
    "terminal.lua TextPanel available" \
    "textpanel.lua" \
    "TextPanel loaded from terminal.lua" \
    "TextPanel not available"

schemahelper_lua_subtest \
    "Dashboard renderer (fixture, headless)" \
    "dashboard.lua" \
    "Dashboard renderer exercises packet.list_reserved" \
    "Dashboard renderer failed (list_reserved regression?)"

schemahelper_lua_subtest \
    "Instance block (picker highlight)" \
    "instance.lua" \
    "Instance block follows highlight without connect" \
    "Instance block assertions failed"

schemahelper_lua_subtest \
    "Picker env-name blurbs" \
    "picker.lua" \
    "Picker blurbs name HOST/USER/PASS/NAME" \
    "Picker blurb assertions failed"

schemahelper_lua_subtest \
    "Eighths bar and issue tint" \
    "eighths.lua" \
    "Eighths width 48; drifted cell tints" \
    "Eighths / issue-tint assertions failed"

schemahelper_lua_subtest \
    "Accept hash + un-accept" \
    "hash.lua" \
    "Hash mismatch re-queues; un-accept restores" \
    "Accept hash / un-accept failed"

schemahelper_cleanup

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
