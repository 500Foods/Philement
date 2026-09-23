#!/usr/bin/env bash
# Firebird test/demo database creation — hydrogen_test.fdb + hydrogen_demo.fdb
#
# Usage:
#   sudo ./create_test_db.sh                 # default: both (recreate test + demo)
#   sudo ./create_test_db.sh both            # same as default
#   sudo ./create_test_db.sh test            # recreate hydrogen_test.fdb only
#   sudo ./create_test_db.sh demo            # recreate hydrogen_demo.fdb only
#   sudo ./create_test_db.sh /path/one.fdb   # one-off single path (legacy)
#   sudo FIREBIRD_CREATE_MODE=test ./create_test_db.sh
#   sudo ./create_test_db.sh --init-security
#
# Environment (preferred — Andrew / Test 37+40 wiring):
#   FIREBIRD_DB_PATH_TEST  Path for migration/test DB (default: <artifacts>/hydrogen_test.fdb)
#   FIREBIRD_DB_PATH_DEMO  Path for demo/dev DB      (default: <artifacts>/hydrogen_demo.fdb)
#   FIREBIRD_SYSDBA_PASSWORD  SYSDBA password (default: masterkey)
#   FIREBIRD_CREATE_MODE   both|test|demo (overridden by positional mode arg)
#   HYDROGEN_ROOT          Hydrogen tree root (for default artifacts dir)
#
# Deprecated / back-compat:
#   FIREBIRD_DB_PATH (singular) — deprecated. If set while the dual vars are
#   unset, it is treated as a one-off single create target (same as passing a
#   path argument). Prefer FIREBIRD_DB_PATH_TEST + FIREBIRD_DB_PATH_DEMO.
#   When only one of the dual vars is needed by older tools, map TEST as the
#   primary singular stand-in (do not silently point both roles at one file).
#
# Default artifacts dir (when dual env unset):
#   ${HYDROGEN_ROOT}/tests/artifacts/database/firebird/
#   filenames: hydrogen_test.fdb , hydrogen_demo.fdb
#   (replaces old testfb.fdb / demofb.fdb defaults)
#
# Requires: sudo, Firebird SuperServer on 3050, firebird system user.
#
# --init-security: Stop the server, initialize security4.fdb with SYSDBA in
#   embedded mode, restart. Run once (or when security DB is missing/corrupt).
#
# CHANGELOG
# 2.3.2 - 2026-09-23 - Shellcheck: drop unused SERVER; report perms via stat;
#           do not mask ls in the present-file message.
# 2.3.1 - 2026-09-22 - Default both: do not one-off on legacy FIREBIRD_DB_PATH
#           (hydrogen.fdb/testfb.fdb/demofb.fdb); map other singular → TEST and
#           still create DEMO. Explicit path arg still one-off.
# 2.3.0 - 2026-09-22 - Dual DB: FIREBIRD_DB_PATH_TEST + FIREBIRD_DB_PATH_DEMO;
#           modes both|test|demo; defaults hydrogen_test.fdb / hydrogen_demo.fdb
#           under tests/artifacts/database/firebird/; deprecate singular
#           FIREBIRD_DB_PATH (one-off / TEST-primary back-compat).
# 2.2.1 - 2026-09-22 - Fix: bash '#' comment inside isql heredoc prevented CREATE;
#           require .fdb on disk after create; honor PKEXEC_UID for group; open DB_DIR.
# 2.2.0 - 2026-09-22 - After create: chgrp to sudo caller's group, chmod g+rw,o+rw
#           so developers can read/write the .fdb under a repo path without
#           being in the firebird group. PAGE_SIZE default raised to 32768.
# 2.1.2 - 2026-09-21 - Fix: heredoc delimiter <<'SQL' was missing closing quote
#           (should be <<'SQL' or <<SQL). The unterminated quote caused bash
#           to reject the script with "unexpected EOF while looking for matching `'`.
#           Changed to unquoted <<SQL since DB_PATH is already expanded.
# 2.1.1 - 2026-09-21 - Fix: create_script passed ${SERVER} connection string to
#           isql-fb, causing it to try connecting to a non-existent DB before
#           executing CREATE DATABASE. Removed the connection string argument;
#           CREATE DATABASE is now piped directly to isql-fb without a target DB.
#           Fix: systemd-run calls did not propagate FIREBIRD env var to the
#           firebird user's process, causing lock file permission errors.
#           Added -E "FIREBIRD=${FIREBIRD}" to all systemd-run invocations.
# 2.1.0 - 2026-09-21 - Added honoring of FIREBIRD_DB_PATH env var for the database
#           file location (instead of hardcoded /var/lib/firebird/data/${DB_NAME}).
#           The database is created with proper permissions (chown firebird) and
#           the named account (SYSDBA) password is supplied via FIREBIRD_SYSDBA_PASSWORD.
# 2.0.3 - 2026-09-20 - Fix: GRANT syntax was invalid for Firebird (ON * and ON DATABASE
#           both fail). Removed GRANT step entirely - SYSDBA has full privileges
#           by default in Firebird.
# 2.0.2 - 2026-09-20 - Fix: systemd-run was swallowing isql-fb stderr (exit 126, no error
#           output). Added StandardOutput=inherit + StandardError=inherit to all
#           systemd-run calls so errors surface properly. Removed unused
#           as_firebird_isql_stdin helper. Fixed GRANT syntax (ON * → ON DATABASE).
# 2.0.1 - 2026-09-20 - Fix: `local` keyword used at script top-level (outside
#           function) caused "local: can only be used in a function" for
#           both init_script and create_script variables.
# 2.0.0 - 2026-09-20 - Rewritten: all isql-fb operations run as the firebird
#           user via systemd-run to avoid root_squash permission issues.
#           Embedded-mode CREATE DATABASE + CREATE USER for security DB.
#           Network-mode CREATE/DROP DATABASE for test DB.
# 1.4.0 - 2026-09-20 - Added lock dir auto-fix; data dir creation
# 1.3.0 - 2026-09-20 - Added --init-security; lock dir fix; systemd-run fallback for non-root
# 1.2.0 - 2026-09-20 - Added security database initialization fallback
# 1.1.0 - 2026-09-20 - Requires sudo to write to /var/lib/firebird/data/
# 1.0.0 - 2026-09-18 - Initial version for Firebird 4.0 on Fedora 43

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HYDROGEN_ROOT="${HYDROGEN_ROOT:-$(cd "${SCRIPT_DIR}/../.." && pwd)}"
ARTIFACTS_DIR="${HYDROGEN_ROOT}/tests/artifacts/database/firebird"
DEFAULT_TEST_PATH="${ARTIFACTS_DIR}/hydrogen_test.fdb"
DEFAULT_DEMO_PATH="${ARTIFACTS_DIR}/hydrogen_demo.fdb"

