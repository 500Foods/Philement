#!/usr/bin/env bash

# Test: Reporting Image Scale Endpoint
# Tests POST /api/reporting/image_scale with JWT auth, format conversion,
# dimension units, XO output, and error paths against a live Hydrogen server.

# FUNCTIONS
# api_request()
# file_to_b64()
# wait_for_ready_for_requests()
# acquire_jwt()
# post_image_scale()
# assert_success_response()
# run_image_scale_cases()

# CHANGELOG
# 1.4.0 - 2026-08-20 - Replace python3 oversized-input generator with head/tr
# 1.3.0 - 2026-07-29 - Fix: replace bare `exit` with ORCHESTRATION guard so that
#                       test_00_all.sh can collect results when test is sourced
#                       in a background subshell (run_single_test_parallel).
# 1.2.0 - 2026-07-29 - Phase 6: oversized dimensions (400) and MaxInputBytes (413).
# 1.1.0 - 2026-07-29 - Phase 5: SVG→PNG, PNG alpha→JPEG flatten checks.
# 1.0.0 - 2026-07-29 - Initial blackbox coverage for Reporting image_scale (IMAGE_PLAN Phase 3).

set -euo pipefail

# Test configuration
TEST_NAME="Reporting Image Scale"
TEST_ABBR="RIS"
TEST_NUMBER="27"
TEST_COUNTER=0
TEST_VERSION="1.4.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_27_reporting.json"
IMAGES_DIR="${SCRIPT_DIR}/artifacts/images"
STARTUP_TIMEOUT=30
SHUTDOWN_TIMEOUT=15
SHUTDOWN_ACTIVITY_TIMEOUT=5
READY_TIMEOUT=45
CURL_MAX_TIME=60

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_COUNT=0

# Generic HTTP request helper. Prints HTTP status code to stdout.
api_request() {
    local method="$1"
    local url="$2"
    local data="$3"
    local output_file="$4"
    local jwt_token="${5:-}"
    local max_retries=3
    local retry=1
    while [[ "${retry}" -le "${max_retries}" ]]; do
        local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json")
        if [[ -n "${jwt_token}" ]]; then
            curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
        fi
        if [[ "${method}" != "GET" ]]; then
            curl_cmd+=(-d "${data}")
        fi
        curl_cmd+=(-w "%{http_code}" -o "${output_file}" --connect-timeout 5 --max-time "${CURL_MAX_TIME}" "${url}")
        local http_status
        http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
        http_status=${http_status:-"000"}
        if [[ "${http_status}" == "200" ]] || \
           [[ "${http_status}" == "201" ]] || \
           [[ "${http_status}" == "400" ]] || \
           [[ "${http_status}" == "401" ]] || \
           [[ "${http_status}" == "403" ]] || \
           [[ "${http_status}" == "404" ]] || \
           [[ "${http_status}" == "413" ]] || \
           [[ "${http_status}" == "500" ]] || \
           [[ "${http_status}" == "503" ]]; then
            echo "${http_status}"
            return 0
        fi
        if [[ "${retry}" -eq "${max_retries}" ]]; then
            echo "${http_status}"
            return 0
        fi
        sleep 0.5
        retry=$(( retry + 1 ))
    done
    echo "000"
}

file_to_b64() {
    local path="$1"
    base64 -w0 "${path}" 2>/dev/null || base64 "${path}" | tr -d '\n'
}

