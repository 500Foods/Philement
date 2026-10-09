<!-- markdownlint-disable MD024 -->

# Argent — phased implementation plan

**Date:** 2026-10-06 (PT)
**Author:** Folly (for Andrew)
**Status:** Phases 0–14 complete. Phase 12 closed 2026-10-08 on Andrew's report that migration 2044 is applied on every engine, and on Test 73 1.0.4 diagnostics `test_73_20261008_145836`: 276/276 on PostgreSQL, YugabyteDB, SQLite, MariaDB, DB2, MSSQL, MySQL, and Firebird (271 tool cases plus 5 session cases, 0 failures). The reverse half of tests 32–39 was not run. Test 31 and the full Test 98 were not re-run after the 1.0.1 edit. Phase 13 closed 2026-10-08. Andrew reported that all Unity framework unit tests pass and `mkp` passes. He did not quote a count. No migration was added in Phase 13. Phase 14 closed 2026-10-08 on Test 73 1.0.5 diagnostics `test_73_20261008_173729`: 300/300 on PostgreSQL, YugabyteDB, SQLite, MariaDB, DB2, MSSQL, MySQL, and Firebird (295 tool cases plus 5 session cases, 0 failures). The harness is 22 pass, 0 fail, 314.196s. Every engine log shows argent AVAIL = LOAD = APPLY = 2048. The down-host match stayed saved at status 3 with calendar state 4 and error `H.wait: Could not connect to server`. Work item 14.4 is checked. The full Test 31 harness was not run. Phase 15 has not started.
**Design name:** Argent
**Helium path:** `elements/002-helium/argent/`
**Database:** the Acuranzo database (same schema, same `queries` / `lookups` / `scripts`). Optional pack. Never applied alone.
**Migration series:** `argent_2xxx.lua`. On disk through `argent_2048.lua` (`RetryCalendar`). Andrew has applied through `argent_2044.lua`, including the 1.0.1 `ClearLines` and `CompleteReconciliation` scripts. `argent_2045.lua` through `argent_2048.lua` are applied. Test 73 1.0.5 diagnostics `test_73_20261008_173729` show argent AVAIL = LOAD = APPLY = 2048 on every engine. DB2 passing `test_73_20261008_145836` is that proof.

The earlier Folly copies named `/workspace/folly/argent-plan.md` and `/workspace/folly/hydrogen-bookkeeping-decisions.md` are not on this machine. Decisions from that work are in this file. Amend this file. Do not hunt for the Folly paths.

## Phases

Effort is the remaining work, or the size of the phase when it is already done. Easy, medium, or hard.

| Phase | Status | Effort |
| --- | --- | --- |
| 0 Design lock | Complete. Approved 2026-10-06 | Medium |
| 1 Organizations and ledgers | Complete. Applied 2026-10-07 | Easy |
| 2 Lookup seeds 2000–2002 | Complete. Applied 2026-10-07 | Easy |
| 3 Currencies, terms, contacts | Complete. Applied 2026-10-07 | Medium |
| 4 Transactions, lines, balance query | Complete. Continue directed 2026-10-07. No Test 31 count | Hard |
| 5 Reconciliations | Complete. Continue directed 2026-10-07. No Test 31 count | Easy |
| 6 Schedules | Complete. 2019 applied 2026-10-07. Tests passed. No Test 31 count | Easy |
| 7 Rates and parent rollup query | Complete. 2022 applied 2026-10-07. No Test 31 count | Hard |
| 8 Tax | Complete. 2024 applied 2026-10-07. No Test 31 count | Medium |
| 9 Tags and attachments | Complete. 2029 applied 2026-10-07. Tests 31 and 71 passed. No Test 31 count | Medium |
| 10 Diagrams | Complete. Test 71 3.2.0 on 2026-10-07. 1490 passed, 0 failed. No new migration | Easy |
| 11 MCP CRUD and posting | Complete. Directed close 2026-10-08. Forward load plus Test 73. Reverse half of tests 32–39 was not run | Hard |
| 12 Confirm and reconciliation tools | Complete. Test 73 1.0.4 `test_73_20261008_145836`, 276/276 on eight engines | Hard |
| 13 `H.http.request` | Complete. Andrew 2026-10-08: all Unity tests pass, `mkp` passes. No count quoted. No migration | Medium |
| 14 Schedules and calendar sync | Complete. Test 73 1.0.5 `test_73_20261008_173729`, 300/300 on eight engines | Hard |
| 15 Report queries | Not started | Hard |
| 16 Production | Not started | Easy |

Permissions, imports, Plaid, and QBO are not phases. They need an amendment.

## How a phase runs

One phase per conversation.

1. Read the previous phase Status, Accomplished, Lessons learned, and Handoff.
2. Read this phase Goal, Work items, Done means, and Exit gate. Ask before editing if a lock is ambiguous.
3. Wait for approval before editing Lua or C on a phase that has not been approved.
4. The agent prepares migrations and does not apply them. Andrew applies as the closing step of the phase and reports the result. He owns the state of any database that already applied a file we later edited in place. The agent does not run SchemaTool or SchemaHelper apply, and does not prescribe how he drops or recreates.
5. Check a work item only after the named command passed. "Intent to verify" is not verification.
6. Update Status, Accomplished, Lessons learned, Handoff, and the Working Log. Then stop. The next phase starts in a new conversation.

Payload rule: `payload-generate.sh` or `mka` refreshes `payload.tar.br.enc`. Test 01 regenerates it when a listed design's files are newer than the archive. `mkt` and `mkq` leave the archive in place. Andrew runs those commands. After a migration edit, the payload is regenerated before tests 30–38 or 71.

Lint the agent may be asked to run: Test 31 (expands SQL, no apply) and Test 98 (luacheck). A C phase uses `mkq` or `mkt`, then `mkp`. No new `static` function in `src/`. No new test script unless this plan's work item says so. No new subsystem letter.

## Next session

Phases 0–14 are complete. Phase 12 closed 2026-10-08. Andrew reported migration 2044 applied on every engine. Test 73 1.0.4 diagnostics `test_73_20261008_145836` is 276/276 on all eight engines. The reverse half of tests 32–39 was not run. Test 31 and the full Test 98 were not re-run after the 1.0.1 edit. Phase 13 closed 2026-10-08. Andrew reported that all Unity framework unit tests pass and `mkp` passes. He did not quote a count. No migration was added in Phase 13. Phase 14 closed 2026-10-08. Test 73 1.0.5 diagnostics `test_73_20261008_173729` is 300/300 on all eight engines (295 tool cases plus 5 session cases). The harness is 22 pass, 0 fail, 314.196s. Every engine log shows argent AVAIL = LOAD = APPLY = 2048. Work item 14.4 is checked. The full Test 31 harness was not run. The next conversation starts Phase 15 when Andrew asks. Phase 15 installs QueryRefs 2002–2010, one per file, and the remaining read tools, plus `UpsertRate` and `GetBocRate`. It does not change the Phase 11 posting rules. The next free file is `argent_2049.lua`. The agent does not start Phase 15 here.

---

## Locked decisions

### Pack

Argent is a second migration set on the Acuranzo database. Same schema, same connection, same `queries`, `lookups`, and `scripts`. Login rows stay in `accounts`. Lua reads them with `H.query`. `Scripting.DefaultDatabase` stays that connection, so QueryRefs 87, 152, and 153 and `Mcp.Server` are already there.

| Pack | Numbers | When |
| --- | --- | --- |
| Acuranzo | 1000–1999 | Always. Owns `queries`, `lookups`, `scripts`, `accounts`. |
| Argent | 2000–2999 | Optional. Requires Acuranzo. |
| Gaius pack | 3000–3999 | Optional, later. Requires Acuranzo. |
| Allowed | | `acuranzo`; `acuranzo+argent`; `acuranzo+gaius`; `acuranzo+gaius+argent` |
| Refused | | argent alone; gaius alone; argent+gaius with no acuranzo |

`elements/002-helium/gaius/` still bootstraps its own `queries` table in `gaius_2000.lua`. That tree is not this pack. Do not edit it and do not apply it onto Acuranzo. GLM and the Helium printing design stay on their own databases.

Four different numbers. For Argent, three of them share the range 2000–2999. The same integer may be a file number, a caller-facing QueryRef, and a lookup id. That is intentional. Someone looking up a lookup is not looking up a migration or a QueryRef. `query_id` stays the shared `MAX+1` sequence and is not in this range.

- Migration number is `cfg.MIGRATION`, the file number, 2000–2999. Bookkeeping rows store it in `query_ref` with the migration types (1000, 1001, 1002, 1003).
- `query_id` is `MAX(query_id)+1` on the shared table. No pack owns a range.
- Caller-facing QueryRef is `cfg.QUERY_REF`, also 2000–2999. One per migration, in the file that installs it. Uniqueness on `queries` is `(query_ref, query_type_a28)`, so QueryRef 2000 with type SQL can sit beside migration 2000's bookkeeping rows. The file that installs QueryRef 2000 is not `argent_2000.lua`. That file creates the organizations table. The installing file takes the next free file number and sets `cfg.QUERY_REF` to `"2000"`.
- Lookup id is its own sequence, also 2000–2999 for Argent. Acuranzo keeps 0–199 and its existing small QueryRefs. A later pack on this database uses its own migration thousand (Gaius 3000–3999) for its files, its QueryRefs, and its lookup ids. One lookup family per migration. The column name is `*_aN` with that id (`status_a2000`). A key may be 0 or negative when that value means something. The families below start at 1 because none of those states is zero or negative.

Acuranzo is not renumbered.

### One file, one change

From [`GUIDE.md`](/docs/He/GUIDE.md), section One Migration = One Logical Change:

- One file covers every engine. Macros handle the dialect.
- A file is one table, one lookup family, or one caller-facing QueryRef. It is not two of those.
- The table's own migration lists every column we already expect. A column we only learn about later is a new file: `ADD COLUMN`, reverse `DROP COLUMN`, and `${REORG}` around the drop on DB2.
- Forward, reverse, and diagram share the file number in `query_ref`. Reverse undoes only what that file did.

`argent_2000.lua` and `argent_2001.lua` name the lookup columns `status_a2000`, `ledger_type_a2001`, and `status_a2002`. Reverse of those files is `DROP TABLE`. The 2026-10-06 edit changed those files in place. While this plan is open, that is allowed. See **In-place edits until the plan is done**.

### Rates (2026-10-06)

A rate is one quote: how many units of `quote_currency` equal one unit of `base_currency`, from one source, on one date. Andrew confirmed the direction. 1.324434 CAD per 1 USD is base `usd`, quote `cad`, `rate_n` 1324434, `rate_d` 1000000.

Unique key: `(base_currency, quote_currency, source_a2012, as_of)`. One row per day. A new quote for the same key replaces the row.

`source_a2012` is lookup 2012, not free text, so each source can carry an icon in `collection` the way Acuranzo lookups do (`{"icon":...}`). Keys: 1 `boc`, 2 `bank`, 3 `paypal`, 4 `vendor`, 5 `manual`, 6 `implied`.

An implied quote is that lookup key. A mixed-currency transaction writes or replaces the one `implied` row for that pair and date. `txn_id` is nullable provenance of the latest writer. It is not part of the unique key. `ledger_id` is not a column.

### Rollups (2026-10-06)

Parent ledgers are non-posting. `is_posting` 0 means the ledger receives no lines. Its balance is a query over its descendant posting ledgers.

There is no `v_ledger_balance` and no `v_ledger_rollup`.

Transaction status is a parameter of the query. When the caller omits it, the query uses Recorded and Reconciled. Reserved and Rescinded are included only when the caller names them.

| QueryRef | Name | Phase | What it returns |
| --- | --- | --- | --- |
| 2000 | Argent balance | 4 | Posting-ledger sums in each ledger's own currency. Parameters: `organization_id`, `as_of`, optional status list. |
| 2001 | Argent rollup | 7 | Parent ledgers in the parent's currency. Parameters: the balance parameters plus `rate_source` (lookup 2012, default key 1 `boc`). Each child row includes `rate_n`, `rate_d`, and the rate's `as_of`. A missing rate leaves the converted amount null and sets a warning column. |
| 2002 | Due | 15 | Schedule rows and Reserved transactions. Default status is Reserved (lookup 2003 key 1). |
| 2003 | Reconciliation status | 15 | Last reconciliation and uncleared count. |
| 2004 | Ledger history | 15 | Lines and a running balance. Default status is Recorded and Reconciled. |
| 2005 | Tax summary | 15 | Per tax code. |
| 2006 | Income and expense | 15 | Period totals. Default status is Recorded and Reconciled. |
| 2007 | Calendar view | 15 | Calendar fields. |
| 2008 | FX premium | 15 | A named source against `boc`. |
| 2009 | Sync problems | 15 | Pending or failed calendar rows. |
| 2010 | Search | 15 | Names, descriptions, and `file_text`. |

These integers also appear as lookup ids and as file numbers. The query type, the lookup table, and the filename are what tell them apart.

`Argent.QueryBalances` calls 2000 and, when parents are requested, 2001.

The status parameter is a list of lookup 2003 keys, not a second encoding of the same words. When the caller omits it, balance, rollup, history, and income use Recorded (key 3) and Reconciled (key 4). Reserved (key 1) and Rescinded (key 5) are included only when the caller names those keys. `QueryDue` (2002) defaults to Reserved (key 1).

### Lookups that do not exist

These columns are not created. There is no leftover-lookup phase.

- Contact status. `valid_until` closes a contact.
- Currency status. A row in `currencies` is the allowlist.
- Transaction source (`manual` / `folly` / `plaid` / `qbo`). Imports are not a phase.
- `transactions.external_id`. Same reason.
- Tax-code status. `valid_until` closes a code.
- Attachment status. `att_type_a2010` distinguishes a note from a file.
- `permissions`. Full read/write for Andrew and Folly until an amendment.

Every column we already expect is on the table's create migration. Calendar columns are on `transactions`. Reconciliation and tax columns are on `lines`. `schedule_id` and `replaces_txn_id` are on `transactions`. Later phases do not go back and add those columns.

`ledgers` already has `calendar_url`, `calendar_id`, `latest_reconciliation_id`, `latest_reconciled_on`, and `opening_txn_id`. Phase 1 renamed `ledger_type_a201` to `ledger_type_a2001` and `status_a202` to `status_a2002`. It did not add or drop any other column.

### Confirm tokens

The sandbox has no HMAC helper. A confirm token is a row in `confirm_tokens`, created in Phase 12. The row stores the tool name, the canonical JSON body, `account_id`, and `expires_at` (15 minutes). The retry sends the same body plus the token. Lua compares the stored body. No hash helper is required.

### Who applies

Andrew applies every migration, and that apply is the closing step of the phase. The agent prepares the files, updates this plan, and stops. He reports the result. The agent then records it and may mark the phase complete. The agent does not apply, and does not manage databases that already hold an older copy of a file edited in place.

The usual gate is tests 32–39 on every engine that suite covers. Test 31 lints the SQL for all seven engines without applying. Test 40 stays on `PAYLOAD:acuranzo+argent`. Where the first real database is deployed does not change this gate. Phase 16 is still his declaration that Argent is the source of truth for balances.

### In-place edits until the plan is done

Argent is not officially deployed. Through the end of this plan, a migration that has not shipped may be edited in place when the phase needs a correction. Dropping the objects and recreating them is acceptable. Andrew decides how each database catches up.

When this plan is done, that window closes. A new lookup, a new QueryRef, or a change to a table, lookup, or QueryRef that already shipped is a new migration: forward, reverse, and diagram. Andrew sets that lock at the close of the plan. Until then, the phases here may still adjust the files they own.

### Product locks already in force

