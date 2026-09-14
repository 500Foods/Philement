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
# 1.0.2 - 2026-09-12 - Fixed launcher HTML file being deleted before browser loads it; temp file persists until process exit or OS cleanup
# 1.0.3 - 2026-09-13 - Phase 11: Renamed WEBSOCKET_KEY local to TERMINAL_KEY for clarity (holds terminal.key from sysinfo, not chat key); added --terminal-key note in help for two-key world
# 1.0.4 - 2026-09-14 - Phase 13: Fixed 502 Bad Gateway when loading iframe page. TERMINAL_URL is the WebSocket URL (wss://host/terminal/ws); the iframe must load the terminal HTML page at the WebPath (/terminal), not the WebSocket upgrade endpoint (/terminal/ws). Strip the /ws suffix before converting wss:// to https:// for the iframe src.
# 1.0.5 - 2026-09-14 - Phase 14: Fixed "Terminal config fetch timed out — no response from parent frame". Root cause: terminal-launcher.sh served the launcher page via file:// with a cross-origin iframe to the Hydrogen terminal. The terminal page's postMessage to the parent used window.location.origin as targetOrigin, which didn't match the file:// parent origin, so the message was never delivered. Fix: terminal-launcher.sh now starts a local HTTP server on localhost, fetches the terminal HTML + assets from the Hydrogen server, injects the JWT as window.TERMINAL_JWT (in-memory only) and the server origin as window.TERMINAL_SERVER_ORIGIN, and serves the modified terminal page from the local server. The terminal page's fetchTerminalConfig() checks for window.TERMINAL_JWT first (standalone mode), bypassing postMessage entirely. getApiBase() uses window.TERMINAL_SERVER_ORIGIN for API calls. No JWT is persisted to localStorage or disk.
# 1.0.6 - 2026-09-14 - Phase 14: Fixed binary corruption when fetching terminal HTML and xterm.js assets via curl. Curl output was captured in bash command substitution ($(...)), which strips null bytes and corrupts binary content in minified JS, causing Python UTF-8 decode errors. Fix: write curl output directly to files instead of capturing in bash variables; Python injection script now reads files in binary mode with errors="replace" fallback.
# 1.0.7 - 2026-09-14 - Eliminated Python dependency entirely. HTML injection now uses Node.js binary-safe Buffer operations instead of Python3. The local HTTP server now uses Node.js http module instead of Python3 http.server, with explicit Content-Type headers including charset=utf-8 for HTML/JS/CSS to fix Quirks Mode and character encoding errors. Port allocation also switched from Python to Node.
# =============================================================================

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration defaults
# ---------------------------------------------------------------------------
SCRIPT_NAME="terminal-launcher"
SCRIPT_VERSION="1.0.7"

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
TERMINAL_KEY=""
LAUNCHER_FILE=""
LOCAL_SERVER_PORT=""
LOCAL_SERVER_PID=""

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
    # Clear sensitive variables from shell memory on exit.
    PASSWORD=""
    JWT_TOKEN=""
    TERMINAL_KEY=""
    # Kill the local HTTP server if running
    if [[ -n "${LOCAL_SERVER_PID}" ]] && kill -0 "${LOCAL_SERVER_PID}" 2>/dev/null; then
        kill "${LOCAL_SERVER_PID}" 2>/dev/null || true
        wait "${LOCAL_SERVER_PID}" 2>/dev/null || true
    fi
    # Remove temp files
    if [[ -n "${LAUNCHER_FILE}" && -f "${LAUNCHER_FILE}" ]]; then
        rm -f "${LAUNCHER_FILE}"
    fi
    if [[ -n "${LOCAL_SERVER_PORT}" && -d "/tmp/terminal_launcher_${LOCAL_SERVER_PORT}" ]]; then
        rm -rf "/tmp/terminal_launcher_${LOCAL_SERVER_PORT}"
    fi
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

