# Test 73: Argent MCP tools

## Overview

The [`test_73_argent_mcp.sh`](/elements/001-hydrogen/hydrogen/tests/test_73_argent_mcp.sh)
script calls every Argent MCP tool, and the validation variants each tool
returns, on eight database engines in parallel. Tool arguments and result
checks live in
[`argent_mcp_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/argent_mcp_helpers.sh).
HTTP, JWT, and case recording stay in
[`mcp_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/mcp_helpers.sh).

Test 47 remains the MCP protocol blackbox. Test 73 starts from an
initialized session and calls the Argent tools.

## Purpose

Each engine logs in, checks `GET /api/mcp/status`, initializes MCP, sends
`notifications/initialized`, then exercises the Argent tools. A missing
prerequisite records that case as a failure and the engine continues, so
the case count stays stable.

The sixteen tools are `Argent.ListOrganizations`, `Argent.UpsertOrganization`,
`Argent.ListLedgers`, `Argent.GetLedger`, `Argent.UpsertLedger`,
`Argent.UpsertLedgerTerms`, `Argent.UpsertContact`, `Argent.PostTransaction`,
`Argent.AddTags`, `Argent.RemoveTags`, `Argent.AddAttachment`,
`Argent.UpsertTaxCode`, `Argent.UpsertTaxRate`, `Argent.GetTransaction`,
`Argent.ListTransactions`, and `Argent.QueryBalances`. `tools/list` asks for
page size 500 so all sixteen names are on one page.

Success is HTTP 200 with `result.error` null and
`result.structuredContent.ok` true. A tool error is the same HTTP 200 with
`ok` false and `structuredContent.code` set to the variant name. The script
builds arguments with `jq -n`.

Covered variants include:

- Organization: missing name, bad status, fiscal month and day, unknown
  currency, create, list, rename, summary, archive, missing id
- Ledger: required fields, mask and external-ref length, idempotency key
  shape, opening balance with and without an offset, non-posting parent,
  child, currency and organization fixed, self-parent, other-organization
  parent and offset, list filters, `GetLedger` balances
- Terms and contacts: date and close-day checks, create, update, duplicate,
  role and address checks
- Transactions: empty lines, date, description, kinds 0 and 6–8 and 12,
  statuses 4 and 5, unknown and non-posting ledgers, organization mismatch,
  unbalanced rejection, balanced post, idempotent repeat, list filters by
  date, kind, status, and ledger
- Tax: code and target checks, rate date, duplicate rate, future rate that
  does not apply, net, gross, manual cent, and `needs_confirm` with no write
- Tags and attachments: create, repeat link, remove, absent remove, note
  and file revisions, `GetTransaction` meta without `file_data` or `file_text`
- Balances: required `organization_id` and `as_of`, six statuses, a bad
  rate source sent with `include_parents` (the tool checks `rate_source`
  on the rollup path), the card balance after its opening only, and parent rollup

The card ledger is not posted after its opening, so its balance stays 500.
`GetLedger` for the January term uses `as_of` 2026-03-01. A later `as_of`
would select the June term.

## Test Configuration

- **Test Name**: Argent MCP
- **Test Abbreviation**: ARG
- **Test Number**: 73
- **Version**: 1.0.2

The exercise writes `EXPECTED_TOOL_CASES` at runtime. Version 1.0.2 of the
helper records 173 tool cases. Five session cases sit beside them: login,
`api_status`, `initialize`, `initialized_202`, and `shutdown_clean`. An
engine passes when the fail count is 0 and the pass count equals
`EXPECTED_TOOL_CASES` plus 5. Startup, ready, and login failures are
separate results.

## Port Assignment

Dedicated ports in the `1573x` (WebServer) and `1574x` (MCP daemon) ranges,
the same `15<TT>x` convention as Test 47. Slot 7 is unused, matching Test
47's disabled config.

| Engine | WebServer | MCP |
|--------|-----------|-----|
| PostgreSQL | 15730 | 15740 |
| MySQL | 15731 | 15741 |
| SQLite | 15732 | 15742 |
| DB2 | 15733 | 15743 |
| MariaDB | 15734 | 15744 |
| Firebird | 15735 | 15745 |
| YugabyteDB | 15736 | 15746 |
| MSSQL | 15738 | 15748 |

The suite runs Test 73 in group 7, after the group-4 demo tests.

## Configuration Files

- `hydrogen_test_73_argent_mcp_postgres.json`
- `hydrogen_test_73_argent_mcp_mysql.json`
- `hydrogen_test_73_argent_mcp_sqlite.json` (isolated DB copy under diagnostics)
- `hydrogen_test_73_argent_mcp_db2.json`
- `hydrogen_test_73_argent_mcp_mariadb.json`
- `hydrogen_test_73_argent_mcp_firebird.json`
- `hydrogen_test_73_argent_mcp_yugabytedb.json`
- `hydrogen_test_73_argent_mcp_mssql.json`

Each config enables **API** (JWT), **Scripting** (`WorkerCount` 2,
`DefaultQueryTimeout` 60), and **MCP** (`Protocol` `Mcp.Server`,
`RequestTimeoutSeconds` 60). `Migrations` is `PAYLOAD:acuranzo+argent`.
`AutoMigration` is true. `TestMigration` is false. The ready wait is 300
seconds because startup can apply `argent_2030.lua` through `argent_2037.lua`.

Connection targets are the demo databases used by Test 40, with schema
`demo` (empty on SQLite and Firebird, `demoms` on MSSQL). A run with a
regenerated payload applies those migrations onto the shared demo schema.
SQLite copies `tests/artifacts/database/sqlite/hydrodemo.sqlite` into the
diagnostics directory, checks that `Mcp.Server` is seeded, and migrates the
copy. The shared SQLite file stays as it was.

## Prerequisites

- A payload that already contains `argent_2030.lua` through `argent_2037.lua`
  (`payload-generate.sh` or `mka`)
- Hydrogen binary (via `find_hydrogen_binary`)
- `HYDROGEN_DEMO_USER_NAME`, `HYDROGEN_DEMO_USER_PASS`, `HYDROGEN_DEMO_API_KEY`,
  `HYDROGEN_DEMO_JWT_KEY`, `PAYLOAD_KEY`
- Live demo engines for the non-SQLite variants

## Limits

`partial_write` is the code returned when an insert fails after validation.
The script has no fault injection, so that code is not asserted. Confirm
tokens, reconciliation edits, calendar sync, and the report QueryRefs are
later Argent phases.

## Related

- Plan: [ARGENT_PLAN.md](/docs/H/plans/ARGENT_PLAN.md) Phase 11, work item 11.7
- Protocol blackbox: [test_47_mcp.md](/docs/H/tests/test_47_mcp.md)
- Script migrations: Helium `argent_2030`–`argent_2037`