- Notes are text attachments. Short memos are `transactions.memo` and `lines.memo`. No `memos` table.
- CalDAV credentials are `ARGENT_CAL_USER`, `ARGENT_CAL_PASS`, and `ARGENT_CAL_HTTP`. They are never stored in the database.
- Line sign: for every currency in a transaction, the sum of `amount_cents` is 0. Debit-positive for asset and expense. Credit-positive for liability, equity, and income. Tools apply that when they compose lines.
- No SQL foreign keys. Integer keys are assigned in Lua with `COALESCE(MAX(id),0)+1`. No `${SERIAL}`.
- Money is `${INTEGER_BIG}` minor units. Currency codes are lowercase ISO 4217 in `${VARCHAR_20}`.
- Chat-hosted MCP tools do not call `H.query` or `H.http`. `Argent.*` is full read/write for Folly.
- Domain table names must not reuse Acuranzo's table list (`accounts`, `queries`, `lookups`, `scripts`, `documents`, `notes`, and the rest of the list in section Schema).

---

## Schema

Conventions for every new table: `${COMMON_CREATE}` (`valid_after`, `valid_until`, `created_id`, `created_at`, `updated_id`, `updated_at`). Soft close is `valid_until` and, where listed, a status lookup. Dates are `${DATE}`. Audit timestamps are `${TIMESTAMP_TZ}`. `${JSON}` is `collection`. No engine `DEFAULT` clause. Inserts write the zero.

### Shared Acuranzo tables

Argent inserts rows. It does not create `queries`, `lookups`, or `scripts`.

### Lookup families

| Id | Column | Keys (idx, name) | Seed phase |
| --- | --- | --- | --- |
| 2000 | `organizations.status_a2000` | 1 active, 2 archived | 2 |
| 2001 | `ledgers.ledger_type_a2001` | 1 asset, 2 liability, 3 equity, 4 income, 5 expense | 2 |
| 2002 | `ledgers.status_a2002` | 1 open, 2 closed, 3 archive | 2 |
| 2003 | `transactions.status_a2003` | 1 reserved, 2 reviewable, 3 recorded, 4 reconciled, 5 rescinded | 4 |
| 2004 | `transactions.kind_a2004` | 1 transfer, 2 purchase, 3 payment, 4 fee, 5 interest, 6 opening, 7 statement, 8 period_close, 9 invoice_record, 10 adjustment, 11 other | 4 |
| 2005 | `contacts.role_a2005` | 1 primary, 2 billing, 3 shipping, 4 other | 3 |
| 2006 | `reconciliations.status_a2006` | 1 open, 2 completed, 3 voided | 5 |
| 2007 | `schedules.status_a2007` | 1 active, 2 paused, 3 ended | 6 |
| 2008 | `schedules.horizon_mode_a2008` | 1 through_fye, 2 fixed_days, 3 manual | 6 |
| 2009 | `tag_links.entity_type_a2009`, `attachments.entity_type_a2009` | 1 organization, 2 ledger, 3 transaction, 4 line, 5 reconciliation, 6 schedule, 7 attachment, 8 contact, 9 tag | 9 |
| 2010 | `attachments.att_type_a2010` | 1 note, 2 pdf, 3 image, 4 report, 5 other | 9 |
| 2011 | `transactions.calendar_state_a2011` | 1 not_set, 2 pending, 3 set, 4 failed | 4 |
| 2012 | `rates.source_a2012` | 1 boc, 2 bank, 3 paypal, 4 vendor, 5 manual, 6 implied | 7 |

Each seed is its own file. The icon for lookup 2012 lives in `collection`, same shape as an Acuranzo lookup row. Keys start at 1 here. A later family may use 0 or a negative key when that value is the meaning.

### `organizations` (`argent_2000`, shipped)

`organization_id` `${INTEGER}` PK. `status_a2000` `${INTEGER}` NOT NULL. `name` `${TEXT}` NOT NULL. `fiscal_year_start_month` `${INTEGER}` NOT NULL (1–12, Lua-checked). `fiscal_year_start_day` `${INTEGER}` NOT NULL. `default_currency` `${VARCHAR_20}` NOT NULL. `summary` `${TEXT_BIG}`. `collection` `${JSON}`. `${COMMON_CREATE}`.

### `ledgers` (`argent_2001`, shipped)

`ledger_id` PK. `organization_id` NOT NULL. `parent_id` NULL. `status_a2002`. `ledger_type_a2001`. `is_posting` `${INTEGER_SMALL}` (1 posting, 0 parent). `name` `${TEXT}`. `currency` `${VARCHAR_20}` NOT NULL, fixed for the life of the ledger. `opening_on` `${DATE}` NOT NULL. `opening_balance_cents` `${INTEGER_BIG}` NOT NULL. `opening_txn_id` NULL. `latest_reconciliation_id` NULL. `latest_reconciled_on` NULL. `calendar_url` NULL. `calendar_id` `${VARCHAR_100}` NULL. `mask` `${VARCHAR_50}` NULL (last4 only). `external_ref` `${VARCHAR_100}` NULL. `summary` `${TEXT_BIG}`. `collection` `${JSON}`.

Counterparties are ordinary ledgers. Arrears are balances.

### `currencies` (Phase 3)

`currency_code` `${VARCHAR_20}` PK. `name` `${TEXT}`. `minor_units` `${INTEGER}` (2 for CAD and USD). `summary` `${TEXT}`. `collection` `${JSON}`. `${COMMON_CREATE}`. Seed `cad` and `usd`. No status column.

### `ledger_terms` (Phase 3)

`ledger_term_id` PK. `ledger_id` NOT NULL. `effective_on` `${DATE}` NOT NULL. Nullable: `credit_limit_cents`, `od_limit_cents`, `apr_purchase_bps`, `apr_cash_bps` (2699 means 26.99%), `annual_fee_cents`, `statement_close_day`, `payment_due_offset_days`. `summary` `${TEXT}`. `collection` `${JSON}`. `${COMMON_CREATE}`. Unique `(ledger_id, effective_on)`. Current row is the latest `effective_on` on or before the as-of date.

### `contacts` (Phase 3)

`contact_id` PK. `ledger_id` NOT NULL. `role_a2005` NOT NULL. `name` `${TEXT}` NOT NULL. `email` NULL. `phone` NULL. `summary` `${TEXT_BIG}`. `collection` `${JSON}` (address lives here). `${COMMON_CREATE}`. No status column.

### `transactions` (Phase 4, one file, full column list)

`txn_id` PK. `organization_id` NOT NULL. `status_a2003` NOT NULL. `kind_a2004` NOT NULL. `txn_on` `${DATE}` NOT NULL. `description` `${TEXT}` NOT NULL. `memo` `${TEXT}` NULL. `schedule_id` NULL. `replaces_txn_id` NULL. `calendar_state_a2011` NOT NULL. `calendar_event_id` `${VARCHAR_100}` NULL. `calendar_error` `${TEXT}` NULL. `calendar_attempts` `${INTEGER}` NOT NULL. `calendar_synced_at` `${TIMESTAMP_TZ}` NULL. `summary` `${TEXT_BIG}`. `collection` `${JSON}` (idempotency key lives here). `${COMMON_CREATE}`. Inserts write `calendar_attempts` 0 and `calendar_state_a2011` key 1 (`not_set`) until a tool sets them.

Zero-amount kinds `statement` (7) and `period_close` (8) carry attachments and participate in reconciliation and close warnings.

### `lines` (Phase 4, one file, full column list)

`line_id` PK. `txn_id` NOT NULL. `line_seq` NOT NULL. `ledger_id` NOT NULL (posting ledger). `amount_cents` `${INTEGER_BIG}` NOT NULL, in that ledger's currency. `tax_code_id` NULL. `tax_cents` `${INTEGER_BIG}` NULL. `tax_manual` `${INTEGER_SMALL}`. `cleared` `${INTEGER_SMALL}`. `reconciliation_id` NULL. `statement_txn_id` NULL. `memo` `${TEXT}` NULL. `collection` `${JSON}`. `${COMMON_CREATE}`. Unique `(txn_id, line_seq)`.

### `reconciliations` (Phase 5)

`reconciliation_id` PK. `ledger_id` NOT NULL. `statement_txn_id` NOT NULL. `reconciled_on` `${DATE}` NOT NULL. `statement_balance_cents` NOT NULL. `book_balance_cents` NOT NULL. `override_reason` `${TEXT}` NULL, required when the two balances differ and the tool completes anyway. `adjustment_txn_id` NULL. `status_a2006`. `summary` `${TEXT_BIG}`. `collection` `${JSON}`. `${COMMON_CREATE}`.

Completing a reconciliation sets `cleared` on the chosen lines, writes `ledgers.latest_reconciliation_id` and `latest_reconciled_on`, and moves a transaction to Reconciled (key 4) when every one of its lines on that ledger is cleared.

### `schedules` (Phase 6)

`schedule_id` PK. `organization_id` NOT NULL. `status_a2007`. `name` `${TEXT}`. `from_ledger_id` NOT NULL. `to_ledger_id` NOT NULL. `amount_cents` NOT NULL. `currency` NOT NULL. Both ledgers use that currency. Mixed currency is rejected by a later tool, not by this table. `tax_code_id` NULL. `rrule` `${TEXT}` NOT NULL. `anchor_on` `${DATE}` NOT NULL. `end_on` NULL. `estimate_flag` `${INTEGER_SMALL}`. `horizon_mode_a2008`. `summary` `${TEXT_BIG}`. `collection` `${JSON}`. `${COMMON_CREATE}`.

### `rates` (Phase 7)

`rate_id` PK. `base_currency` NOT NULL. `quote_currency` NOT NULL. `source_a2012` NOT NULL. `as_of` `${DATE}` NOT NULL. `rate_n` `${INTEGER_BIG}` NOT NULL. `rate_d` `${INTEGER_BIG}` NOT NULL. `txn_id` NULL. `summary` `${TEXT}`. `collection` `${JSON}` (raw BoC snippet when the source is `boc`). `${COMMON_CREATE}`. Unique `(base_currency, quote_currency, source_a2012, as_of)`.

### `tax_codes` and `tax_rates` (Phase 8)

`tax_codes`: `tax_code_id` PK. `organization_id` NOT NULL. `code` `${VARCHAR_50}` NOT NULL (`GST`, `PST-BC`, `EXEMPT`). `name` `${TEXT}` NOT NULL. `target_ledger_id` NOT NULL. `summary` `${TEXT_BIG}`. `collection` `${JSON}`. `${COMMON_CREATE}`. No status column. No unique constraint.

`tax_rates`: `tax_rate_id` PK. `tax_code_id` NOT NULL. `effective_on` `${DATE}` NOT NULL. `rate_bps` `${INTEGER}` NOT NULL (500 means 5.00%). `summary` `${TEXT}`. `collection` `${JSON}`. `${COMMON_CREATE}`. No unique constraint.

### `tags` and `tag_links` (Phase 9)

`tags`: `tag_id` PK. `organization_id` NULL means global. `name` `${TEXT}` NOT NULL. `summary` `${TEXT}`. `collection` `${JSON}`. `${COMMON_CREATE}`. No unique constraint.

`tag_links`: `tag_link_id` PK. `tag_id` NOT NULL. `entity_type_a2009` NOT NULL. `entity_id` NOT NULL. `${COMMON_CREATE}`. Unique `(tag_id, entity_type_a2009, entity_id)`.

### `attachments` (Phase 9)

Argent-native. Not a link to Acuranzo `documents`. A row may be a file, a note, or both.

`attachment_id` and `rev_id`, PK `(attachment_id, rev_id)`. `entity_type_a2009` NOT NULL. `entity_id` NOT NULL. `txn_id` NULL, set when the entity is a transaction. `att_type_a2010` NOT NULL. `mime_type` `${TEXT}` NULL (`text/plain` for a note). `file_name` `${TEXT}` NULL for a pure note. `file_data` `${TEXT_BIG}` NULL (base64; null for a pure note). `file_text` `${TEXT_BIG}` NULL (note body and extracted text). `byte_len` `${INTEGER_BIG}` NULL. `name` `${TEXT}` NOT NULL. `summary` `${TEXT_BIG}`. `collection` `${JSON}`. `${COMMON_CREATE}`. No status column. CalDAV secrets never go in `collection`.

### `confirm_tokens` (Phase 12)

`confirm_id` PK. `token` `${VARCHAR_100}` NOT NULL. `tool_name` `${VARCHAR_100}` NOT NULL. `body` `${TEXT_BIG}` NOT NULL. `account_id` NOT NULL. `expires_at` `${TIMESTAMP_TZ}` NOT NULL. `used_at` `${TIMESTAMP_TZ}` NULL. `${COMMON_CREATE}`.

---

## Business rules

These live in `scripts` (group `Argent`) and in the QueryRefs above. Calendar sync uses `H.http.request` after Phase 13. BoC fetch uses `H.http.get`.

| Rule | Behaviour |
| --- | --- |
| Balance | On every line write, group by ledger currency. Each group sums to 0. Otherwise reject. |
| Implied rate | Lookup 2012 key 6. One row per pair per day. A second transaction for that pair and day replaces the row and updates `txn_id`. |
| BoC | `Argent.GetBocRate` fetches a missing `boc` row for a pair and date. Failure of a view is a null amount plus a warning. Failure is hard only when the caller required BoC. |
| Tax | From `tax_code_id` and a net or gross flag, post companion lines to `target_ledger_id`. A manual `tax_cents` that differs from the computed tax by more than 1 cent returns a warning and requires confirm. |
| Warn-and-confirm | Edits that touch a Reconciled transaction, a date on or before `latest_reconciled_on`, or a period_close boundary return `needs_confirm`, `warning`, and `confirm_token`, and write nothing. The retry with the same body and the token applies the change and knocks status back to Recorded (key 3), clearing recon links on the affected lines. |
| Schedules | Expand the RRULE through the horizon. Default horizon is the organization fiscal year end. Create Reserved transactions. Skip a date that already has a matching Reserved or actual row. |
| Reserved to actual | An actual with `replaces_txn_id`, or a match on schedule plus date plus amount window, marks the Reserved row Rescinded. |
| CalDAV | After a successful save, set calendar state pending. The worker uses `H.http.request`. A down server does not fail the save. `calendar_attempts` increments. Failed carries `calendar_error`. |
| Opening | `Argent.UpsertLedger` posts an opening transaction (kind 6) on `opening_on` and stores its id in `opening_txn_id`. |
| Idempotency | Write tools accept `idempotency_key` and store it in `transactions.collection`. A repeat with the same key returns the existing transaction. |

---

## MCP tools

Registered in `scripts` with `mcp_access=1`. Actor is `params._hydrogen` / JWT, stored as `created_id`.

### Read

| Tool | Params | Returns | Phase |
| --- | --- | --- | --- |
| `Argent.ListOrganizations` | — | orgs | 11 |
| `Argent.ListLedgers` | `organization_id?`, `type?`, `tag?`, `include_non_posting?` | summaries | 11 |
| `Argent.GetLedger` | `ledger_id`, `as_of?` | ledger, terms, contacts, balance | 11 |
| `Argent.GetTransaction` | `txn_id` | header, lines, tags, attachment meta | 11 |
| `Argent.ListTransactions` | ledger, org, dates, status, kind | headers | 11 |
| `Argent.QueryBalances` | `as_of`, `organization_id?`, status keys, `rate_source?` | QueryRef 2000, plus 2001 when parents are requested | 11, FX in 7 |
| `Argent.QueryDue` | `from`, `to`, `organization_id?`, status keys | QueryRef 2002. Default is Reserved, lookup 2003 key 1 | 15 |
| `Argent.QueryReconciliationStatus` | `ledger_id?` | QueryRef 2003 | 15 |
| `Argent.QueryLedgerHistory` | `ledger_id`, `from`, `to`, status keys | QueryRef 2004. Running balance. Default Recorded and Reconciled | 15 |
| `Argent.QueryTaxSummary` | `organization_id`, `from`, `to` | QueryRef 2005 | 15 |
| `Argent.QueryIncomeExpense` | `organization_id`, `from`, `to`, `rate_source?`, status keys | QueryRef 2006 | 15 |
| `Argent.QueryCalendarView` | `from`, `to`, `ledger_id?` | QueryRef 2007 | 15 |
| `Argent.QueryFxPremium` | `from`, `to`, pair, `compare_source` | QueryRef 2008, compared with lookup 2012 key 1 `boc` | 15 |
| `Argent.QuerySyncProblems` | — | QueryRef 2009, pending or failed | 15 |
| `Argent.Search` | `q`, `types[]?` | QueryRef 2010 over names, descriptions, and `file_text` | 15 |
| `Argent.ListRates` | pair, source, from, to | rates | 15 |
| `Argent.GetBocRate` | pair, `as_of` | fetch and cache | 15 |