HOST="127.0.0.1"
PORT="3050"
SYSDBA_PASSWORD="${FIREBIRD_SYSDBA_PASSWORD:-masterkey}"
# shellcheck disable=SC2034 # Named account constant for clarity / future use
SYSDBA_USER="SYSDBA"
SECPATH="/var/lib/firebird/secdb/security4.fdb"
LOCKDIR="/tmp/firebird"
FIREBIRD_SERVICE="firebird"
FIREBIRD_USER="firebird"
# shellcheck disable=SC2034 # Used by test framework / external consumers for test abbreviation
TESTABBR="FBD"

die() {
    echo "Error: $*" >&2
    exit 1
}

if [[ "${FIREBIRD_SYSDBA_PASSWORD:-}" == "" ]]; then
    echo "Warning: FIREBIRD_SYSDBA_PASSWORD is not set. Using default 'masterkey'." >&2
fi

# --- Resolve create mode and target path list ---
# Positional: --init-security | both|test|demo | /path/to.fdb | (empty → both)
CREATE_MODE="${FIREBIRD_CREATE_MODE:-both}"
ONEOFF_PATH=""
ARG1="${1:-}"

if [[ "${ARG1}" == "--init-security" ]]; then
    CREATE_MODE="init-security"
elif [[ "${ARG1}" == "both" || "${ARG1}" == "test" || "${ARG1}" == "demo" ]]; then
    CREATE_MODE="${ARG1}"
