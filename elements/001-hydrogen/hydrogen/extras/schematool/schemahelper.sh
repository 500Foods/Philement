#!/usr/bin/env bash

# SchemaHelper — Interactive SchemaTool front-end
# Lua 5.5 TUI over extras/schematool. Default is review-only.
#
# CHANGELOG
# 0.5.8 - 2026-08-25 - Per-run /tmp work-dir for intermediates; --work-dir/--keep-work-dir
# 0.5.7 - 2026-08-24 - Mouseover highlighting on clickable options; click on release; capitalized option labels
# 0.5.6 - 2026-08-24 - Mouse support: SGR 1006; click wrapper rows and click [key] actions
# 0.5.5 - 2026-08-24 - Phase 7: catalog DDL apply (nullable / add column) + [m] promote packet to Helium
# 0.5.4 - 2026-08-24 - Phase 5 slice: confirmed orphan [u] DELETE (true orphans only)
# 0.5.3 - 2026-08-24 - Dashboard/review [r] re-runs SchemaTool
# 0.5.2 - 2026-08-24 - Dashboard: findings for review, not migrations
# 0.5.1 - 2026-08-23 - [u] labeled update; catalog review shows fold ref
# 0.5.0 - 2026-08-23 - Phase 5: --allow-write enables one-field apply
# 0.4.14 - 2026-08-23 - Explore: Enter decodes brotli line; pageup/pagedown
# 0.4.13 - 2026-08-23 - Explore: full field, both line nos, first-diff line
# 0.4.12 - 2026-08-23 - Review: do not use Lua patterns to drop key lines
# 0.4.11 - 2026-08-23 - Explore panes, red rules, highlight, brotli decode
# 0.4.10 - 2026-08-23 - Explore: Migration vs Database; ignore 1000→1003
# 0.4.9 - 2026-08-23 - Explore: chrome scroll + field diff, no screen.body
# 0.4.8 - 2026-08-23 - Source wrapper to resolve computed connect flags
# 0.4.7 - 2026-08-23 - Result screen does not crash on tabbed log lines
# 0.4.6 - 2026-08-23 - Dashboard q exits without keys.r crash
# 0.4.5 - 2026-08-23 - Connect ping reads wrapper --engine and connect flags
# 0.4.4 - 2026-08-23 - Catalog fold failure keeps metadata dashboard
# 0.4.3 - 2026-08-23 - Progress bar while SchemaTool runs
# 0.4.2 - 2026-08-23 - Keep /usr/local Lua 5.5 cpath so SchemaTool finds brotli
# 0.4.1 - 2026-08-23 - Failed connect returns to wrapper picker
# 0.4.0 - 2026-08-23 - Phase 4: --ref for forced packet numbers
# 0.2.2 - 2026-08-23 - Session header + live connect probe
# 0.2.1 - 2026-08-23 - Persistent chrome; wrapper picker inset at top
# 0.2.0 - 2026-08-23 - Phase 1: flags, wrapper, --track, --reuse, invoke
# 0.1.0 - 2026-08-23 - Launcher + splash; wrapper arg is Database Comparator

set -euo pipefail

# shellcheck disable=SC2154 # HELIUM_ROOT may be set by env
SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_DIR="$(cd "$(dirname "${SCRIPT_PATH}")" && pwd)"
LUA_APP="${SCRIPT_DIR}/schemahelper.lua"
SCHEMATOOL_SH="${SCRIPT_DIR}/schematool.sh"

VERSION="0.5.8"
SCHEMATOOL_VERSION="1.9.0"