### Write

| Tool | Phase | Notes |
| --- | --- | --- |
| `Argent.UpsertOrganization` | 11 | name, FY fields, currency |
| `Argent.UpsertLedger` | 11 | creates the opening txn |
| `Argent.UpsertLedgerTerms` | 11 | |
| `Argent.UpsertContact` | 11 | |
| `Argent.PostTransaction` | 11 | balance check; tax split after Phase 8 |
| `Argent.AddTags` / `Argent.RemoveTags` | 11 | |
| `Argent.AddAttachment` | 11 | note and/or file |
| `Argent.EditTransaction` | 12 | warn path |
| `Argent.RescindTransaction` | 12 | status key 5 |
| `Argent.PostStatement` | 12 | kind 7, zero amount |
| `Argent.PostPeriodClose` | 12 | kind 8 |
| `Argent.StartReconciliation` | 12 | |
| `Argent.ClearLines` | 12 | |
| `Argent.CompleteReconciliation` | 12 | override requires a reason |
| `Argent.UpsertSchedule` | 14 | |
| `Argent.GenerateSchedule` | 14 | Reserved rows through FYE |
| `Argent.MatchReserved` | 14 | |
| `Argent.RetryCalendar` | 14 | |
| `Argent.UpsertRate` | 15 | manual source |
| `Argent.UpsertTaxCode` / `Argent.UpsertTaxRate` | 11 | tables exist after Phase 8 |

`Argent.QueryBalances` calls QueryRef 2000. QueryRef 2001 is installed in Phase 7. The tool calls it when the caller asks for parents. Status filters pass lookup 2003 keys.

---

## Phase 0 — Design lock

### Goal

Approve this file. No Lua edits and no C edits.

### Work items

- [x] 0.1 Record the pack rules, the four number spaces, and the refusal of Argent alone.
- [x] 0.2 Lock rate uniqueness as `(base_currency, quote_currency, source_a2012, as_of)`. Source is lookup 2012, with an icon. One row per day. Implied is key 6.
- [x] 0.3 Lock parent rollup as QueryRef 2001 and posting sums as QueryRef 2000. The status parameter is lookup 2003's keys. Omitted means Recorded and Reconciled.
- [x] 0.4 Assign lookup ids 2000–2012. Keys start at 1. A key may be 0 or negative when that is the meaning. Known columns sit on the create migration. Each file is one table, one lookup family, or one QueryRef.
- [x] 0.5 Assign caller-facing QueryRefs 2000–2010. The same integers may also be file numbers or lookup ids.
- [x] 0.6 Split the old single schema phase into Phases 2–10, each with its own apply gate.
- [x] 0.7 The apply gate is tests 32–39 on every engine in that suite. Andrew runs it. Deployment of the first real database is outside this plan.
- [x] 0.8 Andrew approves this file, or amends it in chat and the amendment is written here. Approved 2026-10-06.

### Done means

Work item 0.8 is checked. No migration file was added in the approval turn.

### Exit gate

Andrew's written approval of Phase 0 in chat. Given 2026-10-06.

### Status

**Complete.** Approved 2026-10-06. The phase index at the top carries status and effort.

### Accomplished

2026-10-06: replaced the prose phase list with per-phase gates and the locks above. Andrew approved this file the same day. The approval turn did not add a migration file. The column rename is Phase 1.

### Lessons learned

- The Folly decision paths are not on this machine. This file has to carry the locks.
- Phase 1 files were on disk before Phase 0 was approved. Approval did not throw them out. It sent them to Phase 1 for the column rename.
- An implied rate is a row: lookup 2012 key 6, one per pair per day. `txn_id` is not part of the unique key.
- Lookup columns that the product does not need are omitted. They are not a leftover phase.
- `mkt` does not refresh the payload archive. `payload-generate.sh` or `mka` does.
- Migration number, caller-facing QueryRef, and lookup id share 2000–2999 on purpose. The audiences are different.

### Handoff

Phase 1 renames the three lookup columns in `argent_2000.lua` and `argent_2001.lua`. It does not add a table. It does not add `argent_2002.lua`. It does not start `H.http.request`. Andrew runs the suite. The agent does not apply.

---

## Phase 1 — Organizations and ledgers

### Goal

Close the gate on the two migrations already on disk. No new tables.

### Work items

- [x] 1.1 `elements/002-helium/argent/` has the README, eight `database*.lua` copies, `argent_2000.lua`, and `argent_2001.lua`.
- [x] 1.2 Plus-list loader and per-thousand watermarks are in source. Tests 32–40 use `PAYLOAD:acuranzo+argent`. Test 71 stays Acuranzo-only until Phase 10.
- [x] 1.3 2026-10-05: payload archive contained 2000 and 2001. Test 34 applied both forward on `hydrotst.sqlite` (6/6). `TestMigration` was off, so reverse did not run.
- [x] 1.4 Amend `argent_2000.lua` so the column is `status_a2000`, and `argent_2001.lua` so the columns are `ledger_type_a2001` and `status_a2002`. No other column changes. Reverse remains `DROP TABLE`. Both files are 1.0.1 (2026-10-06).
- [x] 1.5 Andrew runs tests 32–39 and reports the result. Test 31's result is recorded here. 2026-10-07: he reported the build works and the initial application succeeded. No Test 31 count was quoted.

### Done means

Andrew has applied the amended files and reported the result. That apply is the closing step. Test 31's result is written in Status when he includes it. No new migration file.

### Exit gate

Andrew applies. Met 2026-10-07 by his report. The agent did not apply.

### Status

**Complete.** 2026-10-07: build works, initial application of the amended migrations succeeded.

### Accomplished

Design folder, organizations, ledgers, payload packing, and the plus-list loader, 2026-10-05. Coverage and regular binaries were rebuilt with that archive the same day. 2026-10-06: `organizations.status_a2000`, `ledgers.ledger_type_a2001`, and `ledgers.status_a2002` in the CREATE, the summary, and the diagram. No other column changed. Reverse is still `DROP TABLE`. 2026-10-07: Andrew applied that revision.

### Lessons learned

- Apply Argent only after Acuranzo, on the same schema. The loader reads `argent/database.lua` for an Argent file.
- SQLite's schema prefix is empty, so both packs share one file. Table names must stay distinct.
- A cycle under 10 seconds is re-run once by Test 34.
- While this plan is open, an unshipped Argent file may be edited in place. Andrew owns the databases. After the plan, a change is a new migration.

### Handoff

Do not start Phase 2 until Andrew reports that he has applied Phase 1.

Phase 2, after that report, adds only the next three files, one lookup family each. `argent_2002.lua` seeds lookup 2000 (1 active, 2 archived). `argent_2003.lua` seeds lookup 2001 (1 asset, 2 liability, 3 equity, 4 income, 5 expense). `argent_2004.lua` seeds lookup 2002 (1 open, 2 closed, 3 archive). Shape follows an Acuranzo lookup seed such as `acuranzo_1055.lua`: one family, forward, reverse, diagram, `INSERT … VALUES (...), (...)`, icon JSON in `collection` when a row has an icon. Keys start at 1. Reverse deletes only the keys that file inserted. No `queries` bootstrap, no caller-facing QueryRef, no new table. Leave `argent_2000.lua` and `argent_2001.lua` alone unless he asks for another in-place adjustment. He applies as the closing step.

---

## Phase 2 — Lookup seeds 2000, 2001, and 2002

### Goal

Seed the three families whose columns already exist on `organizations` and `ledgers`.

### Work items

- [x] 2.1 The next file seeds lookup 2000, keys 1 active and 2 archived. Icon strings may live in `collection`. `argent_2002.lua`.
- [x] 2.2 The next file seeds lookup 2001, keys 1–5 (asset, liability, equity, income, expense). `argent_2003.lua`.
- [x] 2.3 The next file seeds lookup 2002, keys 1 open, 2 closed, 3 archive. `argent_2004.lua`.
- [x] 2.4 Each file is one family, with forward, reverse, and diagram. Reverse deletes only the keys that file inserted.
- [x] 2.5 No `queries` / `lookups` / `scripts` bootstrap. No caller-facing QueryRef. No new table.
- [x] 2.6 Andrew applies and reports the result. Test 31 and Test 98 results are written here when he includes them. 2026-10-07: payload regenerated and the migrations applied. No Test 31 count was quoted.

### Done means

Andrew has applied `argent_2002.lua`, `argent_2003.lua`, and `argent_2004.lua` and reported the result. That apply is the closing step.

### Exit gate

Andrew applies. Met 2026-10-07 by his report. The agent did not apply.

### Status

**Complete.** 2026-10-07: payload regenerated and the three lookup seeds applied.

### Accomplished

2026-10-07: `argent_2002.lua` seeds lookup 2000, `argent_2003.lua` seeds lookup 2001, `argent_2004.lua` seeds lookup 2002. Each file is one family. Luacheck reported 0 warnings. SQLite expansion of 2002 shows the directory row, the value rows, and a reverse that deletes only those keys. The agent did not apply them.

### Lessons learned

- A family seed inserts two groups. Lookup_id 0, key_idx equal to the family id, is the directory row. The value keys are a second insert and start at 1.
- Reverse deletes those two groups and nothing else. DB2 rejects a delete that matches zero rows, so the key list has to match the insert.
- The diagram row uses `object_type` `lookup`. The table renderer ignores that object. It does not draw a second `lookups` table.
- Icons are in `collection` as `{"icon":"<fa ...></fa>"}`. `code` holds the stable token (`active`, `asset`, `open`). `value_txt` is the display word.

### Handoff

Do not start Phase 3 until Andrew reports that he has applied Phase 2.

Phase 3, after that report, adds `currencies`, `ledger_terms`, and `contacts`, plus the lookup 2005 seed (contact role: 1 primary, 2 billing, 3 shipping, 4 other), each in its own file. The next free file number is 2005. Suggested order: seed 2005, then `currencies` (`cad`, `usd`, no status column), then `ledger_terms`, then `contacts` with `role_a2005`. No SQL foreign keys. No `transactions`. Leave files 2000 through 2004 alone unless he asks for an in-place adjustment. He applies as the closing step.

---

## Phase 3 — Currencies, terms, and contacts

### Goal

Add the reference rows a ledger needs before any transaction exists.

### Work items

- [x] 3.1 Seed lookup 2005 (contact role) in its own file. `argent_2005.lua`.
- [x] 3.2 Create `currencies`. Seed `cad` and `usd`, `minor_units` 2. No status column. `argent_2006.lua`.
- [x] 3.3 Create `ledger_terms` with unique `(ledger_id, effective_on)`. `argent_2007.lua`.
- [x] 3.4 Create `contacts` with `role_a2005`. No status column. `argent_2008.lua`.
- [x] 3.5 Andrew applies and reports the result. Test 31 and Test 98 results are written here when he includes them. 2026-10-07: build succeeded and the migrations applied. No Test 31 count was quoted.

### Done means

Andrew has applied `argent_2005.lua` through `argent_2008.lua` and reported the result. `cad` and `usd` are the currency seed. That apply is the closing step.

### Exit gate

Andrew applies. Met 2026-10-07 by his report. The agent did not apply.

### Status

**Complete.** 2026-10-07: build succeeded and `argent_2005.lua` through `argent_2008.lua` applied.

### Accomplished

2026-10-07: `argent_2005.lua` seeds lookup 2005. `argent_2006.lua` creates `currencies` and inserts `cad` and `usd`. `argent_2007.lua` creates `ledger_terms` with unique `(ledger_id, effective_on)`. `argent_2008.lua` creates `contacts` with `role_a2005`. Luacheck reported 0 warnings. SQLite expansion shows the currency delete before `${DROP_CHECK}`, and the terms unique constraint. The agent did not apply them.

### Lessons learned

- A table that inserts its own seed cannot reverse with `${DROP_CHECK}` first. PostgreSQL's check ends the session when any row remains, and DB2 signals. Delete the seeded keys, then check, then drop.
- The delete names only `cad` and `usd`. Another currency left in the table makes the drop check refuse. That is the point of the check.
- `${INTEGER_BIG}` is still an integer on SQLite. The macro picks the engine type.

### Handoff

Do not start Phase 4 until Andrew reports that he has applied Phase 3.

Phase 4, after that report, uses the next free file numbers. `argent_2009.lua` seeds lookup 2003 (txn status, keys 1–5). `argent_2010.lua` seeds lookup 2004 (txn kind, keys 1–11). `argent_2011.lua` seeds lookup 2011 (calendar state, keys 1–4). `argent_2012.lua` creates `transactions` with the full column list in Schema. `argent_2013.lua` creates `lines` with the full column list, unique `(txn_id, line_seq)`. `argent_2014.lua` installs QueryRef 2000. That file's migration number is 2014. `cfg.QUERY_REF` is `"2000"`. One family or one table or one QueryRef per file. No edit to files 2000–2008 unless he asks. He applies as the closing step.

---

## Phase 4 — Transactions, lines, and the balance query

### Goal

Store a double-entry transaction and expose posting balances through QueryRef 2000.

### Work items

- [x] 4.1 Seed lookups 2003 (txn status), 2004 (txn kind), and 2011 (calendar state), one family per file, keys as in the lookup table. `argent_2009.lua`, `argent_2010.lua`, `argent_2011.lua`.
- [x] 4.2 Create `transactions` with the full column list in Schema, including schedule, replaces, and calendar columns. `argent_2012.lua`.
- [x] 4.3 Create `lines` with the full column list in Schema, including tax and reconciliation columns. Unique `(txn_id, line_seq)`. `argent_2013.lua`.
- [x] 4.4 Install QueryRef 2000 in its own file. Parameters: organization, as-of, optional list of lookup 2003 keys. Omitted means keys 3 and 4 (Recorded, Reconciled). Sum `amount_cents` for posting ledgers. Reserved (key 1) and Rescinded (key 5) stay out unless named. `argent_2014.lua`, `cfg.QUERY_REF` `"2000"`.
- [x] 4.5 Andrew applies and reports the result, including the QueryRef row disappearing on reverse. Test 31 and Test 98 results are written here when he includes them. 2026-10-07: he said to continue. No Test 31 count was quoted.

### Done means

QueryRef 2000 is installed and reversed on tests 32–39. The SQL rejects nothing by itself: the per-currency zero-sum is a Phase 11 Lua rule.

### Exit gate

Andrew applies. 2026-10-07 he said to continue. No Test 31 count was quoted. The agent did not apply.

### Status

**Complete.** Directed to continue on 2026-10-07. No Test 31 count was quoted.

### Accomplished

2026-10-07: `argent_2009.lua` seeds lookup 2003, `argent_2010.lua` seeds lookup 2004, `argent_2011.lua` seeds lookup 2011. `argent_2012.lua` creates `transactions`. `argent_2013.lua` creates `lines` with unique `(txn_id, line_seq)`. `argent_2014.lua` installs QueryRef 2000 as type SQL. Luacheck reported 0 warnings. SQLite expansion shows the unique constraint, `DROP_CHECK` then `DROP`, each balance parameter once, and a reverse delete of query_ref 2000 limited to type SQL. The agent did not apply them.

### Lessons learned

