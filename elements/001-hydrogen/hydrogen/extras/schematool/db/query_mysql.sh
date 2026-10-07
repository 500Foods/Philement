#!/usr/bin/env bash
# SchemaTool MySQL metadata adapter — read-only SELECT on queries
#
# Emits JSON array of {query_ref,query_type,name,summary,code}.
# Large text fields are transferred as HEX and decoded with xxd.
#
# CHANGELOG
# 1.5.0 - 2026-10-07 - MySQL client only; body lives in mysql_family.sh
# 1.4.0 - 2026-08-20 - HEX decode via xxd+jq (drop python3)
# 1.3.0 - 2026-08-06 - SET SESSION TRANSACTION READ ONLY before SELECT
# 1.2.0 - 2026-08-02 - HEX+Python decode (avoid client line truncation)
# 1.1.0 - 2026-08-02 - NDJSON rows + schema-as-DB
# 1.0.0 - 2026-07-29 - Phase 3 MySQL adapter

set -euo pipefail

# shellcheck source=extras/schematool/db/mysql_family.sh # shared mysql-protocol query/catalog
source "$(dirname "${BASH_SOURCE[0]}")/mysql_family.sh"

echo "adapter: query_mysql.sh" >&2
schematool_mysql_family_dump_queries mysql "$@"
