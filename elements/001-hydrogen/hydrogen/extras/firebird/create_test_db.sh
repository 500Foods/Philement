#!/usr/bin/env bash
# Firebird test database creation — creates testfb.fdb (drops existing first)
#
# Usage:
#   sudo ./create_test_db.sh [database.fdb]
#   sudo FIREBIRD_DB_PATH=/path/to/test.fdb ./create_test_db.sh
#   sudo ./create_test_db.sh --init-security
#
# Environment:
#   FIREBIRD_DB_PATH         Path to the database file (default: /var/lib/firebird/data/testfb.fdb)
#   FIREBIRD_SYSDBA_PASSWORD Password for the SYSDBA user (default: masterkey)
#
# Requires: sudo, Firebird SuperServer running on 3050, the firebird system user.
#
# --init-security: Stop the server, initialize the security database
#   (security4.fdb) with a SYSDBA user in embedded mode, restart the server.
#   This only needs to be run once, or whenever the security database is
#   missing or corrupted.
#
# CHANGELOG
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

DB_NAME="${1:-testfb.fdb}"
# Honor FIREBIRD_DB_PATH for the database file location; fall back to the
# default data directory. When --init-security is passed, $1 is the flag
# and DB_NAME keeps its default.
if [[ "${1:-}" == "--init-security" ]]; then
    DB_NAME="testfb.fdb"
fi
DB_PATH="${FIREBIRD_DB_PATH:-/var/lib/firebird/data/${DB_NAME}}"
DB_DIR="$(dirname "${DB_PATH}")"
HOST="127.0.0.1"
PORT="3050"
SYSDBA_PASSWORD="${FIREBIRD_SYSDBA_PASSWORD:-masterkey}"
# shellcheck disable=SC2034 # Named account constant for clarity / future use
SYSDBA_USER="SYSDBA"
SERVER="${HOST}/${PORT}:${DB_PATH}"
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

if [[ -z "${FIREBIRD_DB_PATH:-}" ]]; then
    echo "Info: FIREBIRD_DB_PATH not set. Using default '${DB_PATH}'." >&2
fi

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

# --- Helper: test if SYSDBA can authenticate via network ---
test_auth() {
    local uid
    uid=$(get_firebird_uid)
    systemd-run --uid="${uid}" --wait \
        -p "User=${FIREBIRD_USER}" \
        -p "StandardOutput=inherit" \
        -p "StandardError=inherit" \
        -E "FIREBIRD=${FIREBIRD}" \
        isql-fb -user SYSDBA -password "${SYSDBA_PASSWORD}" "${SERVER}" <<'SQL' 2>&1
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

# --- Helper: ensure data directory exists and is owned by firebird ---
ensure_data_dir() {
    mkdir -p "${DB_DIR}"
    chown "${FIREBIRD_USER}:${FIREBIRD_USER}" "${DB_DIR}" 2>/dev/null || true
    chmod 755 "${DB_DIR}" 2>/dev/null || true
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

# --- Require sudo ---
if [[ "${EUID}" -ne 0 ]]; then
    echo "Firebird database management requires sudo privileges."
    echo "Run this script with: sudo $0"
    echo ""
    exec sudo "$0" "$@"
fi

resolve_service
fix_lockdir
ensure_data_dir

# --- Handle --init-security flag ---
if [[ "${1:-}" == "--init-security" ]]; then
    echo "Initializing Firebird security database at ${SECPATH}..."

    stop_service

    # Create the security database and SYSDBA user in embedded mode as the firebird user.
    # Embedded mode: isql-fb with a direct file path, no network, no password needed.
    init_script="rm -f '${SECPATH}' && echo \"CREATE DATABASE '${SECPATH}' PAGE_SIZE 4096 DEFAULT CHARACTER SET UTF8;\" | isql-fb && echo \"CREATE USER SYSDBA PASSWORD '${SYSDBA_PASSWORD}';\" | isql-fb '${SECPATH}' -user SYSDBA"

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

# --- Test authentication ---
echo "Testing SYSDBA authentication..."
# shellcheck disable=SC2310 # Function invoked in ! condition, auth failure is a hard error
if ! test_auth >/dev/null 2>&1; then
    echo "Warning: Cannot authenticate as SYSDBA. The security database may need re-initialization."
    echo "Run: sudo $0 --init-security"
    die "Cannot authenticate as SYSDBA at ${SERVER}"
fi
echo "SYSDBA authentication OK."

# --- Drop existing database if it exists ---
echo "Checking for existing ${DB_NAME} at ${DB_PATH}..."
# shellcheck disable=SC2310 # Function invoked in || condition, rm failure is non-fatal
as_firebird "rm -f '${DB_PATH}'" 2>/dev/null || true

# --- Create the database (as firebird user, via network) ---
echo "Creating database ${DB_NAME}..."
    create_script="isql-fb -user SYSDBA -password '${SYSDBA_PASSWORD}' <<SQL"
create_script+="
CREATE DATABASE '${DB_PATH}' PAGE_SIZE 4096 DEFAULT CHARACTER SET UTF8;
SQL"

# shellcheck disable=SC2310 # Function invoked in if condition, error handling intentional
if as_firebird "${create_script}" 2>&1; then
    echo "Database ${DB_NAME} created at ${SERVER}"
else
    die "Failed to create database ${DB_NAME}"
fi

# --- Verify it was created ---
# shellcheck disable=SC2310 # Function invoked in if condition, verification intentional
if test_auth >/dev/null 2>&1; then
    echo "Database ${DB_NAME} verified connectable."
else
    die "Failed to verify database ${DB_NAME}"
fi

# --- Database is ready (SYSDBA has full privileges by default in Firebird) ---
echo "Database is ready for migrations."