- Reverse of QueryRef 2000 deletes `query_ref = 2000` and `query_type_a28 = TYPE_SQL` (1). A delete by query_ref alone would also remove the `argent_2000.lua` bookkeeping rows, which use that same integer with the migration types.
- Each parameter name appears once in the balance SQL. On SQLite, MySQL, MariaDB, DB2, Firebird, and MSSQL a repeated name becomes one placeholder per occurrence and is bound once.
- `USE_DEFAULT` 1 selects lookup 2003 keys 3 and 4. `USE_DEFAULT` 0 selects the five `STATUS` slots. Any other value matches no status. Every call binds all eight names. A null slot does not match.
- `transactions` and `lines` insert no seed, so reverse is `${DROP_CHECK}` then `DROP TABLE`.
- `tax_manual`, `cleared`, and `calendar_attempts` are NOT NULL. There is no engine DEFAULT. Inserts write 0.

### Handoff

Do not start Phase 5 until Andrew reports that he has applied Phase 4.

Phase 5, after that report, uses the next free file numbers. `argent_2015.lua` seeds lookup 2006 (1 open, 2 completed, 3 voided). `argent_2016.lua` creates `reconciliations` with the column list in Schema. Do not alter `lines`. Do not implement the warn-and-confirm tools. He applies as the closing step.

---

## Phase 5 — Reconciliations

### Goal

Store a reconciliation and mark the lines it clears.

### Work items

- [x] 5.1 Seed lookup 2006 (open, completed, voided) in its own file. `argent_2015.lua`.
- [x] 5.2 Create `reconciliations` in its own file. Do not alter `lines`. `argent_2016.lua`.
- [x] 5.3 Andrew applies and reports the result. Reverse drops the table and the lookup keys only. Test 31 and Test 98 results are written here when he includes them. 2026-10-07: he said to continue. No Test 31 count was quoted.

### Done means

The recon table and lookup 2006 apply and reverse on tests 32–39.

### Exit gate

Andrew applies. 2026-10-07 he said to continue. No Test 31 count was quoted. The agent did not apply.

### Status

**Complete.** Directed to continue on 2026-10-07. No Test 31 count was quoted.

### Accomplished

2026-10-07: `argent_2015.lua` seeds lookup 2006. `argent_2016.lua` creates `reconciliations`. `lines` is unchanged. Luacheck reported 0 warnings. SQLite expansion shows keys 1 through 3, `status_a2006` NOT NULL, and `${DROP_CHECK}` then `DROP TABLE`. The agent did not apply them.

### Lessons learned

- A reconciliation row does not clear lines. Completing one writes `lines.cleared`, `ledgers.latest_reconciliation_id`, and transaction status key 4 in a later tool.
- `status_a2006` is NOT NULL. The schema line names the column and does not say NULL. Inserts write key 1 until a tool sets another. There is no engine DEFAULT.
- The table has no seed, so reverse is `${DROP_CHECK}` then `DROP TABLE`. The lookup reverse deletes the directory row and keys 1 through 3.

### Handoff

Do not start Phase 6 until Andrew reports that he has applied Phase 5.

Phase 6, after that report, uses the next free file numbers. `argent_2017.lua` seeds lookup 2007 (1 active, 2 paused, 3 ended). `argent_2018.lua` seeds lookup 2008 (1 through_fye, 2 fixed_days, 3 manual). `argent_2019.lua` creates `schedules` with the column list in Schema. Do not alter `transactions`. Do not generate Reserved transactions. Mixed-currency schedules are rejected by the later tool. He applies as the closing step.

---

## Phase 6 — Schedules

### Goal

Store a recurring template. Do not generate Reserved transactions yet.

### Work items

- [x] 6.1 Seed lookup 2007 and lookup 2008, one family per file. `argent_2017.lua`, `argent_2018.lua`.
- [x] 6.2 Create `schedules` with the full column list. Do not alter `transactions`. `argent_2019.lua`.
- [x] 6.3 Andrew applies and reports the result. Test 31 and Test 98 results are written here when he includes them. 2026-10-07: he confirmed migration 2019 applied and tests passed. No Test 31 count was quoted.

### Done means

The schedule table and lookups 2007 and 2008 apply and reverse on tests 32–39.

### Exit gate

Andrew applies. 2026-10-07 he confirmed migration 2019 applied and tests passed. No Test 31 count was quoted. The agent did not apply.

### Status

**Complete.** Andrew confirmed migration 2019 applied and tests passed on 2026-10-07. No Test 31 count was quoted.

### Accomplished

2026-10-07: `argent_2017.lua` seeds lookup 2007. `argent_2018.lua` seeds lookup 2008. `argent_2019.lua` creates `schedules`. `transactions` is unchanged. Luacheck reported 0 warnings. SQLite expansion shows both key lists, `status_a2007` and `horizon_mode_a2008` NOT NULL, and `${DROP_CHECK}` then `DROP TABLE`. Andrew later confirmed migration 2019 applied and tests passed. He did not quote a Test 31 count.

### Lessons learned

- This table stores the template. It does not insert Reserved transactions.
- `status_a2007`, `horizon_mode_a2008`, `name`, and `estimate_flag` are NOT NULL. The schema lines name them and do not say NULL. Inserts write key 1 for the two lookups and 0 for `estimate_flag`. There is no engine DEFAULT.
- Both ledgers use `currency`. Mixed currency is a later tool rule. This CREATE does not enforce it.
- The table has no seed, so reverse is `${DROP_CHECK}` then `DROP TABLE`.

### Handoff

Do not start Phase 7 until Andrew reports that he has applied Phase 6.

Phase 7, after that report, uses the next free file numbers. `argent_2020.lua` seeds lookup 2012 (1 boc, 2 bank, 3 paypal, 4 vendor, 5 manual, 6 implied), with an icon in `collection`. `argent_2021.lua` creates `rates`, unique `(base_currency, quote_currency, source_a2012, as_of)`, `txn_id` nullable. `argent_2022.lua` installs QueryRef 2001. `cfg.QUERY_REF` is `"2001"`. It does not fetch BoC. Fetch is `Argent.GetBocRate` in Phase 15. Key 6 `implied` is a normal source. One row per pair per day. He applies as the closing step.

2026-10-07: Andrew confirmed migration 2019 applied and tests passed. No Test 31 count was quoted. Phase 7 is written in the next section.

---

## Phase 7 — Rates and the parent rollup

### Goal

Store named-source quotes and convert parent ledgers with QueryRef 2001.

### Work items

- [x] 7.1 Seed lookup 2012 in its own file, including an `icon` in `collection` for each key. `argent_2020.lua`.
- [x] 7.2 Create `rates` with unique `(base_currency, quote_currency, source_a2012, as_of)` and nullable `txn_id`. `argent_2021.lua`.
- [x] 7.3 Install QueryRef 2001 in its own file. Parent currency is the parent's `ledgers.currency`. Child conversion uses the newest `rates` row for that pair and source with `as_of` on or before the requested date. Default source is lookup 2012 key 1. Missing rate: null converted amount and a warning column. `argent_2022.lua`.
- [x] 7.4 Andrew applies and reports the result. Test 31 and Test 98 results are written here when he includes them. 2026-10-07: he confirmed migration 2022 applied and said to keep going. No Test 31 count was quoted.

### Done means

QueryRef 2001 installs and reverses on tests 32–39. A fixture rate is not required for the gate. The gate is the migration apply.

### Exit gate

Andrew applies. 2026-10-07 he confirmed migration 2022 applied and said to keep going. No Test 31 count was quoted. The agent did not apply.

### Status

**Complete.** Andrew confirmed migration 2022 applied on 2026-10-07 and said to keep going. No Test 31 count was quoted.

### Accomplished

2026-10-07: `argent_2020.lua` seeds lookup 2012. `argent_2021.lua` creates `rates`. `argent_2022.lua` installs QueryRef 2001 as type SQL. Luacheck reported 0 warnings. SQLite expansion shows keys 1 through 6, `${UNIQUE}(base_currency, quote_currency, source_a2012, as_of)`, `${DROP_CHECK}` then `DROP TABLE`, each rollup parameter once, and a reverse delete of query_ref 2001 limited to type SQL. MSSQL expansion stores `WITH`. PostgreSQL, SQLite, DB2, and Firebird store `WITH RECURSIVE`. DB2 uses `SYSIBM.SYSDUMMY1`. Firebird uses `RDB$DATABASE`. No leftover `${}`. Andrew later confirmed migration 2022 applied and said to keep going. He did not quote a Test 31 count.

### Lessons learned

- `rate_n` and `rate_d` are `${INTEGER_BIG}`. The sample 1324434 / 1000000 fits a smaller integer. A longer numerator may not. `summary` is `${TEXT}`, which is `NVARCHAR(255)` on MSSQL.
- Lookup 2012 icons are defaults: 1 `fa-building-columns`, 2 `fa-landmark`, 3 `fa-credit-card`, 4 `fa-store`, 5 `fa-pen`, 6 `fa-link`. Correct them in place before apply.
- `USE_RATE_DEFAULT` 1 selects lookup 2012 key 1. 0 selects `RATE_SOURCE`. Any other value matches no source. `RATE_SOURCE` is always bound. Each of the ten parameter names appears once, in a one-row `req` CTE that uses `${DUMMY_TABLE}`.
- Same currency uses rate 1/1, a null `rate_as_of`, and `converted_cents` equal to `balance_cents`. No `rates` row is required. A different currency prefers a direct quote (base = child, quote = parent) and falls back to the inverse. Integer division truncates toward zero. A missing rate, or a zero numerator or denominator, leaves `converted_cents` null and sets `rate_warning` to 1.
- The descendant walk stops at depth 16. `SELECT DISTINCT` keeps one row per parent and posting child so a `parent_id` cycle does not multiply the sum. A parent with no posting descendant returns no row.
- `cfg.WITH_RECURSIVE` is set in this file. DB2 and MSSQL store `WITH`. The other engines store `WITH RECURSIVE`. It is not a new macro in `database.lua`. The 1.0.0 expansion stored `WITH RECURSIVE` on DB2. Test 73 `test_73_20261007_173712` rejected that with SQL0104N. Version 1.0.2 stores `WITH` for DB2. Version 1.0.3 keeps that keyword. Test 73 `test_73_20261007_182200` accepted `WITH` and returned SQL0345N on `JOIN ... ON` in the recursive member. Both arms of `descendants` now use a comma join. See Phase 11.
- Reverse of QueryRef 2001 deletes `query_ref = 2001` and `query_type_a28 = TYPE_SQL` (1). A delete by query_ref alone would also remove the `argent_2001.lua` ledgers bookkeeping rows.
- The table has no seed, so reverse is `${DROP_CHECK}` then `DROP TABLE`. This file does not fetch BoC.

### Handoff

Do not start Phase 8 until Andrew reports that he has applied Phase 7.

Phase 8, after that report, uses the next free file numbers. `argent_2023.lua` creates `tax_codes`. No status lookup. `argent_2024.lua` creates `tax_rates`. Do not alter `lines`. The tax columns are already there. He applies as the closing step.

2026-10-07: Andrew confirmed migration 2022 applied and said to keep going. No Test 31 count was quoted. Phase 8 is written in the next section.

---

## Phase 8 — Tax

### Goal

Store a tax code and its dated rate. The line columns a tax split writes already exist.

### Work items

- [x] 8.1 Create `tax_codes` in its own file. No status lookup. `argent_2023.lua`.
- [x] 8.2 Create `tax_rates` in its own file. Do not alter `lines`. `argent_2024.lua`.
- [x] 8.3 Andrew applies and reports the result. Test 31 and Test 98 results are written here when he includes them. 2026-10-07: he confirmed migration 2024 applied. No Test 31 count was quoted.

### Done means

Both tax tables apply and reverse on tests 32–39.

### Exit gate

Andrew applies. 2026-10-07 he confirmed migration 2024 applied. No Test 31 count was quoted. The agent did not apply.

### Status

**Complete.** Andrew confirmed migration 2024 applied on 2026-10-07. No Test 31 count was quoted.

### Accomplished

2026-10-07: `argent_2023.lua` creates `tax_codes`. `argent_2024.lua` creates `tax_rates`. `lines` is unchanged. Luacheck reported 0 warnings. SQLite expansion shows `code` and `name` NOT NULL, `rate_bps` integer, `effective_on` text, a primary key only, and `${DROP_CHECK}` then `DROP TABLE`. No leftover `${}`. Andrew later confirmed migration 2024 applied. He did not quote a Test 31 count.

### Lessons learned

- `code` and `name` are NOT NULL. The schema names them and does not say NULL. `GST`, `PST-BC`, and `EXEMPT` are examples. This file does not seed them.
- `rate_bps` is `${INTEGER}`. 500 means 5.00%. `effective_on` is `${DATE}`. The current rate is the latest `effective_on` on or before the as-of date.
- The schema lists no unique key, so two `tax_rates` rows may share a tax code and date. Add `(tax_code_id, effective_on)` in place if that should be refused.
- Neither table has a status column or a seed, so reverse is `${DROP_CHECK}` then `DROP TABLE`. Neither file alters `lines`. Companion tax lines are a later tool.

### Handoff

Do not start Phase 9 until Andrew reports that he has applied Phase 8.

Phase 9, after that report, uses the next free file numbers. `argent_2025.lua` seeds lookup 2009 (1 organization, 2 ledger, 3 transaction, 4 line, 5 reconciliation, 6 schedule, 7 attachment, 8 contact, 9 tag). `argent_2026.lua` seeds lookup 2010 (1 note, 2 pdf, 3 image, 4 report, 5 other). `argent_2027.lua` creates `tags`. `organization_id` NULL means global. `argent_2028.lua` creates `tag_links`, unique `(tag_id, entity_type_a2009, entity_id)`. `argent_2029.lua` creates `attachments` with primary key `(attachment_id, rev_id)`. No attachment-status lookup. He applies as the closing step.

2026-10-07: Andrew confirmed migration 2024 applied. No Test 31 count was quoted. Phase 9 is written in the next section.

---

## Phase 9 — Tags and attachments

### Goal

Store tags and revisioned notes or files on any Argent entity.

### Work items

- [x] 9.1 Seed lookup 2009 and lookup 2010, one family per file. `argent_2025.lua` and `argent_2026.lua`.
- [x] 9.2 Create `tags` and `tag_links`, one table per file. `argent_2027.lua` and `argent_2028.lua`.
- [x] 9.3 Create `attachments` with PK `(attachment_id, rev_id)`. A note has `file_data` null and `file_text` set. `argent_2029.lua`.
- [x] 9.4 Andrew applies and reports the result. Test 31 and Test 98 results are written here when he includes them. 2026-10-07: he confirmed migration 2029 applied. Tests 31 and 71 passed. No Test 31 count was quoted. He did not include a Test 98 result.

### Done means

Tag and attachment tables apply and reverse on tests 32–39.

### Exit gate

Andrew applies. 2026-10-07 he confirmed migration 2029 applied. Tests 31 and 71 passed. No Test 31 count was quoted. The agent did not apply.

### Status

**Complete.** Andrew confirmed migration 2029 applied on 2026-10-07. Tests 31 and 71 passed. No Test 31 count was quoted.

### Accomplished

2026-10-07: `argent_2025.lua` seeds lookup 2009. `argent_2026.lua` seeds lookup 2010. `argent_2027.lua` creates `tags`. `argent_2028.lua` creates `tag_links`. `argent_2029.lua` creates `attachments`. Luacheck reported 0 warnings. SQLite expansion shows both key lists and their icons, `tags` with a primary key only, `UNIQUE(tag_id, entity_type_a2009, entity_id)`, `PRIMARY KEY(attachment_id, rev_id)`, and `${DROP_CHECK}` then `DROP TABLE` on each of the three tables. The diagram marks both attachment key columns primary and not unique by themselves. No leftover `${}`. Andrew later confirmed migration 2029 applied. Tests 31 and 71 passed. He did not quote a Test 31 count. That Test 71 run used `DESIGNS` set to Acuranzo alone. Phase 10 changes the list.

