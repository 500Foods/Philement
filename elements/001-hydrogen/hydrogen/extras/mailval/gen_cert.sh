#!/usr/bin/env bash
#
# Generate a self-signed certificate/key pair for the mailval TLS fixtures.
# Output: mailval.pem (cert) and mailval.key (key) in the script directory.
#
# CHANGELOG
# 2026-07-06 - Initial version for mailval TLS testing.

set -euo pipefail

HERE="$(cd "$(dirname "${0}")" && pwd)"
CERT="${HERE}/mailval.pem"
KEY="${HERE}/mailval.key"

if [[ -f "${CERT}" && -f "${KEY}" ]]; then
    echo "mailval cert/key already exist: ${CERT}"
    exit 0
fi

openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "${KEY}" \
    -out "${CERT}" \
    -days 3650 \
    -subj "/CN=mailval.local" \
    -addext "subjectAltName=DNS:mailval.local,IP:127.0.0.1"

echo "Generated: ${CERT} and ${KEY}"
echo "Private key must never be committed or logged."
