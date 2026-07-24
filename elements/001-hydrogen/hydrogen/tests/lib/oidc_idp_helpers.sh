#!/usr/bin/env bash
# OIDC IdP blackbox helpers for test_45_oidc_idp.sh
# shellcheck shell=bash # Explicit bash for pipefail and arrays used by callers
# Temp paths use BASHPID+RANDOM so parallel engine jobs do not clobber each other.

# Sourced library marker (mirrors FRAMEWORK_GUARD pattern)
OIDC_IDP_HELPERS_GUARD=1
export OIDC_IDP_HELPERS_GUARD

# PKCE S256: verifier (43-128 unreserved) → challenge
# Sets OIDC_IDP_CODE_VERIFIER and OIDC_IDP_CODE_CHALLENGE (exported for callers)
oidc_idp_pkce_pair() {
    local verifier b64_raw challenge_b64
    b64_raw="$(openssl rand -base64 48)" || return 1
    # base64url without piping (avoids SC2312 masking)
    verifier="${b64_raw//+/-}"
    verifier="${verifier//\//_}"
    verifier="${verifier//=/}"
    OIDC_IDP_CODE_VERIFIER="${verifier}"
    export OIDC_IDP_CODE_VERIFIER
    local tmp_v tmp_d uniq
    uniq="${BASHPID:-$$}_${RANDOM}"
    tmp_v="/tmp/oidc_idp_pkce_v_${uniq}"
    tmp_d="/tmp/oidc_idp_pkce_d_${uniq}"
    printf '%s' "${verifier}" > "${tmp_v}" || return 1
    openssl dgst -binary -sha256 -out "${tmp_d}" "${tmp_v}" || {
        rm -f "${tmp_v}" "${tmp_d}"
        return 1
    }
    challenge_b64="$(openssl base64 -A -in "${tmp_d}")" || {
        rm -f "${tmp_v}" "${tmp_d}"
        return 1
    }
    rm -f "${tmp_v}" "${tmp_d}"
    OIDC_IDP_CODE_CHALLENGE="${challenge_b64//+/-}"
    OIDC_IDP_CODE_CHALLENGE="${OIDC_IDP_CODE_CHALLENGE//\//_}"
    OIDC_IDP_CODE_CHALLENGE="${OIDC_IDP_CODE_CHALLENGE//=/}"
    export OIDC_IDP_CODE_CHALLENGE
}

oidc_idp_http_code() {
    local url="$1"
    shift
    local body
    body="/tmp/oidc_idp_body_${BASHPID:-$$}_${RANDOM}.out"
    curl -sS -o "${body}" -w '%{http_code}' "$@" "${url}" 2>/dev/null || echo "000"
    rm -f "${body}"
}

oidc_idp_get_body() {
    # Deprecated for parallel use; callers should pass explicit out files
    true
}

oidc_idp_cleanup_tmp() {
    rm -f /tmp/oidc_idp_body_* /tmp/oidc_idp_hdr_* /tmp/oidc_idp_pkce_* 2>/dev/null || true
}

# Returns Location header value (first) from curl -D headers file
oidc_idp_location_from_headers() {
    local hdr_file="$1"
    awk 'BEGIN{IGNORECASE=1} /^Location:/{sub(/\r$/,""); sub(/^Location:[[:space:]]*/,""); print; exit}' "${hdr_file}" 2>/dev/null
}

# Parse code= and state= from redirect URL query
# Sets OIDC_IDP_AUTH_CODE and OIDC_IDP_STATE_OUT (exported)
oidc_idp_parse_code_from_location() {
    local location="$1"
    local query="${location#*\?}"
    OIDC_IDP_AUTH_CODE=""
    OIDC_IDP_STATE_OUT=""
    local part
    local -a parts
    IFS='&' read -ra parts <<< "${query}"
    for part in "${parts[@]}"; do
        case "${part}" in
            code=*)
                OIDC_IDP_AUTH_CODE="${part#code=}"
                ;;
            state=*)
                OIDC_IDP_STATE_OUT="${part#state=}"
                ;;
            *)
                ;;
        esac
    done
    export OIDC_IDP_AUTH_CODE
    export OIDC_IDP_STATE_OUT
}