### Lessons learned

- Lookup 2009 icons are defaults: 1 `fa-building`, 2 `fa-book`, 3 `fa-right-left`, 4 `fa-list`, 5 `fa-scale-balanced`, 6 `fa-calendar-days`, 7 `fa-paperclip`, 8 `fa-address-book`, 9 `fa-tag`. Lookup 2010 icons are defaults: 1 `fa-note-sticky`, 2 `fa-file-pdf`, 3 `fa-image`, 4 `fa-chart-column`, 5 `fa-ellipsis`. The PDF value text is `PDF`. Correct them in place before apply.
- `tags.organization_id` NULL means the tag is global. The schema lists no unique key, so two tags may share a name. `summary` is `${TEXT}`.
- `tag_links` has no `summary` column and no `collection` column. Unique `(tag_id, entity_type_a2009, entity_id)`. Each of those three columns is not unique by itself.
- `attachments` uses a pair primary key. The diagram marks `attachment_id` and `rev_id` primary and not unique by themselves. `mime_type` and `file_name` are `${TEXT}`. `byte_len` is `${INTEGER_BIG}`. A note has `file_data` null and `file_text` set. This CREATE does not enforce that shape.
- None of the three tables has a status column, a seed, or a SQL foreign key, so reverse is `${DROP_CHECK}` then `DROP TABLE`. There is no attachment-status lookup. This table is not a link to Acuranzo `documents`.

### Handoff

Do not start Phase 10 until Andrew reports that he has applied Phase 9.

Phase 10, after that report, may edit `tests/test_71_database_diagrams.sh` only, to add `argent` to `DESIGNS`, plus the script's `CHANGELOG` and `TEST_VERSION`. It does not add a migration. Schema phases leave Test 71 on Acuranzo alone. He runs Test 71.

2026-10-07: Andrew confirmed migration 2029 applied. Tests 31 and 71 passed. No Test 31 count was quoted. Phase 10 is written in the next section.

---

## Phase 10 — Diagrams

### Goal

Test 71 diagrams the Argent tables as well as Acuranzo.

### Work items

- [x] 10.1 Add `argent` to `DESIGNS` in `tests/test_71_database_diagrams.sh`. Schema list matches Acuranzo: `app::acuranzo:ACURANZO`.
- [x] 10.2 Bump that script's `CHANGELOG` and `TEST_VERSION`. Version 3.1.0.
- [x] 10.3 `mks` is clean. Andrew runs Test 71 and the result is recorded here. 2026-10-07: version 3.1.0 completed with 120 zero-byte Argent SVGs, so that run does not close the phase. `mks` exited 0 (Test 92, 202 files, 0 fail). The closing run is 10.6.
- [x] 10.4 Sidequest: seven engines, in this order: postgresql, mysql, sqlite, db2, mariadb, firebird, mssql. Schema slots `app:acuranzo::ACURANZO:test:testfb:testms`.
- [x] 10.5 Sidequest: a missing template uses the built-in page SVG. A zero-byte file is generated again. `get_diagram.js` 2.2.0, `get_diagram.sh` 3.2.0, Test 71 3.2.0.
- [x] 10.6 Andrew runs Test 71 3.2.0. Argent SVGs are non-empty. 2026-10-07 diagnostics `test_71_20261007_113705_483159983_2387479`: version 3.2.0, 14 design/engine passes, 2912 combinations, 1490 passed, 0 failed, elapsed 4422.095s. Argent has 30 non-empty SVGs on each of the seven engines, migrations 2000–2029. Acuranzo has 386 non-empty SVGs on each engine.

### Done means

Test 71 exits 0 with `argent` in `DESIGNS`, and the Argent SVG files are non-empty.

### Exit gate

`mks` passed on 2026-10-07 (Test 92, 202 files, 0 fail). Test 71 3.2.0 on 2026-10-07 recorded 1490 passed and 0 failed.

### Status

**Complete.** Test 71 3.2.0 recorded 1490 passed and 0 failed on 2026-10-07. Argent SVGs for migrations 2000–2029 are non-empty on seven engines. The database stays at migration 2029. No new migration.

### Accomplished

2026-10-07: `DESIGNS` lists `acuranzo` and `argent`. Version 3.1.0 completed and wrote 120 zero-byte Argent SVGs, 30 files on each of postgresql, mysql, sqlite, and db2. Those files were removed. Version 3.2.0 adds mariadb, firebird, and mssql. A one-file render of `argent_2000` produced a non-empty SVG on all seven engines. The sqlite render through `argent_2029` was 198573 bytes, 15 tables, with `attachments` highlighted. Acuranzo migration 1000 still renders. `mks` exited 0: Test 92, 202 shell files, 0 fail, 24.080s. Andrew then ran the full Test 71. Diagnostics `test_71_20261007_113705_483159983_2387479` record version 3.2.0, 14 design/engine passes, 2912 combinations, 1490 passed, 0 failed, elapsed 4422.095s. On disk, Argent has 30 non-empty SVGs on each of the seven engines, migrations 2000–2029. The sqlite file for 2029 is 198573 bytes. Acuranzo has 386 non-empty SVGs on each engine. No migration was added.

### Lessons learned

- Argent diagram JSON has tables and no `template` object. Acuranzo ships that object in `acuranzo_1000.lua`. `get_diagram.js` threw, `get_diagram.sh` exited before printing SVG, and the shell redirect left a zero-byte file.
- Test 71 treated any existing file as success, including those empty files. A later run can complete while every Argent SVG stays empty. A file now has to be non-empty to be skipped.
- Engine order is postgresql, mysql, sqlite, db2, mariadb, firebird, mssql. The empty slot is sqlite. DB2 names are uppercased in the SVG.
- `get_diagram.js` draws `object_type` `table` only. Lookup and query objects stay out of the picture. The 2029 snapshot has 15 tables.
- This test calls `get_migration.lua` on the design directory. A payload regenerate is not required.
- Acuranzo already had 120 zero-byte SVGs (42 postgresql, 42 mysql, 36 sqlite, none on db2). Version 3.2.0 retried those. After the closing run every Acuranzo SVG on the seven engines is non-empty.
- A skipped file does not print a subtest. The closing footer can show 1490 passed while the run still covers 2912 combinations. The two bookkeeping subtests sit beside the diagrams that were actually drawn.
- The version 3.1.0 footer that recorded 2 passed in about 8 seconds was that skip. An existing zero-byte file counted as success.
- Generated SVGs stay out of git. `.gitignore` uses `elements/002-helium/*/diagrams/`. Commit `0a73c20fc` removed the 1,544 tracked Acuranzo SVGs from the tree and left the files on disk.

### Handoff

Phase 10 is complete. Phase 11 starts in the next conversation.

Phase 11 adds `Argent.*` rows to `scripts` for the CRUD and posting tools listed in that phase. It does not add `H.http.request`. It does not implement confirm tokens. Tools call QueryRef 2000 (`argent_2014.lua`) and, when parents are requested, QueryRef 2001 (`argent_2022.lua`). They do not install a second copy. One script migration per tool, or one migration per tool group under 1000 lines. The next file number is `argent_2030.lua`.

---

## Phase 11 — MCP CRUD and posting

### Goal

Folly can create an organization, two posting ledgers, and one balanced transaction through MCP.

### Work items

- [x] 11.1 Scripts for `ListOrganizations`, `UpsertOrganization`, `ListLedgers`, `GetLedger`, `UpsertLedger`, `UpsertLedgerTerms`, `UpsertContact`.
- [x] 11.2 `PostTransaction` rejects a transaction whose per-currency line sum is not 0. It writes the header and lines when the sum is 0. Idempotency key is stored in `collection`.
- [x] 11.3 `UpsertLedger` writes the opening transaction (kind 6) and `opening_txn_id`.
- [x] 11.4 `AddTags`, `RemoveTags`, `AddAttachment`. `UpsertTaxCode` and `UpsertTaxRate`. `PostTransaction` posts companion tax lines when `tax_code_id` is set.
- [x] 11.5 `GetTransaction`, `ListTransactions`, and `QueryBalances` (QueryRef 2000, and 2001 when parents are requested). Status values are lookup 2003 keys.
- [x] 11.6 One script migration per tool, or one migration per tool group where the file stays under 1000 lines. `mcp_access=1`, group `Argent`. No second caller-facing QueryRef in a file that already installs one.
- [x] 11.7 Andrew runs tests 32–39 for the new script migrations, then Test 73 on all eight engines. The script calls every Argent tool and the validation variants those tools return, including one unbalanced transaction rejected. Directed close 2026-10-08: `database_load.sh` applied the forward half with TestMigration off, and Test 73 `test_73_20261008_085126` is 178/178 on all eight engines. The reverse half of tests 32–39 was not run.

### Done means

Andrew reports tests 32–39 and a green Test 73. The rejected unbalanced transaction is one of the Test 73 cases.

### Exit gate

Test 31, Test 98, payload regenerate, tests 32–39, then Test 73. Test 73 is the MCP round-trip on all eight engines. It calls every Argent tool and the validation variants those tools return. It uses `PAYLOAD:acuranzo+argent` on the demo connections (the same databases as Test 40) with AutoMigration true, so a regenerated payload applies `argent_2030.lua` through `argent_2037.lua` on startup. Test 47 stays the protocol blackbox. Andrew directed this item closed on 2026-10-08. `database_load.sh` applied the forward half with TestMigration off. Test 73 `test_73_20261008_085126` is green. The reverse half of tests 32–39 was not run.

### Status

**Complete. Directed close 2026-10-08.** Diagnostics `test_73_20261008_085126`: all eight engines passed 178 cases. The harness is 22 pass, 0 fail, 172.497s. DB2 ran `FROM DEMO.ledgers p, DEMO.ledgers c, req`. MySQL ran `json_ingest(CAST(:ORG_COLLECTION AS char(255)))`. Firebird returned `balance_cents` and the idempotency retry returned `created` false. `database_load.sh` applied the forward half with TestMigration off. The reverse half of tests 32–39 was not run.

### Accomplished

2026-10-07: eight script migrations, group `Argent`, `mcp_access=1`, `invokable=0`.

| File | Tools | Lines | Stmts |
| --- | --- | --- | --- |
| `argent_2030.lua` | `ListOrganizations`, `UpsertOrganization` | 453 | 6 |
| `argent_2031.lua` | `ListLedgers`, `GetLedger` | 513 | 6 |
| `argent_2032.lua` | `UpsertLedger` | 661 | 5 |
| `argent_2033.lua` | `UpsertLedgerTerms`, `UpsertContact` | 649 | 6 |
| `argent_2034.lua` | `PostTransaction` | 580 | 5 |
| `argent_2035.lua` | `AddTags`, `RemoveTags`, `AddAttachment` | 689 | 7 |
| `argent_2036.lua` | `UpsertTaxCode`, `UpsertTaxRate` | 536 | 6 |
| `argent_2037.lua` | `GetTransaction`, `ListTransactions`, `QueryBalances` | 672 | 7 |

The Argent README listed 38 files, 241 statements, and 38 diagrams when these scripts were written. Andrew said everything is applied. On 2026-10-08 he directed work item 11.7 closed from the forward `database_load.sh` run and Test 73. The reverse half of tests 32–39 was not run.

Test 73 is `tests/test_73_argent_mcp.sh` 1.0.2 with `tests/lib/argent_mcp_helpers.sh` 1.0.2. Diagnostics `test_73_20261007_173712` recorded SQLite 178/178, PostgreSQL 178/178, and DB2 177/178. Diagnostics `test_73_20261007_182200` recorded PostgreSQL, SQLite, MariaDB, and MSSQL at 178/178, DB2 at 177/178, Firebird at 171/178, and MySQL at 31/147. YugabyteDB returned HTTP 401 on login. Diagnostics `test_73_20261008_085126`, after a schema reset and `database_load.sh`, recorded 178/178 on all eight engines.

### Lessons learned

- A Lua nil cannot bind SQL NULL. Hydrogen omits nil table values. Each parameter name appears once per statement. Optional integers are `CASE WHEN CAST(:FLAG AS ${INTEGER}) = 0 THEN CAST(NULL AS ${INTEGER}) ELSE CAST(:VALUE AS ${INTEGER}) END`. Empty text is `NULLIF(CAST(:NAME AS ${TEXT}), '')`. A bare `NULL` is SQL0418N on DB2 and `text` on PostgreSQL. An uncast `NULLIF` against `''` is SQL0302N on DB2 when the value is non-empty. See **Portable named parameters** in [`GUIDE.md`](/docs/He/GUIDE.md).
- The actor is `tonumber(h.user_id) or tonumber(h.sub) or 0`. MCP dispatch injects `sub`. Conduit injects `user_id`.
- `UpsertLedger` posts the opening transaction on create of a posting ledger. A non-posting create stores `opening_balance_cents` and leaves `opening_txn_id` null. A zero balance is one line of 0. A nonzero balance requires `offset_ledger_id` in the same organization, the same currency, and posting. The new ledger receives the supplied amount. The offset receives the negation. An update does not post another opening.
- `PostTransaction` stores caller amounts as given. Kind 6, 7, and 8 are rejected. Status on create is 1, 2, or 3, default 3. Kind defaults to 11. After tax companions, each currency sums to 0.
- Net tax is signed half-up of `amount * rate_bps / 10000`. Gross tax is half-up of `amount * rate_bps / (10000 + rate_bps)`, and the source line is reduced by that tax. The companion uses the same sign on `target_ledger_id`. A computed tax of 0 adds no companion. The target ledger is posting and uses the source currency. No rate row on or before `txn_on` is an error. A manual `tax_cents` more than 1 cent from the computed tax returns `needs_confirm` and writes nothing. Within 1 cent, the manual amount is stored and `tax_manual` is 1.
- There is no multi-statement transaction API. Validation finishes before the first insert. A later insert failure returns `partial_write` and the `txn_id` already stored.
- Idempotency stores `{"idempotency_key":"..."}` in `transactions.collection`. A repeat for the same organization returns the existing transaction and writes nothing. The new body is not compared. Keys longer than 80, or keys with quotes, backslashes, or control characters, are rejected.
- Phase 11 does not write implied rates. Per-currency zero-sum does not define a unique pair. `UpsertRate` and `GetBocRate` stay in Phase 15.
- `QueryBalances` requires `organization_id` and `as_of`. An omitted status list uses lookup 2003 keys 3 and 4. `include_parents` also runs QueryRef 2001. The tool loads `queries.code` where `query_ref` is 2000 or 2001 and `query_type_a28` is 1, then calls `H.query_sync`. It does not install another QueryRef. The Phase 10 handoff sentence that named QueryRefs 400 and 401 was stale.
- `ListTransactions` with an omitted status list returns every status. It requires `organization_id` or `ledger_id`. `GetTransaction` tags are entity type 3, the transaction. Attachment meta omits `file_data` and `file_text`.
- Write tools build `collection` themselves. `AddAttachment` stores `{}`. CalDAV secrets are not copied from the caller.
- `AddTags` finds a tag by name and organization. A null organization is global. An existing link is returned. `RemoveTags` deletes the link and leaves the tag.
- Reverse deletes `scripts` rows by `group_name` and `script_name`, then flips this migration's bookkeeping type. It does not delete a QueryRef.
- Test 73 is the record of work item 11.7. A tool error is `structuredContent.ok` false with `structuredContent.code`. The JSON-RPC `error` field stays null, and HTTP status stays 200. Success is `structuredContent.ok` true. The writing turn did not run the test: the payload does not yet contain these scripts, and AutoMigration on the demo connections would apply them. SQLite copies `hydrodemo.sqlite` first. `partial_write` needs a failed insert after validation, so Test 73 does not force it.
- The ledger-write group did not fit in 1000 lines, so `UpsertLedger` is `argent_2032.lua` and terms plus contacts are `argent_2033.lua`. Reads landed in `argent_2037.lua`.
- `organizations` and `ledgers` on the already-applied schemas kept `status_a200`, `ledger_type_a201`, and `status_a202`. The files and the tools use `status_a2000`, `ledger_type_a2001`, and `status_a2002`. The watermark skips migration 2000 and 2001, so a later apply does not rename those columns. A fresh SQLite copy creates the current names and Test 73 passes there.
- `validate_config_file` already closes its subtest. A second `print_result` in the same subtest prints `extra PASS/FAIL without TEST`.
- In jq, `|` binds tighter than `and`. Each comparison in a compound filter needs its own parentheses.
- `QueryBalances` checks `rate_source` when `include_parents` is set. The posting cases need the 500 bps rate to be the latest row on or before `txn_on`.
- QueryRef 2001 stores `WITH` on DB2 and SQL Server. The other engines store `WITH RECURSIVE`. `WITH RECURSIVE` on DB2 is SQL0104N (`FAIL_bal_parents` on `test_73_20261007_173712`). The keyword is `cfg.WITH_RECURSIVE` in `argent_2022.lua`. It is not a macro. See **Recursive common table expressions** in [`GUIDE.md`](/docs/He/GUIDE.md).
- DB2 also rejects `JOIN ... ON` inside the recursive fullselect. That is SQL0345N (`SQLSTATE` 42836), the `bal_parents` failure on `test_73_20261007_182200`, after `WITH` was loaded. `argent_2022.lua` 1.0.3 uses a comma join in both arms of `descendants`. The `posting` CTE still uses `JOIN ... ON`.
- MySQL rejects `CAST(x AS int)` and `CAST(x AS varchar(n))`. The file-local targets are `cfg.CAST_INTEGER = "signed"` and `cfg.CAST_TEXT = "char(255)"`. MariaDB accepts `int` and `varchar(255)`, so it keeps `cfg.INTEGER` and `cfg.TEXT`. The organization insert on `test_73_20261007_182200` failed at `CAST(:ORG_SUMMARY AS varchar(255))`.
- Firebird 4 types `SUM` of `BIGINT` as INT128. Hydrogen's reader emits JSON null, so `balance_cents` disappears. `cfg.BALANCE_SUM` casts that sum to `BIGINT` on Firebird in QueryRef 2000 and in the posting CTE of QueryRef 2001. The other engines keep the uncast `COALESCE`.
- Firebird describes a `json_ingest` parameter as a blob. Hydrogen does not bind blob inputs, so `transactions.collection` was stored null and the idempotency check inserted a second row. The tool scripts pass `${JIS}CAST(:NAME AS ${CAST_TEXT})${JIE}` on every engine. These fields are not new macros in `database.lua`.