elif [[ -n "${ARG1}" ]]; then
    # One-off: treat as a single database path (legacy / ad-hoc)
    CREATE_MODE="oneoff"
    ONEOFF_PATH="${ARG1}"
elif [[ -n "${FIREBIRD_DB_PATH:-}" && -z "${FIREBIRD_DB_PATH_TEST:-}" && -z "${FIREBIRD_DB_PATH_DEMO:-}" ]]; then
    # Deprecated singular alone: do NOT silently recreate the old single file on
    # default "both". Map singular → TEST path and still create DEMO at default
    # (or warn and ignore singular if it looks like a legacy shared name).
    echo "Warning: FIREBIRD_DB_PATH is deprecated; prefer FIREBIRD_DB_PATH_TEST / FIREBIRD_DB_PATH_DEMO." >&2
    base="$(basename "${FIREBIRD_DB_PATH}")"
    if [[ "${base}" == "hydrogen.fdb" || "${base}" == "testfb.fdb" || "${base}" == "demofb.fdb" ]]; then
        echo "Warning: ignoring legacy singular path '${FIREBIRD_DB_PATH}' for mode=both; using hydrogen_test.fdb + hydrogen_demo.fdb defaults." >&2
        # leave CREATE_MODE as both; dual paths stay at defaults
        :
    else
        # Non-legacy singular: treat as TEST override, still create DEMO default
        FIREBIRD_DB_PATH_TEST="${FIREBIRD_DB_PATH}"
        echo "Info: mapping FIREBIRD_DB_PATH → FIREBIRD_DB_PATH_TEST='${FIREBIRD_DB_PATH_TEST}' (demo still default)." >&2
    fi
fi

TEST_PATH="${FIREBIRD_DB_PATH_TEST:-${DEFAULT_TEST_PATH}}"
DEMO_PATH="${FIREBIRD_DB_PATH_DEMO:-${DEFAULT_DEMO_PATH}}"

# Build TARGET_PATHS array for the selected mode
TARGET_PATHS=()
case "${CREATE_MODE}" in
    both)
        TARGET_PATHS+=("${TEST_PATH}" "${DEMO_PATH}")
        ;;
    test)
        TARGET_PATHS+=("${TEST_PATH}")
        ;;
    demo)
        TARGET_PATHS+=("${DEMO_PATH}")
        ;;
    oneoff)
        TARGET_PATHS+=("${ONEOFF_PATH}")
        ;;
    init-security)
        ;;
    *)
        die "Unknown create mode '${CREATE_MODE}' (use both|test|demo or a .fdb path)"
        ;;
esac

if [[ "${CREATE_MODE}" != "init-security" ]]; then
    echo "Info: create mode=${CREATE_MODE}; targets: ${TARGET_PATHS[*]}" >&2
    if [[ -z "${FIREBIRD_DB_PATH_TEST:-}" && "${CREATE_MODE}" != "oneoff" && "${CREATE_MODE}" != "demo" ]]; then
        echo "Info: FIREBIRD_DB_PATH_TEST unset; using default '${TEST_PATH}'." >&2
    fi
    if [[ -z "${FIREBIRD_DB_PATH_DEMO:-}" && "${CREATE_MODE}" != "oneoff" && "${CREATE_MODE}" != "test" ]]; then
        echo "Info: FIREBIRD_DB_PATH_DEMO unset; using default '${DEMO_PATH}'." >&2
    fi
fi