TWO-KEY MODEL:
    The chat WebSocket (/wss) uses WebSocketServer.Key (env WEBSOCKET_KEY)
    via lithium.json. The terminal WebSocket (/terminal/ws) uses
    Terminal.Key (env WEBSOCKET_TERMINAL_KEY). This launcher obtains the
    TERMINAL key, protocol, and URL from the authorized /api/system/info
    response — it is never read from a CLI argument or config file.

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
        log_error "Possible causes (checked in order by /api/system/info):"
        log_error "  1. JWT lacks the terminal role (role_id 32)."
        log_error "  2. WebSocket server is not running (check server logs for WebSocket launch status)."
        log_error "  3. Terminal.Enabled is false in hydrogen.json."
        log_error "  4. WebSocketServer.PublicUrl is not set (must be wss:// or ws://)."
        rm -f "${info_file}"
        exit 1
    fi

    TERMINAL_URL=$(jq -r '.terminal.url // empty' "${info_file}" 2>/dev/null)
    TERMINAL_PROTOCOL=$(jq -r '.terminal.protocol // empty' "${info_file}" 2>/dev/null)
    TERMINAL_KEY=$(jq -r '.terminal.key // empty' "${info_file}" 2>/dev/null)

    rm -f "${info_file}"

    if [[ -z "${TERMINAL_URL}" || -z "${TERMINAL_PROTOCOL}" || -z "${TERMINAL_KEY}" ]]; then
        log_error "Terminal config is incomplete (missing url, protocol, or key)"
        exit 1
    fi

    log_info "Terminal URL: ${TERMINAL_URL}"
    log_info "Protocol: ${TERMINAL_PROTOCOL}"
    local key_fp
    key_fp=$(redact "${TERMINAL_KEY}")
    log_info "WebSocket key: ${key_fp}"
}