### Handoff

Phase 12 adds `confirm_tokens` and the recon and edit tools. It leaves these scripts in place and calls them. A reconciled edit without a token must not write. Phase 12 waits until Test 73 is green on all eight engines.

Andrew directed work item 11.7 closed on 2026-10-08. `database_load.sh` applied the test schemas and the demo schemas with TestMigration off. Test 73 `test_73_20261008_085126` passed 178 cases on all eight engines. The reverse half of tests 32–39 was not run. Phase 12 is the next section. Andrew later applied those files through `argent_2044.lua`.

---

## Phase 12 — Confirm and reconciliation

### Goal

Dangerous edits do not write until the same body comes back with a confirm token. A reconciliation can be completed, including an override that carries a reason.

### Work items

- [x] 12.1 Create `confirm_tokens` as specified. No HMAC.
- [x] 12.2 `EditTransaction` and `RescindTransaction` implement the warn path and the knock-back to Recorded.
- [x] 12.3 `PostStatement`, `PostPeriodClose`, `StartReconciliation`, `ClearLines`, `CompleteReconciliation`. Override without `override_reason` is rejected when the balances differ.
- [x] 12.4 Andrew's fixture: edit a reconciled txn, observe `needs_confirm` and no mutation, retry with the token, observe Recorded. Test 73 1.0.4 `test_73_20261008_145836`, 276/276 on all eight engines.

### Done means

The fixture in 12.4 is recorded. An override without a reason is rejected.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply, Andrew's fixture report.

### Status

**Complete.** Andrew applied through `argent_2044.lua`, including the 1.0.1 scripts. Test 73 1.0.4 diagnostics `test_73_20261008_145836` passed 276 cases on PostgreSQL, YugabyteDB, SQLite, MariaDB, DB2, MSSQL, MySQL, and Firebird. Each result file has `EXPECTED_TOOL_CASES=271`, `READY=1`, `LOGIN_OK=1`, and 0 `CASE_FAIL`. The YugabyteDB log for that run has no auth-query timeout. Test 31 and the full Test 98 were not re-run after the 1.0.1 edit. Phase 13 has not started.

### Accomplished

2026-10-08: `confirm_tokens` and seven tools. Group `Argent`, `mcp_access=1`, `invokable=0`. No caller-facing QueryRef.

| File | Tools | Lines | Stmts |
| --- | --- | --- | --- |
| `argent_2038.lua` | `confirm_tokens` table | 216 | 6 |
| `argent_2039.lua` | `EditTransaction`, `RescindTransaction` | 994 | 6 |
| `argent_2040.lua` | `PostStatement` | 617 | 5 |
| `argent_2041.lua` | `PostPeriodClose` | 602 | 5 |
| `argent_2042.lua` | `StartReconciliation` | 592 | 5 |
| `argent_2043.lua` | `ClearLines` | 586 | 5 |
| `argent_2044.lua` | `CompleteReconciliation` | 402 | 5 |

The Argent README lists 45 files, 278 statements, and 45 diagrams. The next free file number is `argent_2045.lua`.

A dangerous edit writes nothing on the first call. The response is `fail("needs_confirm", ...)`, so `ok` is false and `code` is `needs_confirm`, and the extra fields are `needs_confirm` true, `warning`, and `confirm_token`. The retry sends the same body plus the token. Lua compares the stored body. The stored body omits `confirm_token` and `_hydrogen`. A confirmed edit sets status 3 (Recorded) and clears recon links on that transaction's lines. A confirmed rescind sets status 5 and clears those links. `CompleteReconciliation` has no token. When `statement_balance_cents` and the recomputed book balance differ and `override_reason` is empty, it returns `override_reason_required` and writes nothing.

### Lessons learned

- There is no shared Lua module inside a migration. The confirm helper is copied into each script that issues a token. `argent_2039.lua` holds both edit tools and is 994 lines. A second copy of the helper puts two tools over the 1000-line ceiling, so `argent_2040.lua` through `argent_2043.lua` are one tool each. `argent_2044.lua` does not copy the helper.
- Expiry SQL and the Firebird body cast are `cfg` strings set in the migration. They are not new macros in `database.lua`. SQLite uses `datetime('now', '+15 minutes')`. MySQL and MariaDB use `DATE_ADD`. DB2 uses `CURRENT TIMESTAMP + 15 MINUTES`. SQL Server uses `DATEADD` on `SYSDATETIMEOFFSET()`. Firebird uses `DATEADD`. PostgreSQL, including Yugabyte, uses `CURRENT_TIMESTAMP + INTERVAL '15 minutes'`. The compare is `expires_at > ${NOW_CMP}`.
- Firebird insert of `body` is `CAST(:BODY AS VARCHAR(8191))`, so the parameter is `VARCHAR`. A canonical body longer than 4000 characters is `body_too_long`. The other engines bind `:BODY` into `${TEXT_BIG}`.
- The token is `c{confirm_id}-{os.time()}-{os.clock fraction}`. Comparison is the canonical body. The sandbox has no HMAC helper.
- Canonical JSON is hand-rolled. Keys are sorted by `tostring`. A table whose keys are a dense positive-integer range encodes as an array. An empty table encodes as `[]`. Whole numbers with absolute value under 1e15 use `%d`. The escape pattern stays a Lua short string inside the `[====[ ]====]` migration so the backslashes are literal.
- Creates (`PostStatement`, `PostPeriodClose`, `StartReconciliation`) consume the token before the insert, so a replay cannot insert twice. Edits and `ClearLines` consume after the write. A token on a safe call is still checked, and consumed after success.
- `ClearLines` does not knock a transaction back to Recorded. Status 4 is `reconciled_line`. Status 5 is `rescinded_line`. It sets `reconciliation_id` and `statement_txn_id` and leaves `cleared` at 0. `CompleteReconciliation` sets `cleared` to 1, promotes a transaction to status 4 when every line on that ledger is cleared, then stores status 2 and the ledger's `latest_reconciliation_id` and `latest_reconciled_on`.
- Date checks are Lua compares of the first 10 characters when they match `YYYY-MM-DD`. Period-close coverage is a select of non-rescinded kind 8 rows, then a Lua compare. These tools do not add a recursive CTE.
- Book balance uses `cfg.BALANCE_SUM`. Firebird casts `SUM` to `BIGINT`. The other engines use `COALESCE(SUM(ln.amount_cents), 0)`. The sum is not returned to Lua as INT128.
- A Lua long string drops one leading newline. Concatenating an `end` block with a `[[` block that opened on a newline produced `endif` in `argent_2040.lua` through `argent_2044.lua`. Those six joins were split before luacheck and `luac -p`.
- Phase 11 posting rules stay in `argent_2034.lua`. Kinds 7 and 8 are posted by the new tools, with one line of amount 0 and status 3. Idempotency on those two tools matches `PostTransaction`: the stored key returns the existing row and does not compare the new body.
- The first `PostPeriodClose` does not warn because the new row will be kind 8. `period close` is the edit or rescind warning for an existing kind-8 row. A later close whose date falls on or before that row warns `period close boundary`. Test 73 follows that order.
- A space inside an unquoted `[[ =~ ]]` class is split by the shell. The warning check in `argent_mcp_helpers.sh` stores `^[a-z ]+$` in `warn_re`.
- DB2 returns `SQL0100W` when an `UPDATE` matches zero rows, and Hydrogen treats that as `update_failed`. `ClearLines` selects `cleared = 0` lines for the reconciliation and skips the unlink when the select is empty. `CompleteReconciliation` selects lines for the reconciliation and skips `cleared = 1` when the select is empty. Do not ignore `SQL0100W` in the DB2 driver. DB2 passing `clear_empty` on `test_73_20261008_145836` is the proof the stored script is that 1.0.1 text.

### Handoff

Phase 12 is complete. The stored `ClearLines` and `CompleteReconciliation` bodies are the 1.0.1 text. The next free Argent file is `argent_2045.lua`.

Phase 13 is C, in `scripting_api_http.c`, `http_client.c`, and `http_pool.c`. It does not add a migration. It does not start the calendar worker. `H.http.get` and `H.http.post` stay as wrappers. The allowlist is `GET`, `POST`, `PUT`, `DELETE`, `PROPFIND`, `REPORT`, `MKCALENDAR`, and `PROPPATCH`. Phase 13 has not started.

---

## Phase 13 — `H.http.request`

### Goal

Lua can send the CalDAV verbs. Non-2xx responses come back as data.

### Work items

- [x] 13.1 `H.http.request(method, url, body, headers, opts)` and `H.http.request_sync`. Allowlist: `GET`, `POST`, `PUT`, `DELETE`, `PROPFIND`, `REPORT`, `MKCALENDAR`, `PROPPATCH`. Any other token, including `CONNECT` and `TRACE`, is an error.
- [x] 13.2 `get` and `post` call the same path. A 207 or 412 returns `{ status, headers, body, elapsed_ms }`.
- [x] 13.3 Unity tests for an allowed verb, a rejected verb, and a non-2xx body. No new `static` function. Prototypes in the existing headers.
- [x] 13.4 Update [`LUA_GUIDE.md`](/docs/H/LUA_GUIDE.md) and [`lua_api.md`](/docs/H/core/subsystems/scripting/lua_api.md).
- [x] 13.5 Andrew runs `mkq` (or `mkt` if a new `src/` file was added) and `mkp`. Named `mku` results are written here. Andrew reported 2026-10-08 that all Unity framework unit tests pass and `mkp` passes. He did not quote a count. The implementation run recorded `http_client_test_request` 7 tests, `scripting_api_http_test_request` 8 tests, and `http_pool_test_worker_process_one` 5 tests, each with 0 failures.

### Done means

The Unity tests pass and `mkp` is clean. No migration was added.

### Exit gate

`mkq` or `mkt`, then `mkp`, then the named Unity tests. Andrew runs them.

### Status

**Complete.** Andrew reported 2026-10-08 that all Unity framework unit tests pass and `mkp` passes. He did not quote a Unity total or an `mkp` file count. No new `src/` file was added. `oidc_rp_http.c` is the existing libcurl file. No migration was added. Phase 14 has not started.

### Accomplished

2026-10-08: `H.http.request` and `H.http.request_sync`. `H.http.get` and `H.http.post` call `scripting_http_request`. The allowlist is an exact match on `GET`, `POST`, `PUT`, `DELETE`, `PROPFIND`, `REPORT`, `MKCALENDAR`, and `PROPPATCH`.

`scripting_http_request` lives in `http_client.c`. GET calls `oidc_rp_http_get_with_headers_slist`. POST calls `oidc_rp_http_post_with_headers_slist`. The other six verbs call `oidc_rp_http_request_with_headers_slist` in `oidc_rp_http.c`, which sets `CURLOPT_CUSTOMREQUEST`. A NULL body on that helper sends no body. POST with a NULL body still sends an empty body, which is the helper that already shipped. A rejected method returns `error_message` `"HTTP method is not allowed"` and does not consume the scripting test seam. Lua stores `"H.http.request: method is not allowed"` on the handle before any wait. The pool and the inline wait still store `"H.wait: unknown HTTP method on handle"` when a raw handle carries a disallowed method.

A status such as 207 or 412 is the result table. `perform_and_finalize` sets `error_message` on a transport failure. An HTTP status leaves it unset.

New Unity files: `tests/unity/src/scripting/http_client_test_request.c` and `tests/unity/src/scripting/scripting_api_http_test_request.c`. `http_pool_test_worker_process_one.c` rejects `CONNECT`. `DELETE` is allowlisted. [`LUA_GUIDE.md`](/docs/H/LUA_GUIDE.md) and [`lua_api.md`](/docs/H/core/subsystems/scripting/lua_api.md) describe the contract. Markdownlint on those two files exited 0.

This session ran `cmake -S . -B ../build --preset default` from `cmake/`. That reconfigured `build/` and did not wipe it. `mkq` skips configure, so a build directory that has not been reconfigured will not see the new `*_test*.c` files. The binaries then ran:

| Binary | Tests | Failures |
| --- | --- | --- |
| `http_client_test_request` | 7 | 0 |
| `scripting_api_http_test_request` | 8 | 0 |
| `http_pool_test_worker_process_one` | 5 | 0 |
| `http_client_test_get` | 4 | 0 |
| `http_client_test_post` | 4 | 0 |
| `scripting_api_http_test_async` | 5 | 0 |
| `scripting_api_http_test` | 33 | 0 |

### Lessons learned

- GET and POST stay on the OIDC helpers that already shipped, so those curl options stay byte-stable for OIDC and MCP. The six other verbs are `CURLOPT_CUSTOMREQUEST` in `oidc_rp_http_request_with_headers_slist`. The plan named three files. Libcurl stays in `oidc_rp_http.c`, so that file is the fourth edit and the only curl contact.
- A non-2xx status was already data when `error_message` is unset. The new path does not turn 207 or 412 into an error.
- `http_pool_test_worker_process_one` used `DELETE` as the unknown method. `DELETE` is allowlisted, so the rejected case is `CONNECT`. A `DELETE` handle with no fixture would reach the network.
- The match is `strcmp` of the eight uppercase tokens. `propfind` and `get` are errors. Lua rejects them on the handle and does not consume a fixture. `scripting_http_request` does the same for a C caller.
- Content-Type is `strdup`'d while the Lua string is still on the stack. A `lua_tostring` pointer does not survive the pop plus a later Lua call.
- New Unity sources are invisible to ninja until CMake configure. `cmake/CMakeLists-unity.cmake` globs `*_test*.c` at configure time. `mkq` skips configure. `mkt` reconfigures and deletes `build/*`. This session reconfigured the existing `build/` directory. No new `src/` file was added, so the plan's "`mkt` if a new `src/` file" clause does not force `mkt`.
- No migration was added. The next free Argent file is still `argent_2045.lua`.
- The close report names the whole Unity suite and `mkp`, and does not quote a count. The named binaries from the implementation run stay in Accomplished.

