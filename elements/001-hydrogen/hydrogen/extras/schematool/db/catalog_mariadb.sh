#!/usr/bin/env bash
# SchemaTool MariaDB live catalog probe — information_schema (filtered)
#
# Separate entry point from catalog_mysql.sh. The client is mariadb.
#
# CHANGELOG
# 1.0.0 - 2026-10-07 - MariaDB catalog entry point

set -euo pipefail

# shellcheck source=extras/schematool/db/mysql_family.sh # shared mysql-protocol query/catalog
source "$(dirname "${BASH_SOURCE[0]}")/mysql_family.sh"

echo "adapter: catalog_mariadb.sh" >&2
schematool_mysql_family_dump_catalog mariadb "$@"