# GET discovery document → file
oidc_idp_fetch_discovery() {
    local base_url="$1"
    local out_file="$2"
    local code
    code="$(curl -sS -o "${out_file}" -w '%{http_code}' "${base_url}/.well-known/openid-configuration" 2>/dev/null || echo "000")"
    echo "${code}"
}

oidc_idp_fetch_jwks() {
    local base_url="$1"
    local out_file="$2"
    local code
    code="$(curl -sS -o "${out_file}" -w '%{http_code}' "${base_url}/oauth/jwks" 2>/dev/null || echo "000")"
    echo "${code}"
}

# POST token (public client)
oidc_idp_token_code() {
    local base_url="$1"
    local client_id="$2"
    local redirect_uri="$3"
    local code="$4"
    local verifier="$5"
    local out_file="$6"
    curl -sS -o "${out_file}" -w '%{http_code}' \
        -X POST "${base_url}/oauth/token" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        --data-urlencode "grant_type=authorization_code" \
        --data-urlencode "code=${code}" \
        --data-urlencode "redirect_uri=${redirect_uri}" \
        --data-urlencode "client_id=${client_id}" \
        --data-urlencode "code_verifier=${verifier}" 2>/dev/null || echo "000"
}

oidc_idp_token_refresh() {
    local base_url="$1"
    local client_id="$2"
    local refresh="$3"
    local out_file="$4"
    curl -sS -o "${out_file}" -w '%{http_code}' \
        -X POST "${base_url}/oauth/token" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        --data-urlencode "grant_type=refresh_token" \
        --data-urlencode "refresh_token=${refresh}" \
        --data-urlencode "client_id=${client_id}" 2>/dev/null || echo "000"
}

oidc_idp_userinfo() {
    local base_url="$1"
    local access_token="$2"
    local out_file="$3"
    curl -sS -o "${out_file}" -w '%{http_code}' \
        -H "Authorization: Bearer ${access_token}" \
        "${base_url}/oauth/userinfo" 2>/dev/null || echo "000"
}

# Full authorize login POST → sets OIDC_IDP_AUTH_CODE on success (302)
oidc_idp_authorize_login() {
    local base_url="$1"
    local client_id="$2"
    local redirect_uri="$3"
    local scope="$4"
    local state="$5"
    local nonce="$6"
    local challenge="$7"
    local username="$8"
    local password="$9"
    local hdr_file="/tmp/oidc_idp_hdr_${BASHPID:-$$}_${RANDOM}.out"

    curl -sS -D "${hdr_file}" -o /dev/null \
        -X POST "${base_url}/oauth/authorize" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        --data-urlencode "client_id=${client_id}" \
        --data-urlencode "redirect_uri=${redirect_uri}" \
        --data-urlencode "response_type=code" \
        --data-urlencode "scope=${scope}" \
        --data-urlencode "state=${state}" \
        --data-urlencode "nonce=${nonce}" \
        --data-urlencode "code_challenge=${challenge}" \
        --data-urlencode "code_challenge_method=S256" \
        --data-urlencode "username=${username}" \
        --data-urlencode "password=${password}" 2>/dev/null || true

    local location
    location="$(oidc_idp_location_from_headers "${hdr_file}")"
    rm -f "${hdr_file}"
    if [[ -z "${location}" ]]; then
        OIDC_IDP_AUTH_CODE=""
        export OIDC_IDP_AUTH_CODE
        return 1
    fi
    oidc_idp_parse_code_from_location "${location}"
    [[ -n "${OIDC_IDP_AUTH_CODE}" ]]
}

# JWT payload middle segment → JSON string (base64url)
oidc_idp_jwt_payload_json() {
    local jwt="$1"
    printf '%s' "${jwt}" | cut -d. -f2 | python3 -c '
import sys, base64
s = sys.stdin.read().strip()
s += "=" * ((4 - len(s) % 4) % 4)
sys.stdout.write(base64.urlsafe_b64decode(s.encode()).decode())
' 2>/dev/null || true
}