# First target (or the TEST default) is the auth probe when no .fdb exists yet.
PRIMARY_PATH="${TARGET_PATHS[0]:-${TEST_PATH}}"

# --- Helper: get the firebird user's UID ---
get_firebird_uid() {
    id -u "${FIREBIRD_USER}" 2>/dev/null || die "Cannot find firebird user UID."
}

# --- Helper: run a script as the firebird user via systemd-run ---
# Writes the script body to a temp file and executes it via systemd-run.
# This avoids root_squash: file ops in /var/lib/firebird/ must be done as firebird.
as_firebird() {
    local script="$1"
    local uid
    uid=$(get_firebird_uid)
    local tmpscript
    tmpscript=$(mktemp /tmp/fb_run.XXXXXX.sh)
    printf '%s\n' "${script}" > "${tmpscript}"
    chmod 755 "${tmpscript}"
    systemd-run --uid="${uid}" --wait \
        -p "User=${FIREBIRD_USER}" \
        -p "StandardOutput=inherit" \
        -p "StandardError=inherit" \
        -E "FIREBIRD=${FIREBIRD}" \
        /bin/bash "${tmpscript}" 2>&1
    local rc=$?
    rm -f "${tmpscript}"
    return "${rc}"
}

# --- Helper: test if SYSDBA can authenticate via network to a path ---
test_auth_path() {
    local db_path="$1"
    local uid
    uid=$(get_firebird_uid)
    local server="${HOST}/${PORT}:${db_path}"
    systemd-run --uid="${uid}" --wait \
        -p "User=${FIREBIRD_USER}" \
        -p "StandardOutput=inherit" \
        -p "StandardError=inherit" \
        -E "FIREBIRD=${FIREBIRD}" \
        isql-fb -user SYSDBA -password "${SYSDBA_PASSWORD}" "${server}" <<'SQL' 2>&1
SELECT 1 FROM RDB$DATABASE;
SQL
}

# --- Helper: check if security database exists and is non-empty ---
check_security_db() {
    local uid
    uid=$(get_firebird_uid)
    systemd-run --uid="${uid}" --wait \
        -p "User=${FIREBIRD_USER}" \
        -p "StandardOutput=inherit" \
        -p "StandardError=inherit" \
        /bin/bash -c "test -s '${SECPATH}'" 2>/dev/null
}

# --- Helper: fix lock directory permissions ---
fix_lockdir() {
    if [[ ! -d "${LOCKDIR}" ]]; then
        mkdir -p "${LOCKDIR}"
    fi
    chown "${FIREBIRD_USER}:${FIREBIRD_USER}" "${LOCKDIR}" 2>/dev/null || true
    chmod 755 "${LOCKDIR}" 2>/dev/null || true
    # Also set the Firebird lock env var
    export FIREBIRD="${LOCKDIR}"
}

# --- Helper: ensure a directory exists and is usable by firebird + developers ---
ensure_dir() {
    local dir="$1"
    mkdir -p "${dir}"
    chown "${FIREBIRD_USER}:${FIREBIRD_USER}" "${dir}" 2>/dev/null || true
    # u=rwx for firebird; g/o=rwx so repo checkouts are listable/writable for recreate
    chmod 777 "${dir}" 2>/dev/null || chmod a+rwx "${dir}" 2>/dev/null || true
}

# --- Helper: stop the firebird service ---
stop_service() {
    systemctl stop "${FIREBIRD_SERVICE}" 2>/dev/null || true
    sleep 2
    # Wait for port to free
    local waited=0
    while [[ ${waited} -lt 10 ]]; do
        if ! ss -tln 2>/dev/null | grep -q ":${PORT} "; then
            return 0
        fi
        waited=$((waited + 1))
        sleep 1
    done
}

# --- Helper: start the firebird service and wait for port ---
start_service() {
    systemctl start "${FIREBIRD_SERVICE}" 2>/dev/null || true
    local waited=0
    while [[ ${waited} -lt 15 ]]; do
        if ss -tln 2>/dev/null | grep -q ":${PORT} "; then
            return 0
        fi
        waited=$((waited + 1))
        sleep 1
    done
    die "Firebird did not start listening on port ${PORT} within 15s."
}

