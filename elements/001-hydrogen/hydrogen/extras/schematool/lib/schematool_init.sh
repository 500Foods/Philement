#!/usr/bin/env bash
# SchemaTool initialization — dependency checks and command path resolution
#
# This library is sourced by schematool.sh. It verifies that the required
# external tools (tables, jq, lua) are available and resolves their full
# paths into global variables consumed by the other libraries and the
# main script.
#
# CHANGELOG
# 1.0.0 - 2026-08-02 - Split from schematool.sh

# shellcheck disable=SC2034,SC2154 # globals set here, consumed by schematool.sh and lib modules

set -euo pipefail

schematool_check_deps() {
    if ! command -v tables >/dev/null 2>&1; then
        echo "Error: 'tables' command not found" >&2
        exit 1
    fi
    if ! command -v jq >/dev/null 2>&1; then
        echo "Error: 'jq' command not found" >&2
        exit 1
    fi
    if ! command -v lua >/dev/null 2>&1; then
        echo "Error: 'lua' command not found" >&2
        exit 1
    fi
}

schematool_resolve_commands() {
    TABLES="$(command -v tables)"
    JQ="$(command -v jq)"
    LUA="$(command -v lua)"
    DATE_CMD="$(command -v gdate 2>/dev/null || command -v date)"
}