### Handoff

Phase 13 is complete. The stored allowlist and the `H.http.request` path stay as written.

Phase 14 adds the schedule and calendar tools. Lookup 2011 and the calendar columns already exist. Do not add columns. The worker calls `H.http.request`. Credentials are `ARGENT_CAL_USER`, `ARGENT_CAL_PASS`, and `ARGENT_CAL_HTTP`. A failed HTTP call does not roll back the transaction. The next free file is `argent_2045.lua`. Phase 14 has not started.

---

## Phase 14 — Schedule generation and calendar sync

### Goal

Generate a month of Reserved rent, match an actual, and save a transaction while CalDAV is down.

### Work items

- [x] 14.1 `UpsertSchedule`, `GenerateSchedule` through fiscal year end, `MatchReserved`. Do not add columns.
- [x] 14.2 After a successful save, set calendar state pending (lookup 2011 key 2). The worker uses `H.http.request` with `ARGENT_CAL_*`. Failure increments `calendar_attempts`, stores `calendar_error`, and leaves the transaction saved.
- [x] 14.3 `RetryCalendar`.
- [x] 14.4 Andrew's fixture: a month of Reserved rows, one matched actual, one save with the calendar host unreachable. The Reserved status written is lookup 2003 key 1.

### Done means

The fixture in 14.4 is recorded. The unreachable host did not fail the save.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply, Andrew's fixture.

### Status

**Complete.** Test 73 1.0.5 diagnostics `test_73_20261008_173729` is 300/300 on all eight engines. The harness is 22 pass, 0 fail, 314.196s. Work item 14.4 is checked. The full Test 31 harness was not run.

### Accomplished

`argent_2045.lua` 1.0.0 stores a schedule and, when `calendar_url` is sent on a real save, writes it on `from_ledger_id`. A repeat `idempotency_key` with no `schedule_id` returns the existing row and writes nothing, including the URL. `argent_2046.lua` expands `FREQ=WEEKLY` and the other accepted RRULE subset. Horizon mode 1 stops at fiscal year end. `through` and `end_on` can stop earlier. Reserved rows are lookup 2003 key 1 and kind 11. A date that already has a row for that schedule is skipped. `argent_2047.lua` checks the amount window before any write, inserts or links the Recorded actual, and rescinds the Reserved row to key 5. `argent_2048.lua` retries calendar state 2 or 4, one `txn_id` or up to 20.

Calendar sync runs only when a line ledger already has a usable `calendar_url`. An absolute `http(s)` URL is used as-is. A relative path joins `ARGENT_CAL_HTTP`. No URL leaves the calendar state unchanged, so existing `PostTransaction` calls and the pre-URL generate do not open a socket. The worker sets state 2, then `PUT`s a small `VEVENT` through `H.http.request_sync` with a 3 second timeout. Basic auth is sent only when `ARGENT_CAL_USER` is non-empty. A transport error, a `pcall` failure, or a non-2xx status sets state 4, increments `calendar_attempts`, stores `calendar_error`, and the tool still returns `ok`. A 2xx sets state 3, `calendar_event_id` `argent-txn-{id}`, and clears the error. Each update is selected first. No column was added. `argent_2032.lua`, `argent_2034.lua`, and `argent_2037.lua` were not edited.

Luacheck on the four files reported 0 warnings. Test 98 1.1.1 at 2026-10-08 17:21:57 found no issues in 531 files (2 pass, 0 fail, 4.485s). That run linted these four files and used the cache for the other 527. `luac -p` accepted the migration files and the extracted script bodies. `tests/lib/get_migration.sh` expanded each file for postgresql, sqlite, mysql, db2, mariadb, firebird, and mssql (28 files). None of that SQL still contains `${...}`. The full Test 31 harness was not run, so there is no Test 31 count. The script bodies are brotli and base64 when the engine sets `COMPRESS_START`.

`tests/test_73_argent_mcp.sh` and `tests/lib/argent_mcp_helpers.sh` are 1.0.5. The exercise adds the four tool names and 24 cases. `EXPECTED_TOOL_CASES` stays the live count. Test 92 4.1.1 at 2026-10-08 17:21:57 found no issues in 207 shell files (3 pass, 0 fail, 27.120s). Test 73 was not run. The Argent README lists 49 files, 298 statements, and 49 diagrams.

### Lessons learned

- The calendar gate is `ledgers.calendar_url`, not the process environment alone. Generate checks that URL before it inserts, so a month of Reserved rows can be written with no HTTP call. The dead host is attached afterward, by `schedule_id`, and the next match is the one that must survive the refusal.
- Pending is one update, and the attempt counter moves only in the failure update. A down host still returns `ok`. The save is already committed as its own `H.query_sync` calls. There is no multi-statement transaction to roll back.
- A repeat idempotency key returns before validation and before `calendar_url`. The Test 73 repeat sends `http://127.0.0.1:9/cal/` on purpose. The following generate must still show calendar state 1. That is the proof the URL was not stored.
- There is no shared Lua module inside a migration. The helper is copied into 2046, 2047, and 2048. 2045 does not call HTTP. The wrapper's nested long strings reject `]=]` and `]==]` in the inner script. 2046 is 943 lines.
- Select before every calendar update. DB2 `SQL0100W` still fails a zero-row update. The driver stays unchanged.
- "A month" in the fixture is `FREQ=WEEKLY` from `2026-10-01` through `2026-10-22`: four dates, `2026-10-01`, `2026-10-08`, `2026-10-15`, and `2026-10-22`. Mode 1 still caps at fiscal year end. The Test 73 organization starts its year on April 1, so that anchor's year ends `2027-03-31`. `through` stops the expansion earlier.
- Each file is five statements: forward insert, the applied update, reverse insert, the reverse update, and the diagram insert. The README total is 49 files, 298 statements, and 49 diagrams.
- Grep of the expanded SQL does not show the tool's `INSERT`. `COMPRESS_START` stores the script as brotli plus base64. The unsubstituted-`${}` check is the one that matters. This session did not run the Test 31 harness.
- The refused connect is `H.wait: Could not connect to server`. On every engine the saved actual stayed status 3, calendar state 4, one attempt. `RetryCalendar` made that two attempts and left the status at 3.

### Handoff

Phase 14 is complete. `argent_2045.lua` through `argent_2048.lua` are applied. Test 73 1.0.5 diagnostics `test_73_20261008_173729` recorded the fixture.

Phase 15 installs QueryRefs 2002–2010, one per file, and the remaining read tools, plus `UpsertRate` and `GetBocRate`. It does not change the Phase 11 posting rules. Phase 15 has not started. The next free file is `argent_2049.lua`. The next conversation starts Phase 15 when Andrew asks.

---

## Phase 15 — Report queries

### Goal

Each report in the read-tool table returns stable JSON. The migrations have been through tests 32–39.

### Work items

- [ ] 15.1 Install QueryRefs 2002 through 2010, one per file. Defaults match the tool table. `QueryDue` (2002) defaults to lookup 2003 key 1 (Reserved). The others that take a status list default to keys 3 and 4 (Recorded, Reconciled).
- [ ] 15.2 MCP wrappers: `QueryDue`, `QueryReconciliationStatus`, `QueryLedgerHistory`, `QueryTaxSummary`, `QueryIncomeExpense`, `QueryCalendarView`, `QueryFxPremium`, `QuerySyncProblems`, `Search`, `ListRates`, `GetBocRate`.
- [ ] 15.3 `UpsertRate` for lookup 2012 key 5 (`manual`). `GetBocRate` inserts key 1 (`boc`) through `H.http.get`.
- [ ] 15.4 Andrew runs tests 32–39 and a fixture. The JSON paths are recorded here.

### Done means

Each QueryRef 2002–2010 has a recorded fixture result. Reserved stays out of QueryRef 2000 unless the status list names lookup 2003 key 1. `QueryDue` includes Reserved by default.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply, Andrew's fixture.

### Status

**Not started.**

### Handoff

Phase 16 is the production database Andrew already uses for Acuranzo. The agent does not apply. There is no bulk import.

---

## Phase 16 — Production

### Goal

Argent is applied on the real Acuranzo database and Folly records day-to-day entries there.

### Work items

- [ ] 16.1 Andrew sets that connection's `Migrations` to `PAYLOAD:acuranzo+argent` and applies. Same schema, same connection. No second database.
- [ ] 16.2 Organizations and ledgers are entered through MCP from his tracker. Opening balances are opening transactions or statement snapshots. No import pipeline.
- [ ] 16.3 A dual-write period: Folly updates Argent and the tracker until he says otherwise.
- [ ] 16.4 He declares Argent the source of truth for balances.

### Done means

He has said Argent is the source of truth. About 25 ledgers are visible through `Argent.ListLedgers`.

### Exit gate

Andrew's report. No agent apply.

### Status

**Not started.**

### Handoff

None. An amendment opens permissions, imports, or QBO. Those are not implied by this phase closing.

---

## Working log

### 2026-10-05 — Folder and first two tables

`argent_2000` and `argent_2001` landed. Plus-list loader compiled. Test 34 applied both forward on SQLite. Reverse did not run. Phase 0 was not approved.

### 2026-10-06 — Restructure

Andrew locked the 2000 range for migration numbers, caller-facing QueryRefs, and lookup ids, and said the same integer may be all three. Rate source is lookup 2012 with an icon, one row per day, implied as key 6. Known columns go on the create migration. One file is one table, one lookup family, or one QueryRef. The apply gate is tests 32–39. Status filters use lookup 2003's keys.

### 2026-10-06 — Phase 0 approved, Phase 1 columns renamed

Andrew approved Phase 0 and said to proceed one phase at a time. Phase 0 is complete. `argent_2000.lua` and `argent_2001.lua` are 1.0.1: `status_a2000`, `ledger_type_a2001`, `status_a2002`. The suite was not run. Phase 1 stays open. The top of this file lists every phase with status and effort.

He applies as the closing step of a phase and owns databases that already hold an in-place edit. Through the end of this plan, unshipped Argent files may be edited in place, and dropping and recreating is acceptable. When the plan is done, a new lookup, QueryRef, or change to one that shipped is a new migration. He sets that lock at the close.

### 2026-10-07 — Phase 1 applied, Phase 2 seeds written

Andrew reported the build works and the initial application succeeded. Phase 1 is complete. Phase 2 adds `argent_2002.lua` (lookup 2000), `argent_2003.lua` (lookup 2001), and `argent_2004.lua` (lookup 2002). Not applied. Phase 3 waits for his report.

### 2026-10-07 — Phase 2 applied, Phase 3 tables written

Andrew regenerated the payload and applied the Phase 2 seeds. Phase 2 is complete. Phase 3 adds `argent_2005.lua` through `argent_2008.lua`: lookup 2005, `currencies` (`cad`, `usd`), `ledger_terms`, and `contacts`. Not applied. Phase 4 waits for his report.

### 2026-10-07 — Phase 3 applied, Phase 4 written

Andrew reported the build succeeded and the Phase 3 migrations applied. Phase 3 is complete. Phase 4 adds `argent_2009.lua` through `argent_2014.lua`: lookups 2003, 2004, and 2011, `transactions`, `lines`, and QueryRef 2000. Not applied. Phase 5 waits for his report.

### 2026-10-07 — Phase 4 directed forward, Phase 5 written

Andrew said to continue. He did not quote a Test 31 count. Phase 5 adds `argent_2015.lua` (lookup 2006) and `argent_2016.lua` (`reconciliations`). Not applied. `lines` is unchanged. Phase 6 waits for his report.

### 2026-10-07 — Phase 5 directed forward, Phase 6 written

Andrew said to continue. He did not quote a Test 31 count. Phase 6 adds `argent_2017.lua` (lookup 2007), `argent_2018.lua` (lookup 2008), and `argent_2019.lua` (`schedules`). Not applied. `transactions` is unchanged. Phase 7 waits for his report.

### 2026-10-07 — Phase 6 applied, Phase 7 written

Andrew confirmed migration 2019 applied and tests passed. He did not quote a Test 31 count. Phase 6 is complete. Phase 7 adds `argent_2020.lua` (lookup 2012), `argent_2021.lua` (`rates`), and `argent_2022.lua` (QueryRef 2001). Not applied. Phase 8 waits for his report.

### 2026-10-07 — Phase 7 applied, Phase 8 written

Andrew confirmed migration 2022 applied and said to keep going. He did not quote a Test 31 count. Phase 7 is complete. Phase 8 adds `argent_2023.lua` (`tax_codes`) and `argent_2024.lua` (`tax_rates`). Not applied. `lines` is unchanged. Phase 9 waits for his report.

### 2026-10-07 — Phase 8 applied, Phase 9 written

Andrew confirmed migration 2024 applied. He did not quote a Test 31 count. Phase 8 is complete. Phase 9 adds `argent_2025.lua` (lookup 2009), `argent_2026.lua` (lookup 2010), `argent_2027.lua` (`tags`), `argent_2028.lua` (`tag_links`), and `argent_2029.lua` (`attachments`). Not applied. Phase 10 waits for his report.

### 2026-10-07 — Phase 9 applied, Phase 10 script updated

Andrew confirmed migration 2029 applied. Tests 31 and 71 passed. He did not quote a Test 31 count. Phase 9 is complete. That Test 71 run used `DESIGNS` set to Acuranzo alone. Phase 10 sets `tests/test_71_database_diagrams.sh` to 3.1.0 and adds `argent` to `DESIGNS`. No migration was added. `mks` exited 0 (Test 92, 193 files, 0 fail). Phase 11 waits for his Test 71 report.

### 2026-10-07 — Phase 10 sidequest, empty Argent diagrams

Andrew said the latest build and apply is migration 2029, and Test 71 completes. All 120 Argent SVGs were zero bytes. The sidequest is Test 71 3.2.0: seven engines, a built-in page template, and a retry of empty files. No migration was added. Phase 11 waits for his 3.2.0 report.

### 2026-10-07 — Phase 10 closed

Andrew reported Test 71 completed. Diagnostics `test_71_20261007_113705_483159983_2387479` record version 3.2.0, 1490 passed, 0 failed, elapsed 4422.095s, and 2912 combinations. Argent SVGs for migrations 2000–2029 are non-empty on all seven engines. Acuranzo SVGs on those engines are non-empty as well. Phase 10 is complete. No migration was added. The database stays at migration 2029. Phase 11 has not started. The next file is `argent_2030.lua`.

### 2026-10-07 — Phase 11 scripts written

`argent_2030.lua` through `argent_2037.lua` install the Argent MCP scripts. Luacheck reported 0 warnings. SQLite expansion left no `${...}` in the stored bodies, and `luac -p` accepted them. The files are not applied. Work item 11.7 stays open: Andrew regenerates the payload, runs tests 32–39, and records the MCP round-trip. Phase 12 has not started. The confirmed apply high-water remains migration 2029.

### 2026-10-07 — Test 73 written

Andrew asked for Test 73 on all eight engines, covering every Argent MCP tool and the variants each tool returns. `tests/test_73_argent_mcp.sh` 1.0.0, `tests/lib/argent_mcp_helpers.sh` 1.0.1, and eight `hydrogen_test_73_argent_mcp_*.json` configs are in the tree. Web ports are 15730–15736 and 15738. MCP ports are 15740–15746 and 15748. Payload is `acuranzo+argent` with AutoMigration true on the demo connections. Test 92 (`mks`) exited 0: 205 shell files, 0 fail, 25.467s. The test was not run. Work item 11.7 stays open. Phase 12 has not started. The confirmed apply high-water remains migration 2029.