# --- Helper: resolve the actual systemd service name ---
resolve_service() {
    for svc in firebird firebird-superserver; do
        if systemctl list-unit-files "${svc}.service" >/dev/null 2>&1; then
            FIREBIRD_SERVICE="${svc}"
            return 0
        fi
    done
}

# --- Apply ownership/mode on a created .fdb ---
apply_perms() {
    local db_path="$1"
    local db_dir
    db_dir="$(dirname "${db_path}")"
    # Permissions: firebird owns the file (server); group/other get rw so
    # developers can inspect or wipe it under a repo path without joining the
    # firebird group. Prefer the elevating caller's primary group (sudo or pkexec).
    local inv_user="${SUDO_USER:-}"
    if [[ -z "${inv_user}" && -n "${PKEXEC_UID:-}" ]]; then
        inv_user="$(getent passwd "${PKEXEC_UID}" | cut -d: -f1 || true)"
    fi
    local inv_gid=""
    if [[ -n "${inv_user}" ]]; then
        inv_gid="$(id -g "${inv_user}" 2>/dev/null || true)"
    fi
    if [[ -n "${inv_gid}" ]]; then
        chown "${FIREBIRD_USER}:${inv_gid}" "${db_path}" 2>/dev/null || \
            chown "${FIREBIRD_USER}:${FIREBIRD_USER}" "${db_path}" 2>/dev/null || true
    else
        chown "${FIREBIRD_USER}:${FIREBIRD_USER}" "${db_path}" 2>/dev/null || true
    fi
    chmod 666 "${db_path}" 2>/dev/null || chmod a+rw "${db_path}" 2>/dev/null || true
    chmod a+rwX "${db_dir}" 2>/dev/null || true
    local perm_line
    perm_line=$(stat -c '%A %U %G' "${db_path}" 2>/dev/null || true)
    echo "Permissions on ${db_path}: ${perm_line}"
}

# --- Create (drop+recreate) one database file ---
create_one_db() {
    local db_path="$1"
    local db_name
    db_name="$(basename "${db_path}")"
    local db_dir
    db_dir="$(dirname "${db_path}")"
    local server="${HOST}/${PORT}:${db_path}"

    ensure_dir "${db_dir}"

    echo "Checking for existing ${db_name} at ${db_path}..."
    # shellcheck disable=SC2310 # Function invoked in || condition, rm failure is non-fatal
    as_firebird "rm -f '${db_path}'" 2>/dev/null || true

    # PAGE_SIZE 32768: UTF8 UNIQUE(varchar(500),varchar(500)) needs >4KB key (page/4 limit).
    # Do not put bash '#' comments inside the isql heredoc — isql will choke / no-op.
    echo "Creating database ${db_name}..."
    local create_script
    create_script="isql-fb -user SYSDBA -password '${SYSDBA_PASSWORD}' <<SQL
CREATE DATABASE '${db_path}' PAGE_SIZE 32768 DEFAULT CHARACTER SET UTF8;
SQL"

    # shellcheck disable=SC2310 # Function invoked in if condition, error handling intentional
    if as_firebird "${create_script}" 2>&1; then
        echo "Database ${db_name} create command finished for ${server}"
    else
        die "Failed to create database ${db_name}"
    fi

    # --- Verify the file exists on disk (connect checks alone can false-pass) ---
    if [[ ! -f "${db_path}" ]]; then
        die "CREATE reported success but file missing at ${db_path} (check Firebird DatabaseAccess / dir perms for ${FIREBIRD_USER})"
    fi
    echo "Database file present: $(ls -l "${db_path}" || true)"

    # shellcheck disable=SC2310 # Function invoked in if condition, verification intentional
    if test_auth_path "${db_path}" >/dev/null 2>&1; then
        echo "Database ${db_name} verified connectable."
    else
        die "Failed to verify database ${db_name}"
    fi

    apply_perms "${db_path}"
    echo "Database ${db_name} is ready for migrations."
}

