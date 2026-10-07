#!/usr/bin/env bash
# SchemaTool MySQL live catalog probe — information_schema (filtered)
#
# Prints JSON: { "schema": "...", "tables": [ { table, columns[], primary_key[] } ] }
#
# CHANGELOG
# 1.3.0 - 2026-10-07 - MySQL client only; body lives in mysql_family.sh
# 1.2.0 - 2026-08-20 - Assemble TSV with jq (drop python3)
# 1.1.0 - 2026-08-02 - Flat HEX export + Python assemble (avoid nested JSON_ARRAYAGG)
# 1.0.0 - 2026-08-02 - Phase 7a MySQL catalog probe

set -euo pipefail

# shellcheck source=extras/schematool/db/mysql_family.sh # shared mysql-protocol query/catalog
source "$(dirname "${BASH_SOURCE[0]}")/mysql_family.sh"

echo "adapter: catalog_mysql.sh" >&2
schematool_mysql_family_dump_catalog mysql "$@"