print_help() {
    cat <<EOF
SchemaHelper — Interactive SchemaTool front-end

Usage:
  schemahelper.sh [wrapper]
  schemahelper.sh --wrapper PATH [options]
  schemahelper.sh --help
  schemahelper.sh --version

wrapper is a SchemaTool engine script (credentials + engine). Examples:
  schemahelper.sh schematool_db2.sh
  schemahelper.sh "${SCRIPT_DIR}/schematool_postgresql.sh"

If wrapper is omitted, the TUI lists extras/schematool/schematool_*.sh
after the splash.

Options:
  --wrapper PATH         SchemaTool wrapper (same as positional)
  --migrations DIR       Override Helium migrations directory
  --out-dir DIR          SchemaTool workspace (default: directory of wrapper)
  --work-dir DIR         Use DIR for intermediate JSON/detail/log files
                         (default: /tmp/schemahelper-<timestamp>-<rand>)
  --state-file PATH      Sidecar JSON (default: <out-dir>/schemahelper_<design>_<engine>.json)
  --packet-dir DIR       Packet workspace (default: same as --out-dir)
  --ref N                Force packet ref instead of max(disk, reserved)+1
  --track metadata|catalog|both
                          Which SchemaTool track to queue (default: both)
  --reuse                Load existing --out-dir artifacts; skip SchemaTool
  --allow-write          Enable [U]pdate Database: apply metadata change
                          (type REF.field), delete orphan ref (type REF; true
                          orphans only), apply catalog DDL on nullable/
                          add-column findings (type object.column), and
                          [M] Promote a packet stub into Helium migrations
  --keep-work-dir        Do not remove the auto-generated work-dir on exit
  --help, -h             This help
  --version              Print versions

Migrations default to the same Helium acuranzo tree the wrappers use
(HELIUM_ROOT/acuranzo/migrations, else the repo 002-helium path).

Requires Lua 5.5 and the terminal rock:
  luarocks --lua-version=5.5 install terminal

Docs: /docs/H/plans/SCHEMAHELPER.md
EOF
}

resolve_wrapper() {
    local raw="$1"
    local dir
    if [[ -z "${raw}" ]]; then
        return 0
    fi
    if [[ -x "${raw}" ]]; then
        dir="$(cd "$(dirname "${raw}")" && pwd)"
        printf '%s/%s\n' "${dir}" "$(basename "${raw}")"
        return 0
    fi
    if [[ -x "${SCRIPT_DIR}/${raw}" ]]; then
        printf '%s/%s\n' "${SCRIPT_DIR}" "${raw}"
        return 0
    fi
    if [[ "${raw}" != schematool_* && -x "${SCRIPT_DIR}/schematool_${raw}.sh" ]]; then
        printf '%s/schematool_%s.sh\n' "${SCRIPT_DIR}" "${raw}"
        return 0
    fi
    return 0
}

resolve_migrations() {
    local override="$1"
    local candidate
    if [[ -n "${override}" ]]; then
        if [[ ! -d "${override}" ]]; then
            echo "Error: migrations directory not found: ${override}" >&2
            exit 1
        fi
        (cd "${override}" && pwd)
        return 0
    fi
    if [[ -n "${HELIUM_ROOT:-}" && -d "${HELIUM_ROOT}/acuranzo/migrations" ]]; then
        (cd "${HELIUM_ROOT}/acuranzo/migrations" && pwd)
        return 0
    fi
    candidate="${SCRIPT_DIR}/../../../../002-helium/acuranzo/migrations"
    if [[ -d "${candidate}" ]]; then
        (cd "${candidate}" && pwd)
        return 0
    fi
    return 0
}

resolve_dir() {
    local override="$1"
    if [[ -z "${override}" ]]; then
        return 0
    fi
    mkdir -p "${override}"
    (cd "${override}" && pwd)
}

if [[ ! -f "${LUA_APP}" ]]; then
    echo "Error: ${LUA_APP} not found" >&2
    exit 1
fi

        WRAPPER_ARG=""
MIGRATIONS_ARG=""
OUT_DIR_ARG=""
WORK_DIR_ARG=""
STATE_FILE_ARG=""
PACKET_DIR_ARG=""
TRACK_ARG="both"
REF_ARG=""
REUSE_ARG=0
ALLOW_WRITE_ARG=0
KEEP_WORK_DIR_ARG=0