# --- Require sudo ---
if [[ "${EUID}" -ne 0 ]]; then
    echo "Firebird database management requires sudo privileges."
    echo "Run this script with: sudo $0 $*"
    echo ""
    exec sudo --preserve-env=FIREBIRD_DB_PATH_TEST,FIREBIRD_DB_PATH_DEMO,FIREBIRD_DB_PATH,FIREBIRD_SYSDBA_PASSWORD,FIREBIRD_CREATE_MODE,HYDROGEN_ROOT,FIREBIRD "$0" "$@"
fi

resolve_service
fix_lockdir

# Ensure dirs for all targets up front
if [[ "${CREATE_MODE}" != "init-security" ]]; then
    for p in "${TARGET_PATHS[@]}"; do
        ensure_dir "$(dirname "${p}")"
    done
else
    ensure_dir "$(dirname "${SECPATH}")"
fi

# --- Handle --init-security flag ---
if [[ "${CREATE_MODE}" == "init-security" ]]; then
    echo "Initializing Firebird security database at ${SECPATH}..."

    stop_service

    # Create the security database and SYSDBA user in embedded mode as the firebird user.
    # Embedded mode: isql-fb with a direct file path, no network, no password needed.
    init_script="rm -f '${SECPATH}' && echo \"CREATE DATABASE '${SECPATH}' PAGE_SIZE 32768 DEFAULT CHARACTER SET UTF8;\" | isql-fb && echo \"CREATE USER SYSDBA PASSWORD '${SYSDBA_PASSWORD}';\" | isql-fb '${SECPATH}' -user SYSDBA"

    # shellcheck disable=SC2310 # Function invoked in if condition, error handling intentional
    if as_firebird "${init_script}" 2>&1; then
        echo "Security database initialized with SYSDBA user."
    else
        die "Failed to initialize security database."
    fi

    start_service
    echo "Security database initialized and Firebird restarted."
    exit 0
fi

# --- Ensure security database is initialized ---
# shellcheck disable=SC2310 # Function invoked in ! condition, missing security DB is a hard error
if ! check_security_db; then
    echo "Security database missing or empty at ${SECPATH}."
    echo "Run: sudo $0 --init-security"
    die "Security database at ${SECPATH} does not exist or is empty."
fi

# --- Test authentication (against primary target; may not exist yet — Firebird still auths) ---
echo "Testing SYSDBA authentication..."
# Prefer an existing .fdb among targets for a cleaner auth probe; else primary path.
AUTH_PROBE="${PRIMARY_PATH}"
for p in "${TARGET_PATHS[@]}"; do
    if [[ -f "${p}" ]]; then
        AUTH_PROBE="${p}"
        break
    fi
done
# shellcheck disable=SC2310 # Function invoked in ! condition, auth failure is a hard error
if ! test_auth_path "${AUTH_PROBE}" >/dev/null 2>&1; then
    # If no DB file yet, auth against security via creating a throwaway connect can fail;
    # fall through with warning only when no file exists — hard fail if file existed.
    if [[ -f "${AUTH_PROBE}" ]]; then
        echo "Warning: Cannot authenticate as SYSDBA. The security database may need re-initialization."
        echo "Run: sudo $0 --init-security"
        die "Cannot authenticate as SYSDBA at ${HOST}/${PORT}:${AUTH_PROBE}"
    else
        echo "Info: No existing target .fdb for auth probe; continuing to CREATE (SYSDBA password will be used)."
    fi
else
    echo "SYSDBA authentication OK."
fi

# --- Create selected databases (only those in TARGET_PATHS — never touch the other) ---
for p in "${TARGET_PATHS[@]}"; do
    create_one_db "${p}"
done

echo "All requested databases ready."