### 2026-10-07 — Test 73 run, seven engines blocked on old column names

Andrew said everything is applied. `demo.queries` has argent 2000–2037 at type 1003. Test 73 1.0.0 printed `extra PASS/FAIL without TEST` on each config and listed every prereq miss. Version 1.0.2 removes that warning and reports the root tool error. The 15:29 run is 14 passed, 8 failed, 53.105s. SQLite passed 178 cases. PostgreSQL, MySQL, MariaDB, DB2, Firebird, YugabyteDB, and MSSQL fail in `UpsertOrganization` because `organizations.status_a200`, `ledgers.ledger_type_a201`, and `ledgers.status_a202` are still the first-apply names. The tools use `status_a2000`, `ledger_type_a2001`, and `status_a2002`. Those columns were not renamed. Work item 11.7 stays open. Phase 12 has not started.

### 2026-10-07 — PostgreSQL and DB2 reloads still fail Test 73

Andrew fully reset PostgreSQL and DB2. Diagnostics `test_73_20261007_164151`: both engines pass 65 and fail 113, with 100 of the failures waiting on a ledger. PostgreSQL rejects `CASE WHEN :USE_PARENT = 0 THEN NULL ELSE :PARENT_ID END` because the expression is `text`. DB2 returns SQL0418N for that `NULL` and SQL0302N for `NULLIF(:ORG_SUMMARY, '')` when the summary is `hello`. The 1.0.1 edits cast those parameters in `argent_2014.lua`, `argent_2022.lua`, and `argent_2030.lua` through `argent_2037.lua`. [`GUIDE.md`](/docs/He/GUIDE.md) now has **Portable named parameters**. The stored rows do not change until he loads these files again. Work item 11.7 stays open. Phase 12 has not started.

### 2026-10-07 — DB2 rollup rejects WITH RECURSIVE

Andrew refreshed SQLite, DB2, and PostgreSQL. Diagnostics `test_73_20261007_173712`: SQLite 178/178, PostgreSQL 178/178, DB2 177 pass and 1 fail (`FAIL_bal_parents`, SQL0104N, unexpected token `req` after `WITH RECURSIVE`). `argent_2022.lua` 1.0.2 stores `WITH` for DB2 and SQL Server. [`GUIDE.md`](/docs/He/GUIDE.md) records that under **Recursive common table expressions**. The stored QueryRef 2001 row stays the 1.0.1 text until the payload is rebuilt and DB2 loads migration 2022 again. MySQL, MariaDB, Firebird, YugabyteDB, and MSSQL on that run still report the first-apply column names. Tests 32–39 were not reported. Work item 11.7 stays open. Phase 12 has not started.

### 2026-10-07 — DB2 JOIN, MySQL CAST, Firebird sums and blobs

Diagnostics `test_73_20261007_182200`: PostgreSQL, SQLite, MariaDB, and MSSQL are 178/178. DB2 is 177/178. The `WITH RECURSIVE` error is gone. `FAIL_bal_parents` is SQL0345N (`SQLSTATE` 42836): the recursive fullselect of `DEMO.DESCENDANTS` used `JOIN ... ON`. Firebird is 171/178. `SUM` of `BIGINT` is INT128, so `balance_cents` is omitted, and `json_ingest` parameters are unbound blobs, so idempotency JSON is stored null. MySQL is 31/147. `UpsertOrganization` fails at `CAST(:ORG_SUMMARY AS varchar(255))`. YugabyteDB returned HTTP 401 on login and ran no cases. `argent_2022.lua` 1.0.3 stores comma joins and keeps `WITH` for DB2 and SQL Server. `argent_2014.lua` 1.0.2 casts the Firebird sum to `BIGINT`. `argent_2030.lua` and `argent_2032.lua` through `argent_2036.lua` cast each json parameter before `json_ingest`, and those files plus `argent_2037.lua` use MySQL `signed` and `char(255)`. `argent_2031.lua` did not change. [`GUIDE.md`](/docs/He/GUIDE.md) records the four rules. The stored rows stay the previous text until the payload is rebuilt and those migrations are loaded again. Tests 32–39 were not reported. Work item 11.7 stays open. Phase 12 has not started.

### 2026-10-08 — Test 73 is green on all eight engines

Andrew ran `database_reset.sh`, then `database_load.sh`. The test schemas (tests 32–39) and the demo schemas each came back with 66 tables. TestMigration stayed off, so that load is not the reverse suite. Test 73 `test_73_20261008_085126` then passed 178 cases on PostgreSQL, YugabyteDB, SQLite, MariaDB, DB2, MSSQL, MySQL, and Firebird. The harness is 22 pass, 0 fail, 172.497s. The DB2 rollup that ran is `FROM DEMO.ledgers p, DEMO.ledgers c, req`. MySQL sent `demo.json_ingest(CAST(:ORG_COLLECTION AS char(255)))`. Firebird `GetLedger` returned `balance_cents` -500, and the idempotency retry returned `created` false. The 07:38 run had still been executing the previous stored SQL. Work item 11.7 stays open until tests 32–39 are reported. Phase 12 has not started.

### 2026-10-08 — Phase 11 closed and Phase 12 written

Andrew directed work item 11.7 closed. The report he accepted is `database_load.sh` (TestMigration off, forward only) plus Test 73 `test_73_20261008_085126` (178/178 on all eight engines, harness 22/22, 172.497s). The reverse half of tests 32–39 was not run.

Phase 12 is written and not applied. `argent_2038.lua` creates `confirm_tokens`. `argent_2039.lua` installs `EditTransaction` and `RescindTransaction`. `argent_2040.lua` through `argent_2044.lua` install `PostStatement`, `PostPeriodClose`, `StartReconciliation`, `ClearLines`, and `CompleteReconciliation`. Luacheck reported 0 warnings on the seven files. Expansion for postgresql, sqlite, mysql, db2, mariadb, firebird, and mssql left no `${...}` in the pre-base64 SQL, and each `[[ ]]` block names each parameter once. `luac -p` accepted the migration files and the extracted script bodies. Work items 12.1–12.3 are checked. Work item 12.4 is open. The Argent README lists 45 files, 278 statements, and 45 diagrams. Phase 13 has not started.

### 2026-10-08 — Test 73 1.0.3 contains the Phase 12 fixture

`tests/test_73_argent_mcp.sh` and `tests/lib/argent_mcp_helpers.sh` are 1.0.3. The exercise adds the seven Phase 12 tools and the confirm and reconciliation cases. On the path where every prerequisite is present, that is 271 tool cases. Five session cases sit beside them. The count is `EXPECTED_TOOL_CASES` plus 5, not a hardcoded total. Test 92 (`mks`) exited 0: 207 shell files, 0 fail, 29.571s. The blackbox was not run. The current payload does not contain `argent_2038.lua` through `argent_2044.lua`, so `tools/list` cannot see the new tools until Andrew regenerates the payload and applies those files. Work item 12.4 stays open. Phase 13 has not started.

### 2026-10-08 — Test 73 1.0.3 ran; DB2 SQL0100W and one Yugabyte 503

Andrew had applied through `argent_2044.lua`. Diagnostics `test_73_20261008_115801`: PostgreSQL, SQLite, MariaDB, Firebird, MSSQL, and MySQL passed 276/276. DB2 passed 250 and failed 26. The first failure is `clear_empty`: `ClearLines` ran `UPDATE lines SET reconciliation_id = NULL ... WHERE reconciliation_id = :RECON_ID AND cleared = 0` on a reconciliation with no linked line. DB2 returned `SQL0100W` and the tool returned `update_failed`. `clear_buy` is the same statement. The other 24 DB2 failures follow because the purchase stays Recorded and the reconciliation stays open. `edit_body_long` then writes a 4001-character description as a safe edit and DB2 returns `CLI0109E`. YugabyteDB passed 275 and failed `tools_list` with HTTP 503, body `Authentication service unavailable`. The log shows QueryRef 18 (`conduit_14_1791485891`) hitting the 20 second auth budget; the query later finished with one row. The other tool calls succeeded.

`argent_2043.lua` 1.0.1 selects before that unlink and skips the update when the select is empty. `argent_2044.lua` 1.0.1 does the same before `SET cleared = 1`. The DB2 driver is unchanged. Test 73 and the helper are 1.0.4. `argent_rpc` retries HTTP 503 once. Luacheck reported 0 warnings on the two migrations. Test 92 exited 0: 206 shell files, 0 fail, 28.594s. The blackbox was not re-run. APPLY is already 2044, so AutoMigration will not install 1.0.1 until the payload is regenerated and those two files are loaded again. Work item 12.4 stays open. Phase 13 has not started.

The parallel wall clock was the slowest engine, about 5 minutes 18 seconds (YugabyteDB). Six engines shut down 34–39 seconds after ready. MySQL took about 4 minutes 34 seconds and still passed. Ready was about 3 seconds and migration was 0.002–0.004 seconds. `READY_TIMEOUT` 300 and the HTTP max-time of 90 were not what the clock spent. Each engine ran about 1800 queries. The logged `time: N ms` value is microseconds. Summed, PostgreSQL is 1.8 seconds and SQLite is 1.3 seconds. MySQL is 263.6 seconds and YugabyteDB is 300.2 seconds, almost all of it queries between 0.1 and 0.5 seconds. MySQL is `10.118.0.3:3306`. YugabyteDB is `adm-c:30543`. Tightening the HTTP or ready ceilings would not shorten the run.

### 2026-10-08 — Phase 12 closed

Andrew reported migration 2044 applied on every engine and Test 73 fully passing. Diagnostics `test_73_20261008_145836` (script 1.0.4) record 276 `CASE_PASS` and 0 `CASE_FAIL` on PostgreSQL, YugabyteDB, SQLite, MariaDB, DB2, MSSQL, MySQL, and Firebird. `EXPECTED_TOOL_CASES` is 271. `READY=1` and `LOGIN_OK=1` on each file. PostgreSQL's log shows argent AVAIL = LOAD = APPLY = 2044 and migration completed in 0.003s. The YugabyteDB log has no auth-query timeout. Hydrogen elapsed time was 18.032s on SQLite, 18.868s on PostgreSQL, 19.139s on MariaDB, 20.174s on DB2, 20.738s on MSSQL, 24.045s on Firebird, 253.121s on MySQL, and 278.169s on YugabyteDB. Work item 12.4 is checked. Test 31 and the full Test 98 were not re-run after the 1.0.1 edit. The harness summary table was not in the diagnostics directory. Phase 13 has not started.

### 2026-10-08 — Phase 13 implemented

`H.http.request` and `H.http.request_sync` are in the tree. `H.http.get` and `H.http.post` call `scripting_http_request`. The allowlist is the eight exact tokens. `CONNECT`, `TRACE`, and a lowercase token are errors. A 207 or 412 is the result table. GET and POST keep `oidc_rp_http_get_with_headers_slist` and `oidc_rp_http_post_with_headers_slist`. The other verbs use `oidc_rp_http_request_with_headers_slist` (`CURLOPT_CUSTOMREQUEST`) in the existing `oidc_rp_http.c`. No new `static` function. No new `src/` file. No migration.

`cmake -S . -B ../build --preset default` from `cmake/` reconfigured `build/` without wiping it. The binaries then passed: `http_client_test_request` 7 tests, `scripting_api_http_test_request` 8, `http_pool_test_worker_process_one` 5, `http_client_test_get` 4, `http_client_test_post` 4, `scripting_api_http_test_async` 5, and `scripting_api_http_test` 33. Each report is 0 failures. The worker test rejects `CONNECT` because `DELETE` is allowlisted. Markdownlint on [`LUA_GUIDE.md`](/docs/H/LUA_GUIDE.md) and [`lua_api.md`](/docs/H/core/subsystems/scripting/lua_api.md) exited 0. Work items 13.1–13.4 are checked. Work item 13.5 is open. `mkp` was not run. Andrew runs `mkq` on this build directory (or `mkt`, which wipes `build/`), then `mkp`, then the three named `mku` tests. Phase 14 has not started.

### 2026-10-08 — Phase 13 closed

Andrew reported that all Unity framework unit tests are passing and `mkp` is passing. He did not quote a Unity total or an `mkp` file count. Work item 13.5 is checked. No migration was added. The next free Argent file is `argent_2045.lua`. Phase 14 has not started.

### 2026-10-08 — Phase 14 implemented

`argent_2045.lua` is `UpsertSchedule`. `argent_2046.lua` is `GenerateSchedule`. `argent_2047.lua` is `MatchReserved`. `argent_2048.lua` is `RetryCalendar`. No columns were added. No earlier migration was edited. Luacheck on the four files reported 0 warnings. Test 98 1.1.1 at 17:21:57 found no issues in 531 files (2 pass, 0 fail, 4.485s). That run linted the four new files and used the cache for the other 527. Expansion through `tests/lib/get_migration.sh` for postgresql, sqlite, mysql, db2, mariadb, firebird, and mssql left no `${...}` in the 28 SQL files. The full Test 31 harness was not run, so there is no Test 31 count. `luac -p` accepted the migration files and the extracted script bodies.

Test 73 and `tests/lib/argent_mcp_helpers.sh` are 1.0.5. The exercise adds the four tools and 24 cases: validation, a weekly schedule, four Reserved rows, a skipped regenerate, one match inside a zero window, then a calendar URL of `http://127.0.0.1:9/cal/` and a second match that must stay saved. Test 92 4.1.1 at 17:21:57 found no issues in 207 shell files (3 pass, 0 fail, 27.120s). Test 73 was not run. The payload on disk does not contain 2045–2048 until Andrew regenerates it, so `tools/list` cannot see the new tools until he does that and applies the files.

Work items 14.1–14.3 are checked. Work item 14.4 is open. The Argent README lists 49 files, 298 statements, and 49 diagrams. Phase 15 has not started.

### 2026-10-08 — Phase 14 closed

Andrew reported Test 73 1.0.5 passing. The harness table is 22 pass, 0 fail, 314.196s. Diagnostics `test_73_20261008_173729` record 300 `CASE_PASS` and 0 `CASE_FAIL` on PostgreSQL, YugabyteDB, SQLite, MariaDB, DB2, MSSQL, MySQL, and Firebird. `EXPECTED_TOOL_CASES` is 295. Every engine's migration summary is argent AVAIL = LOAD = APPLY = 2048. SQLite applied `argent_2045.lua` through `argent_2048.lua` during the run and finished migration in 7.425s. The other seven engines finished the migration pass in 0.002–0.003s.

PostgreSQL `gen_month` created four Reserved rows, txn 64–67, on `2026-10-01`, `2026-10-08`, `2026-10-15`, and `2026-10-22`. Each is status 1, kind 11, calendar state 1, and `calendar_attempts` 0. `match_one` saved txn 68 at status 3 and rescinded txn 64 to status 5, with calendar state 1. `match_down` saved txn 69 at status 3. Calendar state is 4, attempts is 1, and `calendar_error` is `H.wait: Could not connect to server`. `retry_down` left status 3, raised attempts to 2, and returned tried 1, failed 1, set 0. The same calendar outcome is on all eight engines: `match_down` is ok, status 3, state 4, attempts 1, and that same error string. `retry_down` is attempts 2, tried 1, failed 1, set 0, status 3.

Hydrogen elapsed time was 20.870s on PostgreSQL, 21.204s on MariaDB, 22.390s on DB2, 22.517s on MSSQL, 26.376s on SQLite, 27.486s on Firebird, 280.964s on MySQL, and 313.581s on YugabyteDB. Work item 14.4 is checked. The full Test 31 harness was not run. Phase 15 has not started.

*End of Argent plan.*
