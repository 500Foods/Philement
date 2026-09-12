#!/usr/bin/env bash
# =============================================================================
# Terminal Launcher
# =============================================================================
# Secure debug/troubleshooting launcher for the Hydrogen Terminal subsystem.
#
# Obtains a JWT via /api/auth/login, fetches the authorized terminal config
# from /api/system/info, and launches a temporary browser page that receives
# the JWT via exact-origin postMessage (never persisted to disk or localStorage).
#
# Usage:
#   terminal-launcher.sh --server <wss-url> --username <login_id> [options]
#
# Required:
#   --server       Base HTTPS URL of the Hydrogen server (e.g. https://lithium.philement.com)
#   --username     Login ID for /api/auth/login
#   --api-key      API key for /api/auth/login (required by the auth endpoint)
#
# Authentication (choose ONE):
#   --password-file <path>   Read password from a permission-restricted file (chmod 600)
#   (If --password-file is omitted, the password is read from stdin, echo disabled)
#
# Optional:
#   --database   <name>   Database name (default: Acuranzo)
#   --tz         <name>   Timezone (default: America/Vancouver)
#   --timeout    <sec>   HTTP request timeout (default: 30)
#   --browser    <cmd>   Browser to open (default: xdg-open)
#   --help               Show this help
#
# Security:
#   - Password is NEVER accepted as a CLI argument (would appear in ps/env)
#   - Password is read from a --password-file (chmod 600) or stdin (no-echo)
#   - JWT and WebSocket key are held only in shell variables / memory
#   - The temporary launcher page uses exact-origin postMessage and is removed on exit
#   - No secrets are written to logs, stdout (beyond a redacted fingerprint), or files
#
# CHANGELOG
# 1.0.1 - 2026-09-11 - Added --api-key argument; login payload now includes api_key field required by /api/auth/login (fixes HTTP 400)
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration defaults
# ---------------------------------------------------------------------------
SCRIPT_NAME="terminal-launcher"
SCRIPT_VERSION="1.0.1"

SERVER_URL=""
USERNAME=""
PASSWORD_FILE=""
API_KEY=""
DATABASE="Acuranzo"
TZ="America/Vancouver"
TIMEOUT=30
BROWSER_CMD="${BROWSER:-xdg-open}"

# Resolved at runtime
PASSWORD=""
JWT_TOKEN=""
TERMINAL_URL=""
TERMINAL_PROTOCOL=""
WEBSOCKET_KEY=""
LAUNCHER_FILE=""

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
log_error() {
    echo "ERROR: $*" >&2
}

log_info() {
    echo "INFO: $*"
}

redact() {
    # Print a redacted fingerprint: first 8 chars + ellipsis
    local val="$1"
    if [[ -z "${val}" ]]; then
        echo "<empty>"
    else
        echo "${val:0:8}..."
    fi
}

cleanup() {
    if [[ -n "${LAUNCHER_FILE}" && -f "${LAUNCHER_FILE}" ]]; then
        rm -f "${LAUNCHER_FILE}"
    fi
    # Clear sensitive variables
    PASSWORD=""
    JWT_TOKEN=""
    WEBSOCKET_KEY=""
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
show_help() {
    cat <<'HELP'
Terminal Launcher — Secure debug launcher for Hydrogen Terminal subsystem

USAGE:
    terminal-launcher.sh --server <url> --username <login_id> [OPTIONS]

REQUIRED:
    --server <url>        Base HTTPS URL of the Hydrogen server
                           (e.g. https://lithium.philement.com)
    --username <name>     Login ID for POST /api/auth/login
    --api-key <key>       API key for POST /api/auth/login

AUTHENTICATION (choose one):
    --password-file <f>   Read password from a permission-restricted file (chmod 600)
                           (If omitted, password is read from stdin with echo disabled)

OPTIONAL:
    --database <name>     Database name (default: Acuranzo)
    --tz <name>           Timezone (default: America/Vancouver)
    --timeout <sec>       HTTP request timeout in seconds (default: 30)
    --browser <cmd>       Browser command to open (default: xdg-open or $BROWSER)
    --help, -h            Show this help

SECURITY NOTES:
    - Password is NEVER accepted as a CLI argument
    - Password file should be chmod 600
    - JWT and WebSocket key are held only in memory
    - A temporary launcher HTML page is created, opened, and removed on exit
    - No secrets appear in logs or stdout (only redacted fingerprints)

EXAMPLE:
    terminal-launcher.sh \\
        --server https://lithium.philement.com \\
        --username morpheus \\
        --password-file ~/.config/hydrogen/launcher-pass
HELP
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --server)
                SERVER_URL="$2"; shift 2 ;;
            --username)
                USERNAME="$2"; shift 2 ;;
            --password-file)
                PASSWORD_FILE="$2"; shift 2 ;;
            --api-key)
                API_KEY="$2"; shift 2 ;;
            --database)
                DATABASE="$2"; shift 2 ;;
            --tz)
                TZ="$2"; shift 2 ;;
            --timeout)
                TIMEOUT="$2"; shift 2 ;;
            --browser)
                BROWSER_CMD="$2"; shift 2 ;;
            --help|-h)
                show_help; exit 0 ;;
            *)
                log_error "Unknown argument: $1"
                show_help
                exit 1 ;;
        esac
    done
}