wait_for_ready_for_requests() {
    local log_file="$1"
    local timeout="${2:-${READY_TIMEOUT}}"
    local start_time
    start_time=${SECONDS}
    while [[ $((SECONDS - start_time)) -lt "${timeout}" ]]; do
        if "${GREP}" -q "READY FOR REQUESTS" "${log_file}" 2>/dev/null; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

record_pass() {
    local name="$1"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
    TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${name}"
}

record_fail() {
    local name="$1"
    local detail="${2:-}"
    FAIL_COUNT=$(( FAIL_COUNT + 1 ))
    TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
    if [[ -n "${detail}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${detail}"
    fi
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${name}"
    EXIT_CODE=1
}

acquire_jwt() {
    local base_url="$1"
    local response_file="$2"
    local login_data
    login_data=$(cat <<EOF
{
    "database": "Acuranzo",
    "login_id": "${HYDROGEN_DEMO_USER_NAME}",
    "password": "${HYDROGEN_DEMO_USER_PASS}",
    "api_key": "${HYDROGEN_DEMO_API_KEY}",
    "tz": "America/Vancouver"
}
EOF
)
    local status
    status=$(api_request "POST" "${base_url}/api/auth/login" "${login_data}" "${response_file}" "")
    if [[ "${status}" != "200" ]]; then
        return 1
    fi
    local token
    token=$(jq -r '.token // empty' "${response_file}" 2>/dev/null || true)
    if [[ -z "${token}" || "${token}" == "null" ]]; then
        return 1
    fi
    echo "${token}"
}

# Build JSON body and POST to image_scale. Writes response to output_file; echoes HTTP status.
post_image_scale() {
    local base_url="$1"
    local jwt_token="$2"
    local body_file="$3"
    local output_file="$4"
    local status
    if [[ -n "${jwt_token}" ]]; then
        status=$(curl -s -X POST \
            -H "Content-Type: application/json" \
            -H "Authorization: Bearer ${jwt_token}" \
            --connect-timeout 5 --max-time "${CURL_MAX_TIME}" \
            -w "%{http_code}" -o "${output_file}" \
            -d @"${body_file}" \
            "${base_url}/api/reporting/image_scale" 2>/dev/null || true)
    else
        status=$(curl -s -X POST \
            -H "Content-Type: application/json" \
            --connect-timeout 5 --max-time "${CURL_MAX_TIME}" \
            -w "%{http_code}" -o "${output_file}" \
            -d @"${body_file}" \
            "${base_url}/api/reporting/image_scale" 2>/dev/null || true)
    fi
    status=${status:-"000"}
    echo "${status}"
}

# image_src is a path to a raw base64 text file (avoids ARG_MAX on large images).
write_scale_body() {
    local out_path="$1"
    local image_src="$2"
    local format="$3"
    local width="$4"
    local height="$5"
    local units="${6:-px}"
    local dpi="${7:-72}"
    local algo="${8:-lanczos}"
    jq -n \
        --rawfile image "${image_src}" \
        --arg format "${format}" \
        --argjson width "${width}" \
        --argjson height "${height}" \
        --arg units "${units}" \
        --argjson dpi "${dpi}" \
        --arg scale_algorithm "${algo}" \
        '{image:($image | gsub("\n";"")), format:$format, width:$width, height:$height, units:$units, dpi:$dpi, scale_algorithm:$scale_algorithm}' \
        > "${out_path}"
}

assert_success_fields() {
    local response_file="$1"
    local expect_format="$2"
    local expect_w="$3"
    local expect_h="$4"
    local expect_mime="${5:-}"

    local success format width height mime image
    success=$(jq -r '.success // false' "${response_file}" 2>/dev/null || echo "false")
    format=$(jq -r '.format // empty' "${response_file}" 2>/dev/null || echo "")
    width=$(jq -r '.width // empty' "${response_file}" 2>/dev/null || echo "")
    height=$(jq -r '.height // empty' "${response_file}" 2>/dev/null || echo "")
    mime=$(jq -r '.mime_type // empty' "${response_file}" 2>/dev/null || echo "")
    image=$(jq -r '.image // empty' "${response_file}" 2>/dev/null || echo "")

    if [[ "${success}" != "true" ]]; then
        echo "success=${success}"
        return 1
    fi
    if [[ "${format}" != "${expect_format}" ]]; then
        echo "format=${format} expected=${expect_format}"
        return 1
    fi
    if [[ "${width}" != "${expect_w}" || "${height}" != "${expect_h}" ]]; then
        echo "dims=${width}x${height} expected=${expect_w}x${expect_h}"
        return 1
    fi
    if [[ -n "${expect_mime}" && "${mime}" != "${expect_mime}" ]]; then
        echo "mime=${mime} expected=${expect_mime}"
        return 1
    fi
    if [[ -z "${image}" || "${image}" == "null" ]]; then
        echo "missing image payload"
        return 1
    fi
    return 0
}

run_image_scale_cases() {
    local base_url="$1"
    local jwt_token="$2"
    local work_dir="$3"

    local png_b64f jpg_b64f bmp_b64f svg_b64f bad_b64f
    png_b64f="${work_dir}/sample.png.b64"
    jpg_b64f="${work_dir}/sample.jpg.b64"
    bmp_b64f="${work_dir}/sample.bmp.b64"
    svg_b64f="${work_dir}/sample.svg.b64"
    bad_b64f="${work_dir}/bad.b64"
    file_to_b64 "${IMAGES_DIR}/sample.png" > "${png_b64f}"
    file_to_b64 "${IMAGES_DIR}/sample.jpg" > "${jpg_b64f}"
    file_to_b64 "${IMAGES_DIR}/sample.bmp" > "${bmp_b64f}"
    file_to_b64 "${IMAGES_DIR}/sample.svg" > "${svg_b64f}"
    printf '%s' 'not-valid-base64!!!' > "${bad_b64f}"

    # 1. PNG to PNG scale
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "PNG to PNG scale (128x128)"
    local body="${work_dir}/body_png.json"
    local resp="${work_dir}/resp_png.json"
    write_scale_body "${body}" "${png_b64f}" "png" 128 128 "px" 72 "lanczos"
    local status detail="" assert_rc=0
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    # shellcheck disable=SC2310 # Capture assert failure without aborting under set -e
    detail=$(assert_success_fields "${resp}" "png" "128" "128" "image/png") || assert_rc=$?
    if [[ "${status}" == "200" && "${assert_rc}" -eq 0 ]]; then
        record_pass "PNG to PNG scale"
    else
        record_fail "PNG to PNG scale" "HTTP ${status} ${detail:-} body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 2. BMP to PNG scale
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "BMP to PNG scale (200x200)"
    body="${work_dir}/body_bmp.json"
    resp="${work_dir}/resp_bmp.json"
    write_scale_body "${body}" "${bmp_b64f}" "png" 200 200 "px" 72 "lanczos"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    detail=""
    assert_rc=0
    # shellcheck disable=SC2310 # Capture assert failure without aborting under set -e
    detail=$(assert_success_fields "${resp}" "png" "200" "200" "image/png") || assert_rc=$?
    if [[ "${status}" == "200" && "${assert_rc}" -eq 0 ]]; then
        record_pass "BMP to PNG scale"
    else
        record_fail "BMP to PNG scale" "HTTP ${status} ${detail:-} body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 3. JPEG to WebP
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JPEG to WebP"
    body="${work_dir}/body_webp.json"
    resp="${work_dir}/resp_webp.json"
    write_scale_body "${body}" "${jpg_b64f}" "webp" 256 256 "px" 72 "lanczos"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    detail=""
    assert_rc=0
    # shellcheck disable=SC2310 # Capture assert failure without aborting under set -e
    detail=$(assert_success_fields "${resp}" "webp" "256" "256" "image/webp") || assert_rc=$?
    if [[ "${status}" == "200" && "${assert_rc}" -eq 0 ]]; then
        record_pass "JPEG to WebP"
    else
        record_fail "JPEG to WebP" "HTTP ${status} ${detail:-} body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 4. Point units: 2 inches = 144 pt at 300 DPI → 600x600 px (px = pt * dpi / 72)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Point units (2in @ 300 DPI → 600x600)"
    body="${work_dir}/body_pt.json"
    resp="${work_dir}/resp_pt.json"
    write_scale_body "${body}" "${png_b64f}" "png" 144 144 "pt" 300 "lanczos"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    detail=""
    assert_rc=0
    # shellcheck disable=SC2310 # Capture assert failure without aborting under set -e
    detail=$(assert_success_fields "${resp}" "png" "600" "600" "image/png") || assert_rc=$?
    if [[ "${status}" == "200" && "${assert_rc}" -eq 0 ]]; then
        record_pass "Point units conversion"
    else
        record_fail "Point units conversion" "HTTP ${status} ${detail:-} body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 5. XO format
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "XO format output"
    body="${work_dir}/body_xo.json"
    resp="${work_dir}/resp_xo.json"
    write_scale_body "${body}" "${png_b64f}" "xo" 32 32 "px" 72 "nearest"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    local xo_ok=0
    detail=""
    if [[ "${status}" == "200" ]]; then
        local xo_b64 xo_bin
        xo_b64=$(jq -r '.image // empty' "${resp}" 2>/dev/null || true)
        local success_xo mime_xo
        success_xo=$(jq -r '.success // false' "${resp}" 2>/dev/null || echo "false")
        mime_xo=$(jq -r '.mime_type // empty' "${resp}" 2>/dev/null || echo "")
        if [[ "${success_xo}" == "true" && -n "${xo_b64}" && "${mime_xo}" == "application/x-xo" ]]; then
            xo_bin="${work_dir}/xo.bin"
            if printf '%s' "${xo_b64}" | base64 -d > "${xo_bin}" 2>/dev/null; then
                local hdr_w hdr_h
                hdr_w=$(od -An -tx1 -N4 -j0 "${xo_bin}" 2>/dev/null | tr -d ' \n' || true)
                hdr_h=$(od -An -tx1 -N4 -j4 "${xo_bin}" 2>/dev/null | tr -d ' \n' || true)
                if [[ "${hdr_w}" == "00000020" && "${hdr_h}" == "00000020" ]]; then
                    xo_ok=1
                else
                    detail="header w=${hdr_w} h=${hdr_h}"
                fi
            else
                detail="base64 decode of XO failed"
            fi
        else
            detail="success=${success_xo} mime=${mime_xo}"
        fi
    else
        detail="HTTP ${status}"
    fi
    if [[ "${xo_ok}" -eq 1 ]]; then
        record_pass "XO format output"
    else
        record_fail "XO format output" "${detail:-unknown}"
    fi

    # 6. Invalid image
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Invalid image data"
    body="${work_dir}/body_badimg.json"
    resp="${work_dir}/resp_badimg.json"
    write_scale_body "${body}" "${bad_b64f}" "png" 64 64 "px" 72 "lanczos"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    if [[ "${status}" == "400" ]]; then
        record_pass "Invalid image data"
    else
        record_fail "Invalid image data" "HTTP ${status} expected 400 body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 7. Missing parameter (omit format)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing format parameter"
    body="${work_dir}/body_missing.json"
    resp="${work_dir}/resp_missing.json"
    jq -n --rawfile image "${png_b64f}" --argjson width 64 --argjson height 64 \
        '{image:($image | gsub("\n";"")), width:$width, height:$height}' > "${body}"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    if [[ "${status}" == "400" ]]; then
        record_pass "Missing format parameter"
    else
        record_fail "Missing format parameter" "HTTP ${status} expected 400"
    fi

    # 8. Invalid / not-allowed format name
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Invalid format"
    body="${work_dir}/body_invfmt.json"
    resp="${work_dir}/resp_invfmt.json"
    write_scale_body "${body}" "${png_b64f}" "not_a_real_format" 64 64 "px" 72 "lanczos"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    if [[ "${status}" == "400" ]]; then
        record_pass "Invalid format"
    else
        record_fail "Invalid format" "HTTP ${status} expected 400 body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 9. Unsupported format (gif known but not in AllowedFormats)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Unsupported format (gif not allowed)"
    body="${work_dir}/body_unsup.json"
    resp="${work_dir}/resp_unsup.json"
    write_scale_body "${body}" "${png_b64f}" "gif" 64 64 "px" 72 "lanczos"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    if [[ "${status}" == "400" ]]; then
        record_pass "Unsupported format"
    else
        record_fail "Unsupported format" "HTTP ${status} expected 400"
    fi

    # 10. Auth required (no JWT)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Auth required (no JWT)"
    body="${work_dir}/body_noauth.json"
    resp="${work_dir}/resp_noauth.json"
    write_scale_body "${body}" "${png_b64f}" "png" 32 32 "px" 72 "nearest"
    status=$(post_image_scale "${base_url}" "" "${body}" "${resp}")
    if [[ "${status}" == "401" ]]; then
        record_pass "Auth required (no JWT)"
    else
        record_fail "Auth required (no JWT)" "HTTP ${status} expected 401"
    fi

    # 11. Scale algorithms
    local algo
    for algo in nearest bilinear bicubic lanczos; do
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Scale algorithm: ${algo}"
        body="${work_dir}/body_algo_${algo}.json"
        resp="${work_dir}/resp_algo_${algo}.json"
        write_scale_body "${body}" "${png_b64f}" "png" 48 48 "px" 72 "${algo}"
        status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
        detail=""
        assert_rc=0
        # shellcheck disable=SC2310 # Capture assert failure without aborting under set -e
        detail=$(assert_success_fields "${resp}" "png" "48" "48" "image/png") || assert_rc=$?
        if [[ "${status}" == "200" && "${assert_rc}" -eq 0 ]]; then
            record_pass "Scale algorithm ${algo}"
        else
            record_fail "Scale algorithm ${algo}" "HTTP ${status} ${detail:-}"
        fi
    done

    # 12. SVG input → PNG (density/dpi rasterization path)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "SVG input to PNG"
    body="${work_dir}/body_svg.json"
    resp="${work_dir}/resp_svg.json"
    write_scale_body "${body}" "${svg_b64f}" "png" 100 100 "px" 96 "lanczos"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    detail=""
    assert_rc=0
    # shellcheck disable=SC2310 # Capture assert failure without aborting under set -e
    detail=$(assert_success_fields "${resp}" "png" "100" "100" "image/png") || assert_rc=$?
    if [[ "${status}" == "200" && "${assert_rc}" -eq 0 ]]; then
        local out_png out_fmt
        out_png="${work_dir}/out_from_svg.png"
        jq -r '.image' "${resp}" | base64 -d > "${out_png}" 2>/dev/null || true
        out_fmt=$(file -b --mime-type "${out_png}" 2>/dev/null || echo "")
        if [[ "${out_fmt}" == "image/png" ]]; then
            record_pass "SVG input to PNG"
        else
            record_fail "SVG input to PNG" "decoded mime=${out_fmt}"
        fi
    else
        record_fail "SVG input to PNG" "HTTP ${status} ${detail:-} body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 13. PNG with alpha → JPEG (flatten onto white; no alpha in output)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "PNG alpha to JPEG flatten"
    body="${work_dir}/body_alpha_jpg.json"
    resp="${work_dir}/resp_alpha_jpg.json"
    write_scale_body "${body}" "${png_b64f}" "jpg" 64 64 "px" 72 "lanczos"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    detail=""
    assert_rc=0
    # shellcheck disable=SC2310 # Capture assert failure without aborting under set -e
    detail=$(assert_success_fields "${resp}" "jpg" "64" "64" "image/jpeg") || assert_rc=$?
    if [[ "${status}" == "200" && "${assert_rc}" -eq 0 ]]; then
        local out_jpg alpha_ch
        out_jpg="${work_dir}/out_alpha.jpg"
        jq -r '.image' "${resp}" | base64 -d > "${out_jpg}" 2>/dev/null || true
        # identify -format %A returns True/False/Undefined for alpha
        alpha_ch=$(identify -format '%A' "${out_jpg}" 2>/dev/null || echo "err")
        if [[ "${alpha_ch}" == "False" || "${alpha_ch}" == "Undefined" ]]; then
            record_pass "PNG alpha to JPEG flatten"
        else
            record_fail "PNG alpha to JPEG flatten" "alpha channel=${alpha_ch}"
        fi
    else
        record_fail "PNG alpha to JPEG flatten" "HTTP ${status} ${detail:-} body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 14. Oversized dimensions (MaxImageSize is 8192 in test config)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Oversized dimensions (9000 > MaxImageSize)"
    body="${work_dir}/body_bigdim.json"
    resp="${work_dir}/resp_bigdim.json"
    write_scale_body "${body}" "${png_b64f}" "png" 9000 9000 "px" 72 "nearest"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    local err_msg
    err_msg=$(jq -r '.error // empty' "${resp}" 2>/dev/null || true)
    if [[ "${status}" == "400" && "${err_msg}" == *"imension"* ]]; then
        record_pass "Oversized dimensions rejected"
    elif [[ "${status}" == "400" ]]; then
        record_pass "Oversized dimensions rejected"
    else
        record_fail "Oversized dimensions" "HTTP ${status} expected 400 body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi

    # 15. Oversized base64 input (MaxInputBytes=9e6 in test config; stay under API_MAX_POST_SIZE 10MiB)
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Oversized input (MaxInputBytes)"
    body="${work_dir}/body_bigin.json"
    resp="${work_dir}/resp_bigin.json"
    local big_b64f="${work_dir}/big.b64"
    # 9.5e6 > MaxInputBytes 9e6; total JSON still under API_MAX_POST_SIZE (10485760)
    head -c 9500000 /dev/zero | tr '\0' 'A' > "${big_b64f}"
    write_scale_body "${body}" "${big_b64f}" "png" 32 32 "px" 72 "nearest"
    status=$(post_image_scale "${base_url}" "${jwt_token}" "${body}" "${resp}")
    # 413 Content Too Large from reporting handler
    if [[ "${status}" == "413" ]]; then
        record_pass "Oversized input rejected (HTTP 413)"
    elif [[ "${status}" == "400" ]]; then
        record_pass "Oversized input rejected (HTTP 400)"
    else
        record_fail "Oversized input" "HTTP ${status} expected 413/400 body=$(head -c 200 "${resp}" 2>/dev/null || true)"
    fi
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate configuration and artifacts"

if [[ ! -f "${CONFIG_FILE}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Config missing: ${CONFIG_FILE}"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    exit "${EXIT_CODE}"
fi

missing_imgs=0
for f in sample.png sample.bmp sample.jpg sample.webp sample.ico sample.svg; do
    if [[ ! -f "${IMAGES_DIR}/${f}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Missing image artifact: ${f}"
        missing_imgs=1
    fi
done
if [[ "${missing_imgs}" -ne 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Image artifacts incomplete"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    exit "${EXIT_CODE}"
fi

if [[ -z "${HYDROGEN_DEMO_USER_NAME:-}" || -z "${HYDROGEN_DEMO_USER_PASS:-}" || -z "${HYDROGEN_DEMO_API_KEY:-}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Demo credentials env vars not set"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    exit "${EXIT_CODE}"
fi

print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration and artifacts OK"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen binary"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if ! find_hydrogen_binary "${PROJECT_DIR}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen binary not found"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    exit "${EXIT_CODE}"
fi
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Binary: ${HYDROGEN_BIN_BASE}"

SERVER_PORT=$(get_webserver_port "${CONFIG_FILE}")
BASE_URL="http://127.0.0.1:${SERVER_PORT}"
SERVER_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_reporting.log"
RESULT_DIR="${LOG_PREFIX}${TIMESTAMP}_reporting"
mkdir -p "${RESULT_DIR}"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen server (port ${SERVER_PORT})"
HYDROGEN_PID=""
temp_pid_var="HYDROGEN_PID_$$"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if start_hydrogen_with_pid "${CONFIG_FILE}" "${SERVER_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${temp_pid_var}"; then
    HYDROGEN_PID=$(eval "echo \$${temp_pid_var}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started PID ${HYDROGEN_PID}"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server failed to start"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    exit "${EXIT_CODE}"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Wait for HTTP ready"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if wait_for_server_ready "${BASE_URL}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "HTTP ready"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "HTTP not ready"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Wait for READY FOR REQUESTS"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if wait_for_ready_for_requests "${SERVER_LOG}" "${READY_TIMEOUT}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Database ready"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "READY FOR REQUESTS not seen"
    EXIT_CODE=1
fi

JWT_TOKEN=""
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Acquire JWT via /api/auth/login"
    login_resp="${RESULT_DIR}/login.json"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if JWT_TOKEN=$(acquire_jwt "${BASE_URL}" "${login_resp}"); then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "JWT acquired"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWT acquisition failed"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "login body=$(head -c 300 "${login_resp}" 2>/dev/null || true)"
        EXIT_CODE=1
    fi
fi

if [[ -n "${JWT_TOKEN}" ]]; then
    run_image_scale_cases "${BASE_URL}" "${JWT_TOKEN}" "${RESULT_DIR}"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen server"
if [[ -n "${HYDROGEN_PID}" ]]; then
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if stop_hydrogen "${HYDROGEN_PID}" "${SERVER_LOG}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${RESULTS_DIR}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server stopped cleanly"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server shutdown issue"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No server PID to stop"
    EXIT_CODE=1
fi

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Image scale cases: ${PASS_COUNT}/${TOTAL_COUNT} passed (${FAIL_COUNT} failed)"

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