while [[ $# -gt 0 ]]; do
    case "${1}" in
        --help|-h)
            print_help
            exit 0
            ;;
        --version)
            echo "SchemaHelper ${VERSION} (SchemaTool ${SCHEMATOOL_VERSION})"
            exit 0
            ;;
        --wrapper)
            if [[ $# -lt 2 ]]; then
                echo "Error: --wrapper requires a path" >&2
                exit 1
            fi
            WRAPPER_ARG="${2}"
            shift 2
            ;;
        --migrations)
            if [[ $# -lt 2 ]]; then
                echo "Error: --migrations requires a directory" >&2
                exit 1
            fi
            MIGRATIONS_ARG="${2}"
            shift 2
            ;;
        --out-dir)
            if [[ $# -lt 2 ]]; then
                echo "Error: --out-dir requires a directory" >&2
                exit 1
            fi
            OUT_DIR_ARG="${2}"
            shift 2
            ;;
        --work-dir)
            if [[ $# -lt 2 ]]; then
                echo "Error: --work-dir requires a directory" >&2
                exit 1
            fi
            WORK_DIR_ARG="${2}"
            shift 2
            ;;
        --state-file)
            if [[ $# -lt 2 ]]; then
                echo "Error: --state-file requires a path" >&2
                exit 1
            fi
            STATE_FILE_ARG="${2}"
            shift 2
            ;;
        --packet-dir)
            if [[ $# -lt 2 ]]; then
                echo "Error: --packet-dir requires a directory" >&2
                exit 1
            fi
            PACKET_DIR_ARG="${2}"
            shift 2
            ;;
        --ref)
            if [[ $# -lt 2 ]]; then
                echo "Error: --ref requires a positive integer" >&2
                exit 1
            fi
            if [[ ! "${2}" =~ ^[1-9][0-9]*$ ]]; then
                echo "Error: --ref must be a positive integer" >&2
                exit 1
            fi
            REF_ARG="${2}"
            shift 2
            ;;
        --track)
            if [[ $# -lt 2 ]]; then
                echo "Error: --track requires metadata|catalog|both" >&2
                exit 1
            fi
            TRACK_ARG="${2}"
            shift 2
            ;;
        --reuse)
            REUSE_ARG=1
            shift
            ;;
        --allow-write)
            ALLOW_WRITE_ARG=1
            shift
            ;;
        --keep-work-dir)
            KEEP_WORK_DIR_ARG=1
            shift
            ;;
        --)
            shift
            break
            ;;
        -*)
            echo "Error: unknown option ${1}" >&2
            exit 1
            ;;
        *)
            if [[ -n "${WRAPPER_ARG}" ]]; then
                echo "Error: unexpected argument ${1}" >&2
                exit 1
            fi
            WRAPPER_ARG="${1}"
            shift
            ;;
    esac
done

case "${TRACK_ARG}" in
    metadata|catalog|both) ;;
    *)
        echo "Error: --track must be metadata, catalog, or both" >&2
        exit 1
        ;;
esac

if ! command -v lua >/dev/null 2>&1; then
    echo "Error: lua not found on PATH" >&2
    exit 1
fi

LUA_VER="$(lua -e 'print(_VERSION)' 2>/dev/null || true)"
if [[ "${LUA_VER}" != "Lua 5.5"* ]]; then
    echo "Error: SchemaHelper requires Lua 5.5 (found: ${LUA_VER:-none})" >&2
    exit 1
fi

if command -v luarocks >/dev/null 2>&1; then
    eval "$(luarocks --lua-version=5.5 path --bin 2>/dev/null || true)"
fi
# luarocks path replaces compiled-in defaults. SchemaTool expect needs
# /usr/local/lib/lua/5.5/brotli.so; terminal stays in the user tree.
LUA_PATH="${LUA_PATH:-}${LUA_PATH:+;}/usr/local/share/lua/5.5/?.lua;/usr/local/share/lua/5.5/?/init.lua"
LUA_CPATH="${LUA_CPATH:-}${LUA_CPATH:+;}/usr/local/lib/lua/5.5/?.so;/usr/local/lib/lua/5.5/loadall.so"
export LUA_PATH LUA_CPATH

if ! lua -e 'require("terminal")' >/dev/null 2>&1; then
    echo "Error: require(\"terminal\") failed." >&2
    echo "Install with: luarocks --lua-version=5.5 install terminal" >&2
    echo "Lua 5.5 supplies utf8; skip the utf8 rock if it fails to compile." >&2
    exit 1
fi

if [[ ! -t 0 || ! -t 1 ]]; then
    echo "Error: SchemaHelper requires a tty" >&2
    exit 1
fi

WRAPPER_ABS=""
if [[ -n "${WRAPPER_ARG}" ]]; then
    WRAPPER_ABS="$(resolve_wrapper "${WRAPPER_ARG}")"
    if [[ -z "${WRAPPER_ABS}" ]]; then
        echo "Error: SchemaTool wrapper not found or not executable: ${WRAPPER_ARG}" >&2
        exit 1
    fi
fi

MIGRATIONS_ABS="$(resolve_migrations "${MIGRATIONS_ARG}")"
OUT_DIR_ABS="$(resolve_dir "${OUT_DIR_ARG}")"
WORK_DIR_ABS="$(resolve_dir "${WORK_DIR_ARG}")"
PACKET_DIR_ABS="$(resolve_dir "${PACKET_DIR_ARG}")"

if [[ -n "${STATE_FILE_ARG}" ]]; then
    STATE_DIR="$(cd "$(dirname "${STATE_FILE_ARG}")" && pwd)"
    STATE_FILE_ARG="${STATE_DIR}/$(basename "${STATE_FILE_ARG}")"
fi

LUA_BANNER="$(lua -v 2>&1 || true)"
LUA_FULL="${LUA_BANNER#Lua }"
LUA_FULL="${LUA_FULL%% *}"

LUA_ARGS=("${LUA_APP}")
if [[ -n "${WRAPPER_ABS}" ]]; then
    LUA_ARGS+=(--wrapper "${WRAPPER_ABS}")
fi
if [[ -n "${MIGRATIONS_ABS}" ]]; then
    LUA_ARGS+=(--migrations "${MIGRATIONS_ABS}")
fi
if [[ -n "${OUT_DIR_ABS}" ]]; then
    LUA_ARGS+=(--out-dir "${OUT_DIR_ABS}")
fi
if [[ -n "${WORK_DIR_ABS}" ]]; then
    LUA_ARGS+=(--work-dir "${WORK_DIR_ABS}")
fi
if [[ -n "${STATE_FILE_ARG}" ]]; then
    LUA_ARGS+=(--state-file "${STATE_FILE_ARG}")
fi
if [[ -n "${PACKET_DIR_ABS}" ]]; then
    LUA_ARGS+=(--packet-dir "${PACKET_DIR_ABS}")
fi
LUA_ARGS+=(--track "${TRACK_ARG}")
if [[ -n "${REF_ARG}" ]]; then
    LUA_ARGS+=(--ref "${REF_ARG}")
fi
if [[ "${REUSE_ARG}" -eq 1 ]]; then
    LUA_ARGS+=(--reuse)
fi
if [[ "${ALLOW_WRITE_ARG}" -eq 1 ]]; then
    LUA_ARGS+=(--allow-write)
fi
if [[ "${KEEP_WORK_DIR_ARG}" -eq 1 ]]; then
    LUA_ARGS+=(--keep-work-dir)
fi
if [[ -f "${SCHEMATOOL_SH}" ]]; then
    LUA_ARGS+=(--schematool "${SCHEMATOOL_SH}")
fi
if [[ -n "${LUA_FULL}" ]]; then
    LUA_ARGS+=(--lua-version "${LUA_FULL}")
fi

exec lua "${LUA_ARGS[@]}"
