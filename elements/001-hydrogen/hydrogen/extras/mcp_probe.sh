#!/usr/bin/env bash
# mcp_probe.sh - Manual Streamable HTTP probe for a Hydrogen MCP listen.
#
# Usage:
#   extras/mcp_probe.sh --url http://127.0.0.1:15482/mcp --jwt TOKEN
#   extras/mcp_probe.sh --url http://127.0.0.1:15482/mcp --login http://127.0.0.1:15472 \
#       --user USER --pass PASS --api-key KEY
#   extras/mcp_probe.sh ... --call Mcp.Echo '{"message":"hi"}'
#
# Prints PRM GET, unauthenticated 401 WWW-Authenticate, initialize,
# tools/list, resources/list, prompts/list, and tools/call.
# Pretty-prints JSON-RPC. Must pass mks.
#
# CHANGELOG
# 1.1.0 - 2026-08-27 - Probe resources/list and prompts/list (Phase 15)
# 1.0.0 - 2026-08-27 - Initial probe

set -euo pipefail

usage() {
    cat <<'EOF'
Usage: mcp_probe.sh --url URL --jwt TOKEN
       mcp_probe.sh --url URL --login LOGIN_BASE [--user U --pass P --api-key K]

Options:
  --url URL          MCP Path URL (e.g. http://127.0.0.1:15482/mcp)
  --jwt TOKEN        Bearer token
  --login BASE       WebServer origin for POST /api/auth/login
  --user NAME        Login id (default HYDROGEN_DEMO_USER_NAME)
  --pass PASS        Password (default HYDROGEN_DEMO_USER_PASS)
  --api-key KEY      API key (default HYDROGEN_DEMO_API_KEY)
  --call NAME JSON   tools/call name and arguments JSON object
  -h, --help         Show this help
EOF
}

url=""
jwt=""
login_base=""
user="${HYDROGEN_DEMO_USER_NAME:-}"
pass="${HYDROGEN_DEMO_USER_PASS:-}"
api_key="${HYDROGEN_DEMO_API_KEY:-}"
call_name="Mcp.Echo"
call_args='{"message":"probe"}'

while [[ $# -gt 0 ]]; do
    case "$1" in
        --url) url="${2:-}"; shift 2 ;;
        --jwt) jwt="${2:-}"; shift 2 ;;
        --login) login_base="${2:-}"; shift 2 ;;
        --user) user="${2:-}"; shift 2 ;;
        --pass) pass="${2:-}"; shift 2 ;;
        --api-key) api_key="${2:-}"; shift 2 ;;
        --call)
            call_name="${2:-}"
            call_args="${3:-}"
            shift 3
            ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
    esac
done

if [[ -z "${url}" ]]; then
    echo "error: --url is required" >&2
    usage >&2
    exit 1
fi

if [[ -n "${login_base}" && -z "${jwt}" ]]; then
    login_payload=$(jq -n \
        --arg login_id "${user}" \
        --arg password "${pass}" \
        --arg api_key "${api_key}" \
        '{database:"Acuranzo",login_id:$login_id,password:$password,api_key:$api_key,tz:"America/Vancouver"}')
    login_body=$(mktemp)
    login_st=$(curl -s -X POST -H "Content-Type: application/json" \
        --connect-timeout 10 --max-time 45 \
        -d "${login_payload}" -o "${login_body}" -w "%{http_code}" \
        "${login_base}/api/auth/login" || true)
    jwt=$(jq -r '.token // empty' "${login_body}" 2>/dev/null || true)
    rm -f "${login_body}"
    if [[ "${login_st}" != "200" || -z "${jwt}" ]]; then
        echo "error: login failed HTTP ${login_st:-000}" >&2
        exit 1
    fi
    echo "login: HTTP ${login_st}"
fi

if [[ -z "${jwt}" ]]; then
    echo "error: --jwt or --login is required" >&2
    usage >&2
    exit 1
fi

origin="${url}"
origin="${origin%/mcp}"
origin="${origin%/}"

echo "=== GET ${origin}/.well-known/oauth-protected-resource ==="
curl -sS --connect-timeout 5 --max-time 15 \
    "${origin}/.well-known/oauth-protected-resource" | jq .

echo "=== POST ${url} (no auth) ==="
hdr=$(mktemp)
body=$(mktemp)
st=$(curl -sS -D "${hdr}" -o "${body}" -w "%{http_code}" \
    -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}' \
    "${url}" || true)
echo "HTTP ${st}"
grep -i '^WWW-Authenticate:' "${hdr}" || true
jq . "${body}" 2>/dev/null || cat "${body}"
echo

echo "=== initialize ==="
st=$(curl -sS -D "${hdr}" -o "${body}" -w "%{http_code}" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer ${jwt}" \
    -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"mcp_probe","version":"1.0.0"}}}' \
    "${url}" || true)
session=$(grep -i '^Mcp-Session-Id:' "${hdr}" | head -1 | cut -d: -f2- | tr -d '\r' | sed 's/^[[:space:]]*//' || true)
echo "HTTP ${st} session=${session}"
jq . "${body}" 2>/dev/null || cat "${body}"
echo

echo "=== tools/list ==="
st=$(curl -sS -D "${hdr}" -o "${body}" -w "%{http_code}" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer ${jwt}" \
    -H "Mcp-Session-Id: ${session}" \
    -d '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}' \
    "${url}" || true)
echo "HTTP ${st}"
jq . "${body}" 2>/dev/null || cat "${body}"
echo

echo "=== resources/list ==="
st=$(curl -sS -D "${hdr}" -o "${body}" -w "%{http_code}" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer ${jwt}" \
    -H "Mcp-Session-Id: ${session}" \
    -d '{"jsonrpc":"2.0","id":4,"method":"resources/list","params":{}}' \
    "${url}" || true)
echo "HTTP ${st}"
jq . "${body}" 2>/dev/null || cat "${body}"
echo

echo "=== prompts/list ==="
st=$(curl -sS -D "${hdr}" -o "${body}" -w "%{http_code}" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer ${jwt}" \
    -H "Mcp-Session-Id: ${session}" \
    -d '{"jsonrpc":"2.0","id":5,"method":"prompts/list","params":{}}' \
    "${url}" || true)
echo "HTTP ${st}"
jq . "${body}" 2>/dev/null || cat "${body}"
echo

call_payload=$(jq -n --arg name "${call_name}" --argjson args "${call_args}" \
    '{jsonrpc:"2.0",id:3,method:"tools/call",params:{name:$name,arguments:$args}}')
echo "=== tools/call ${call_name} ==="
st=$(curl -sS -D "${hdr}" -o "${body}" -w "%{http_code}" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer ${jwt}" \
    -H "Mcp-Session-Id: ${session}" \
    -d "${call_payload}" \
    "${url}" || true)
echo "HTTP ${st}"
jq . "${body}" 2>/dev/null || cat "${body}"

rm -f "${hdr}" "${body}"