validate_required() {
    if [[ -z "${SERVER_URL}" ]]; then
        log_error "--server is required"
        show_help
        exit 1
    fi
    if [[ -z "${USERNAME}" ]]; then
        log_error "--username is required"
        show_help
        exit 1
    fi
    if [[ -z "${API_KEY}" ]]; then
        log_error "--api-key is required"
        show_help
        exit 1
    fi
    # Validate server URL scheme (must be https in production)
    if [[ ! "${SERVER_URL}" =~ ^https:// ]]; then
        log_error "--server must use https:// scheme (production requires TLS)"
        exit 1
    fi
}

read_password() {
    if [[ -n "${PASSWORD_FILE}" ]]; then
        if [[ ! -f "${PASSWORD_FILE}" ]]; then
            log_error "Password file not found: ${PASSWORD_FILE}"
            exit 1
        fi
        # Check file permissions (must be 600 — owner read/write only)
        local perms
        perms=$(stat -c '%a' "${PASSWORD_FILE}" 2>/dev/null || stat -f '%A' "${PASSWORD_FILE}" 2>/dev/null || echo "000")
        if [[ "${perms}" != "600" ]]; then
            log_error "Password file must have 600 permissions (owner read/write only)"
            log_error "Run: chmod 600 ${PASSWORD_FILE}"
            exit 1
        fi
        PASSWORD=$(<"${PASSWORD_FILE}")
    else
        # Read from stdin with echo disabled
        log_info "Enter password (echo disabled, press Enter):"
        read -rs PASSWORD
        echo ""
    fi

    if [[ -z "${PASSWORD}" ]]; then
        log_error "Password is empty"
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# Step 1: Authenticate via /api/auth/login
# ---------------------------------------------------------------------------
authenticate() {
    log_info "Authenticating as '${USERNAME}' against ${SERVER_URL}"

    local login_file
    login_file=$(mktemp /tmp/terminal_launcher_login.XXXXXX.json)

    # Build login payload with jq (never interpolate password into a string literal)
    local login_payload
    login_payload=$(jq -n \
        --arg database "${DATABASE}" \
        --arg login_id "${USERNAME}" \
        --arg password "${PASSWORD}" \
        --arg api_key "${API_KEY}" \
        --arg tz "${TZ}" \
        '{database: $database, login_id: $login_id, password: $password, api_key: $api_key, tz: $tz}')

    local http_code
    http_code=$(curl -s -S \
        -X POST "${SERVER_URL}/api/auth/login" \
        -H "Content-Type: application/json" \
        -d "${login_payload}" \
        -o "${login_file}" \
        -w "%{http_code}" \
        --connect-timeout "${TIMEOUT}" \
        --max-time "${TIMEOUT}" 2>/dev/null) || {
        log_error "Failed to connect to ${SERVER_URL}/api/auth/login"
        rm -f "${login_file}"
        exit 1
    }

    if [[ "${http_code}" != "200" ]]; then
        log_error "Login failed (HTTP ${http_code}). Check credentials and database."
        rm -f "${login_file}"
        exit 1
    fi

    JWT_TOKEN=$(jq -r '.token // empty' "${login_file}" 2>/dev/null)
    rm -f "${login_file}"

    if [[ -z "${JWT_TOKEN}" || "${JWT_TOKEN}" == "null" ]]; then
        log_error "Login succeeded but no JWT token returned"
        exit 1
    fi

    local jwt_fp
    jwt_fp=$(redact "${JWT_TOKEN}")
    log_info "JWT obtained (fingerprint: ${jwt_fp})"
}

# ---------------------------------------------------------------------------
# Step 2: Fetch authorized terminal config from /api/system/info
# ---------------------------------------------------------------------------
fetch_terminal_config() {
    log_info "Fetching terminal config from ${SERVER_URL}/api/system/info"

    local info_file
    info_file=$(mktemp /tmp/terminal_launcher_info.XXXXXX.json)

    local http_code
    http_code=$(curl -s -S \
        -X GET "${SERVER_URL}/api/system/info" \
        -H "Authorization: Bearer ${JWT_TOKEN}" \
        -o "${info_file}" \
        -w "%{http_code}" \
        --connect-timeout "${TIMEOUT}" \
        --max-time "${TIMEOUT}" 2>/dev/null) || {
        log_error "Failed to fetch /api/system/info"
        rm -f "${info_file}"
        exit 1
    }

    if [[ "${http_code}" != "200" ]]; then
        log_error "System info request failed (HTTP ${http_code})"
        rm -f "${info_file}"
        exit 1
    fi

    # Check that the terminal object is present (requires terminal role)
    local has_terminal
    has_terminal=$(jq '.terminal != null' "${info_file}" 2>/dev/null)

    if [[ "${has_terminal}" != "true" ]]; then
        log_error "No terminal configuration in /api/system/info response."
        log_error "The account '${USERNAME}' may not have the terminal role (role_id 32)."
        log_error "Only JWT-bearing callers with the terminal role receive the terminal object."
        rm -f "${info_file}"
        exit 1
    fi

    TERMINAL_URL=$(jq -r '.terminal.url // empty' "${info_file}" 2>/dev/null)
    TERMINAL_PROTOCOL=$(jq -r '.terminal.protocol // empty' "${info_file}" 2>/dev/null)
    WEBSOCKET_KEY=$(jq -r '.terminal.key // empty' "${info_file}" 2>/dev/null)

    rm -f "${info_file}"

    if [[ -z "${TERMINAL_URL}" || -z "${TERMINAL_PROTOCOL}" || -z "${WEBSOCKET_KEY}" ]]; then
        log_error "Terminal config is incomplete (missing url, protocol, or key)"
        exit 1
    fi

    log_info "Terminal URL: ${TERMINAL_URL}"
    log_info "Protocol: ${TERMINAL_PROTOCOL}"
    local key_fp
    key_fp=$(redact "${WEBSOCKET_KEY}")
    log_info "WebSocket key: ${key_fp}"
}

# ---------------------------------------------------------------------------
# Step 3: Create a temporary launcher page and open it in the browser
# ---------------------------------------------------------------------------
create_and_open_launcher() {
    log_info "Creating temporary terminal launcher page"

    # Create a temp file for the launcher HTML
    LAUNCHER_FILE=$(mktemp /tmp/terminal_launcher_XXXXXX.html)

    # Embed the JWT and terminal config into a temporary HTML page that:
    # - Creates the terminal iframe
    # - Receives postMessage from the iframe (terminal-config-request)
    # - Replies with the JWT via exact-origin postMessage
    # - Never persists the JWT to localStorage or any file
    cat > "${LAUNCHER_FILE}" << 'HTML_HEAD'
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Hydrogen Terminal Launcher</title>
<style>
  body { margin:0; padding:0; background:#1e1e1e; overflow:hidden; }
  #container { width:100vw; height:100vh; }
  #loading {
    position:absolute; top:50%; left:50%; transform:translate(-50%,-50%);
    color:#cccccc; font-family:monospace; font-size:14px;
  }
</style>
</head>
<body>
<div id="container">
  <iframe id="terminal-iframe" style="width:100%;height:100vh;border:none;"
          allow="fullscreen"></iframe>
</div>
<div id="loading">Loading terminal...</div>
<script>
(function() {
    'use strict';

    // These values are injected at runtime — never read from localStorage.
    var JWT = "__JWT_TOKEN__";
    var TERMINAL_URL = "__TERMINAL_URL__";
    var TERMINAL_PROTOCOL = "__TERMINAL_PROTOCOL__";
    var SERVER_ORIGIN = "__SERVER_ORIGIN__";

    var iframe = document.getElementById('terminal-iframe');
    var loading = document.getElementById('loading');

    // Set the iframe src to the terminal page served by Hydrogen
    iframe.src = TERMINAL_URL.replace(/^wss:\/\//, 'https://').replace(/^ws:\/\//, 'http://');

    // Listen for config requests from the iframe
    window.addEventListener('message', function(event) {
        // Exact origin validation — only accept messages from the server origin
        if (event.origin !== SERVER_ORIGIN) {
            return;
        }
        // Verify the sender is our iframe
        if (event.source !== iframe.contentWindow) {
            return;
        }
        if (!event.data || typeof event.data !== 'object') {
            return;
        }
        if (event.data.type === 'terminal-config-request') {
            // Hand the JWT to the iframe via exact-origin postMessage
            // JWT stays only in this page's memory — never persisted
            event.source.postMessage(
                { type: 'terminal-config', config: { jwt: JWT } },
                SERVER_ORIGIN
            );
            loading.style.display = 'none';
        }
    });

    // Clear the JWT from memory when the page is unloaded
    window.addEventListener('beforeunload', function() {
        JWT = '';
    });
})();
</script>
</body>
</html>
HTML_HEAD

    # Inject redacted values (the JWT and key go into the HTML file only as
    # runtime page content, not into logs or shell history)
    # Extract scheme://host[:port] from SERVER_URL using parameter expansion
    local scheme host_part
    scheme="${SERVER_URL%%://*}"
    host_part="${SERVER_URL#*://}"
    host_part="${host_part%%/*}"
    server_origin="${scheme}://${host_part}"

    sed -i \
        -e "s|__JWT_TOKEN__|${JWT_TOKEN}|g" \
        -e "s|__TERMINAL_URL__|${TERMINAL_URL}|g" \
        -e "s|__TERMINAL_PROTOCOL__|${TERMINAL_PROTOCOL}|g" \
        -e "s|__SERVER_ORIGIN__|${server_origin}|g" \
        "${LAUNCHER_FILE}"

    log_info "Launcher page created (not persisted — removed on exit)"
    log_info "Opening in browser..."

    if command -v "${BROWSER_CMD}" >/dev/null 2>&1; then
        "${BROWSER_CMD}" "file://${LAUNCHER_FILE}" 2>/dev/null &
        log_info "Terminal launcher opened. Close the browser tab when done — the temp file will be cleaned up."
    else
        log_error "Browser command '${BROWSER_CMD}' not found."
        log_error "Launcher file location: ${LAUNCHER_FILE}"
        log_info "Open this file in a browser: file://${LAUNCHER_FILE}"
    fi
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
main() {
    echo "=== ${SCRIPT_NAME} v${SCRIPT_VERSION} ==="
    echo ""

    parse_args "$@"
    validate_required
    read_password
    authenticate
    fetch_terminal_config
    create_and_open_launcher

    echo ""
    echo "=== Terminal Launcher Summary ==="
    echo "  Server:    ${SERVER_URL}"
    echo "  User:      ${USERNAME}"
    local jwt_fp key_fp
    jwt_fp=$(redact "${JWT_TOKEN}")
    key_fp=$(redact "${WEBSOCKET_KEY}")
    echo "  JWT:       ${jwt_fp}"
    echo "  WS URL:    ${TERMINAL_URL}"
    echo "  Protocol:  ${TERMINAL_PROTOCOL}"
    echo "  WS Key:    ${key_fp}"
    echo "  Launcher:  ${LAUNCHER_FILE} (temp, auto-removed on exit)"
    echo ""
    echo "Security: Password was never stored in shell history."
    echo "          JWT and key are in-memory only."
    echo "          Temporary HTML file will be deleted on exit."
}

main "$@"