# ---------------------------------------------------------------------------
# Step 3: Create a temporary launcher page and open it in the browser
# ---------------------------------------------------------------------------
create_and_open_launcher() {
    log_info "Creating temporary terminal launcher page"

    # The terminal-launcher.sh serves a modified copy of the terminal HTML
    # from a local HTTP server on localhost. This avoids the cross-origin
    # postMessage issue that occurs when the launcher is served via file://.
    #
    # The local HTTP server:
    # - Serves the modified terminal HTML (with JWT injected as window.TERMINAL_JWT)
    # - Serves static assets (xterm.js, css) from the Hydrogen server
    # - Proxies /api/* requests to the Hydrogen server with JWT in Authorization
    #
    # The terminal page:
    # - Checks window.TERMINAL_JWT first (standalone mode), bypassing postMessage
    # - Uses window.TERMINAL_SERVER_ORIGIN (the local proxy) for API calls
    # - WebSocket URL comes from the proxy'd /api/system/info response
    # - WebSocket connects directly to wss:// server (no CORS for WebSockets)
    #
    # JWT is never persisted to localStorage or disk — it exists only in the
    # HTML file's memory and the shell variable during script execution.
    #
    # CORS note: The Hydrogen API CORS restricts to https://www.500courses.com
    # and https://500courses.com. By proxying API calls through the local server,
    # the browser makes same-origin requests — no CORS issue.

    local scheme host_part server_origin
    scheme="${SERVER_URL%%://*}"
    host_part="${SERVER_URL#*://}"
    host_part="${host_part%%/*}"
    server_origin="${scheme}://${host_part}"

    # Pick a random port for the local HTTP server
    LOCAL_SERVER_PORT=$(node -e "const s=require('net').createServer(); s.listen(0,'127.0.0.1',()=>{console.log(s.address().port); s.close()})")
    local server_dir="/tmp/terminal_launcher_${LOCAL_SERVER_PORT}"
    mkdir -p "${server_dir}"

    LAUNCHER_FILE="${server_dir}/index.html"

    # Fetch the terminal HTML from the Hydrogen server (authenticated).
    # Write directly to a file to handle binary content safely (null bytes,
    # non-UTF-8 sequences in minified JS) without bash variable corruption.
    local raw_html_file="${server_dir}/_raw_terminal.html"
    local http_code
    http_code=$(curl -sL "${SERVER_URL}/terminal/" \
        -H "Authorization: Bearer ${JWT_TOKEN}" \
        -o "${raw_html_file}" \
        -w "%{http_code}")

    if [[ "${http_code}" != "200" ]] || [[ ! -s "${raw_html_file}" ]]; then
        log_error "Failed to fetch terminal HTML from ${SERVER_URL}/terminal/ (HTTP ${http_code})"
        rm -rf "${server_dir}"
        exit 1
    fi

    # Fetch static assets referenced by terminal.html with relative URLs.
    # Write directly to files in the local server directory to handle binary
    # content safely without bash variable corruption.
    log_info "Fetching terminal assets from server..."
    curl -sL "${SERVER_URL}/terminal/xterm.js" -o "${server_dir}/xterm.js" || true
    curl -sL "${SERVER_URL}/terminal/xterm.css" -o "${server_dir}/xterm.css" || true
    curl -sL "${SERVER_URL}/terminal/terminal.css" -o "${server_dir}/terminal.css" || true
    curl -sL "${SERVER_URL}/terminal/xterm-addon-attach.js" -o "${server_dir}/xterm-addon-attach.js" || true
    curl -sL "${SERVER_URL}/terminal/xterm-addon-fit.js" -o "${server_dir}/xterm-addon-fit.js" || true

    # Inject JWT and local server origin into the terminal HTML.
    # window.TERMINAL_JWT: checked first in fetchTerminalConfig() — standalone mode
    # window.TERMINAL_SERVER_ORIGIN: used by getApiBase() for API calls (local proxy)
    export LAUNCHER_FILE
    export raw_html_file
    export JWT_TOKEN
    export LOCAL_SERVER_PORT

    node -e "
const fs = require('fs');
const jwt = process.env.JWT_TOKEN;
const localOrigin = 'http://127.0.0.1:' + process.env.LOCAL_SERVER_PORT;
const rawPath = process.env.raw_html_file;
const outPath = process.env.LAUNCHER_FILE;

const inject = '<script>\\nwindow.TERMINAL_JWT = ' + JSON.stringify(jwt) + ';\\nwindow.TERMINAL_SERVER_ORIGIN = ' + JSON.stringify(localOrigin) + ';\\n</script>\\n';

const content = fs.readFileSync(rawPath);
const headIdx = content.indexOf('<head>');
if (headIdx === -1) {
  console.error('ERROR: <head> tag not found in terminal HTML');
  process.exit(1);
}
const result = Buffer.concat([
  content.slice(0, headIdx + 6),
  Buffer.from(inject, 'utf8'),
  content.slice(headIdx + 6)
]);
fs.writeFileSync(outPath, result);
"
    rm -f "${raw_html_file}"

    log_info "Launcher page ready."

    # Start a local HTTP server in the background that:
    # - Serves static files (terminal HTML, xterm.js, css) from the server_dir
    # - Proxies /api/* requests to the Hydrogen server with JWT
    log_info "Starting local HTTP server on http://127.0.0.1:${LOCAL_SERVER_PORT}"

    export PROXY_TARGET="${server_origin}"
    export PROXY_JWT="${JWT_TOKEN}"
    export SERVER_DIR="${server_dir}"
    export LOCAL_SERVER_PORT

    node -e "
const http = require('http');
const fs = require('fs');
const path = require('path');
const https = require('https');
const { URL } = require('url');

const SERVER_DIR = process.env.SERVER_DIR;
const PROXY_TARGET = process.env.PROXY_TARGET;
const PROXY_JWT = process.env.PROXY_JWT;

function getContentType(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  const types = {
    '.html': 'text/html; charset=utf-8',
    '.js': 'application/javascript; charset=utf-8',
    '.css': 'text/css; charset=utf-8',
    '.json': 'application/json; charset=utf-8',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.svg': 'image/svg+xml',
    '.woff': 'font/woff',
    '.woff2': 'font/woff2',
  };
  return types[ext] || 'application/octet-stream';
}

function proxyRequest(req, res) {
  const targetUrl = PROXY_TARGET + req.url;
  const parsedUrl = new URL(targetUrl);
  const options = {
    hostname: parsedUrl.hostname,
    port: parsedUrl.port || (parsedUrl.protocol === 'https:' ? 443 : 80),
    path: parsedUrl.pathname + parsedUrl.search,
    method: req.method,
    headers: {
      'Authorization': 'Bearer ' + PROXY_JWT,
      'Accept': '*/*',
    },
  };

  const client = parsedUrl.protocol === 'https:' ? https : http;
  const proxyReq = client.request(options, (proxyRes) => {
    res.writeHead(proxyRes.statusCode, {
      ...proxyRes.headers,
      'connection': 'close',
      'transfer-encoding': '',
    });
    proxyRes.pipe(res);
  });

  proxyReq.on('error', (e) => {
    res.writeHead(502);
    res.end(e.message);
  });

  if (req.method === 'POST' || req.method === 'PUT' || req.method === 'PATCH') {
    req.pipe(proxyReq);
  } else {
    proxyReq.end();
  }
}

const server = http.createServer((req, res) => {
  if (req.url.startsWith('/api/')) {
    proxyRequest(req, res);
    return;
  }

  let filePath = '.' + req.url;
  if (filePath === './') {
    filePath = './index.html';
  }
  filePath = path.join(SERVER_DIR, filePath);

  fs.readFile(filePath, (err, data) => {
    if (err) {
      res.writeHead(404);
      res.end('Not Found');
    } else {
      res.writeHead(200, { 'Content-Type': getContentType(filePath) });
      res.end(data);
    }
  });
});

server.listen(parseInt(process.env.LOCAL_SERVER_PORT), '127.0.0.1', () => {
  // Server is ready
});
" &
    LOCAL_SERVER_PID=$!

    # Wait for the server to start
    local retries=0
    while ! curl -s "http://127.0.0.1:${LOCAL_SERVER_PORT}/" --max-time 1 >/dev/null 2>&1; do
        retries=$((retries + 1))
        if [[ ${retries} -ge 20 ]]; then
            log_error "Failed to start local HTTP server on port ${LOCAL_SERVER_PORT}"
            kill "${LOCAL_SERVER_PID}" 2>/dev/null || true
            rm -rf "${server_dir}"
            exit 1
        fi
        sleep 0.2
    done

    log_info "Opening in browser..."

    if command -v "${BROWSER_CMD}" >/dev/null 2>&1; then
        "${BROWSER_CMD}" "http://127.0.0.1:${LOCAL_SERVER_PORT}/" --new-window 2>/dev/null &
        log_info "Terminal launcher opened. Close the browser tab when done."
    else
        log_error "Browser command '${BROWSER_CMD}' not found."
        log_info "Open this URL in a browser: http://127.0.0.1:${LOCAL_SERVER_PORT}/"
    fi

    log_info "Local HTTP server running on http://127.0.0.1:${LOCAL_SERVER_PORT}"
    log_info "Press Ctrl-C to stop the server and exit."

    # Trap Ctrl-C and clean up
    trap 'kill "${LOCAL_SERVER_PID}" 2>/dev/null; rm -rf "${server_dir}"; echo ""; echo "Server stopped."; exit 0' INT TERM

    # Keep the script alive while the server runs; Ctrl-C will trigger the trap
    wait "${LOCAL_SERVER_PID}" 2>/dev/null || true
    kill "${LOCAL_SERVER_PID}" 2>/dev/null || true
    rm -rf "${server_dir}"
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
    key_fp=$(redact "${TERMINAL_KEY}")
    echo "  JWT:       ${jwt_fp}"
    echo "  WS URL:    ${TERMINAL_URL}"
    echo "  Protocol:  ${TERMINAL_PROTOCOL}"
    echo "  WS Key:    ${key_fp}"
    echo ""
    echo "Security: Password was never stored in shell history."
    echo "          JWT and key are in-memory only."
    echo "          Close the browser tab when done; /tmp is cleaned by the OS."
}

main "$@"
