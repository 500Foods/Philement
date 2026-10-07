# Argent — phased implementation plan

**Date:** 2026-10-06 (PT)
**Author:** Folly (for Andrew)
**Status:** Phase 3 complete. Phase 4 in progress: transactions, lines, and QueryRef 2000 written, not yet applied.
**Design name:** Argent
**Helium path:** `elements/002-helium/argent/`
**Database:** the Acuranzo database (same schema, same `queries` / `lookups` / `scripts`). Optional pack. Never applied alone.
**Migration series:** `argent_2xxx.lua`. On disk through `argent_2014.lua` (QueryRef 2000). Phase 4 files are not applied yet.

The earlier Folly copies named `/workspace/folly/argent-plan.md` and `/workspace/folly/hydrogen-bookkeeping-decisions.md` are not on this machine. Decisions from that work are in this file. Amend this file. Do not hunt for the Folly paths.

## Phases

Effort is the remaining work, or the size of the phase when it is already done. Easy, medium, or hard.

| Phase | Status | Effort |
| --- | --- | --- |
| 0 Design lock | Complete. Approved 2026-10-06 | Medium |
| 1 Organizations and ledgers | Complete. Applied 2026-10-07 | Easy |
| 2 Lookup seeds 2000–2002 | Complete. Applied 2026-10-07 | Easy |
| 3 Currencies, terms, contacts | Complete. Applied 2026-10-07 | Medium |
| 4 Transactions, lines, balance query | In progress. Files written. Apply is the close | Hard |
| 5 Reconciliations | Not started | Easy |
| 6 Schedules | Not started | Easy |
| 7 Rates and parent rollup query | Not started | Hard |
| 8 Tax | Not started | Medium |
| 9 Tags and attachments | Not started | Medium |
| 10 Diagrams | Not started | Easy |
| 11 MCP CRUD and posting | Not started | Hard |
| 12 Confirm and reconciliation tools | Not started | Hard |
| 13 `H.http.request` | Not started | Medium |
| 14 Schedules and calendar sync | Not started | Hard |
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

Phase 4 is open until Andrew applies `argent_2009.lua` through `argent_2014.lua` and reports it. That apply is his closing step. The next conversation records the result. It does not add Phase 5 files unless he has closed Phase 4.

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

`schedule_id` PK. `organization_id` NOT NULL. `status_a2007`. `name` `${TEXT}`. `from_ledger_id` NOT NULL. `to_ledger_id` NOT NULL. `amount_cents` NOT NULL. `currency` NOT NULL. Both ledgers use that currency. Mixed currency is a Phase 7 concern and is rejected in Phase 6. `tax_code_id` NULL. `rrule` `${TEXT}` NOT NULL. `anchor_on` `${DATE}` NOT NULL. `end_on` NULL. `estimate_flag` `${INTEGER_SMALL}`. `horizon_mode_a2008`. `summary` `${TEXT_BIG}`. `collection` `${JSON}`. `${COMMON_CREATE}`.

### `rates` (Phase 7)

`rate_id` PK. `base_currency` NOT NULL. `quote_currency` NOT NULL. `source_a2012` NOT NULL. `as_of` `${DATE}` NOT NULL. `rate_n` NOT NULL. `rate_d` NOT NULL. `txn_id` NULL. `summary` `${TEXT}`. `collection` `${JSON}` (raw BoC snippet when the source is `boc`). `${COMMON_CREATE}`. Unique `(base_currency, quote_currency, source_a2012, as_of)`.

### `tax_codes` and `tax_rates` (Phase 8)

`tax_codes`: `tax_code_id` PK. `organization_id` NOT NULL. `code` `${VARCHAR_50}` (`GST`, `PST-BC`, `EXEMPT`). `name` `${TEXT}`. `target_ledger_id` NOT NULL. `summary` `${TEXT_BIG}`. `collection` `${JSON}`. `${COMMON_CREATE}`. No status column.

`tax_rates`: `tax_rate_id` PK. `tax_code_id` NOT NULL. `effective_on` NOT NULL. `rate_bps` NOT NULL (500 means 5.00%). `summary` `${TEXT}`. `collection` `${JSON}`. `${COMMON_CREATE}`.

### `tags` and `tag_links` (Phase 9)

`tags`: `tag_id` PK. `organization_id` NULL means global. `name` `${TEXT}` NOT NULL. `summary` `${TEXT}`. `collection` `${JSON}`. `${COMMON_CREATE}`.

`tag_links`: `tag_link_id` PK. `tag_id` NOT NULL. `entity_type_a2009` NOT NULL. `entity_id` NOT NULL. `${COMMON_CREATE}`. Unique `(tag_id, entity_type_a2009, entity_id)`.

### `attachments` (Phase 9)

Argent-native. Not a link to Acuranzo `documents`. A row may be a file, a note, or both.

`attachment_id` and `rev_id`, PK `(attachment_id, rev_id)`. `entity_type_a2009` NOT NULL. `entity_id` NOT NULL. `txn_id` NULL, set when the entity is a transaction. `att_type_a2010` NOT NULL. `mime_type` NULL (`text/plain` for a note). `file_name` NULL for a pure note. `file_data` `${TEXT_BIG}` NULL (base64; null for a pure note). `file_text` `${TEXT_BIG}` NULL (note body and extracted text). `byte_len` NULL. `name` `${TEXT}` NOT NULL. `summary` `${TEXT_BIG}`. `collection` `${JSON}`. `${COMMON_CREATE}`. No status column. CalDAV secrets never go in `collection`.

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
- [ ] 4.5 Andrew applies and reports the result, including the QueryRef row disappearing on reverse. Test 31 and Test 98 results are written here when he includes them.

### Done means

QueryRef 2000 is installed and reversed on tests 32–39. The SQL rejects nothing by itself: the per-currency zero-sum is a Phase 11 Lua rule.

### Exit gate

Andrew applies. The agent does not apply.

### Status

**In progress.** The six files are on disk. Andrew applies as the closing step.

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

- [ ] 5.1 Seed lookup 2006 (open, completed, voided) in its own file.
- [ ] 5.2 Create `reconciliations` in its own file. Do not alter `lines`.
- [ ] 5.3 Andrew runs tests 32–39. Reverse drops the table and the lookup keys only.

### Done means

The recon table and lookup 2006 apply and reverse on tests 32–39.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply.

### Status

**Not started.**

### Handoff

Phase 6 adds `schedules` and seeds lookups 2007 and 2008, each in its own file. `schedule_id` and `replaces_txn_id` are already on `transactions`. Mixed-currency schedules are rejected by the later tool, not by a rate join in this phase.

---

## Phase 6 — Schedules

### Goal

Store a recurring template. Do not generate Reserved transactions yet.

### Work items

- [ ] 6.1 Seed lookup 2007 and lookup 2008, one family per file.
- [ ] 6.2 Create `schedules` with the full column list. Do not alter `transactions`.
- [ ] 6.3 Andrew runs tests 32–39.

### Done means

The schedule table and lookups 2007 and 2008 apply and reverse on tests 32–39.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply.

### Status

**Not started.**

### Handoff

Phase 7 seeds lookup 2012, creates `rates` with `source_a2012` and nullable `txn_id`, and installs QueryRef 2001 in its own file. It does not fetch BoC. Fetch is `Argent.GetBocRate` in Phase 15. Key 6 `implied` is a normal source. One row per pair per day.

---

## Phase 7 — Rates and the parent rollup

### Goal

Store named-source quotes and convert parent ledgers with QueryRef 2001.

### Work items

- [ ] 7.1 Seed lookup 2012 in its own file, including an `icon` in `collection` for each key.
- [ ] 7.2 Create `rates` with unique `(base_currency, quote_currency, source_a2012, as_of)` and nullable `txn_id`.
- [ ] 7.3 Install QueryRef 2001 in its own file. Parent currency is the parent's `ledgers.currency`. Child conversion uses the newest `rates` row for that pair and source with `as_of` on or before the requested date. Default source is lookup 2012 key 1. Missing rate: null converted amount and a warning column.
- [ ] 7.4 Andrew runs tests 32–39.

### Done means

QueryRef 2001 installs and reverses on tests 32–39. A fixture rate is not required for the gate. The gate is the migration apply.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply.

### Status

**Not started.**

### Handoff

Phase 8 adds `tax_codes` and `tax_rates`, each in its own file. The tax columns are already on `lines`. No tax-status lookup.

---

## Phase 8 — Tax

### Goal

Store a tax code and its dated rate. The line columns a tax split writes already exist.

### Work items

- [ ] 8.1 Create `tax_codes` in its own file. No status lookup.
- [ ] 8.2 Create `tax_rates` in its own file. Do not alter `lines`.
- [ ] 8.3 Andrew runs tests 32–39.

### Done means

Both tax tables apply and reverse on tests 32–39.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply.

### Status

**Not started.**

### Handoff

Phase 9 adds `tags`, `tag_links`, and `attachments`, and seeds lookups 2009 and 2010, each in its own file. It does not add an attachment-status lookup.

---

## Phase 9 — Tags and attachments

### Goal

Store tags and revisioned notes or files on any Argent entity.

### Work items

- [ ] 9.1 Seed lookup 2009 and lookup 2010, one family per file.
- [ ] 9.2 Create `tags` and `tag_links`.
- [ ] 9.3 Create `attachments` with PK `(attachment_id, rev_id)`. A note has `file_data` null and `file_text` set.
- [ ] 9.4 Andrew runs tests 32–39.

### Done means

Tag and attachment tables apply and reverse on tests 32–39.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply.

### Status

**Not started.**

### Handoff

Phase 10 may edit `tests/test_71_database_diagrams.sh` only, to add `argent` to `DESIGNS`, plus the script's `CHANGELOG` and `TEST_VERSION`. It does not add a migration. Schema phases leave Test 71 on Acuranzo alone.

---

## Phase 10 — Diagrams

### Goal

Test 71 diagrams the Argent tables as well as Acuranzo.

### Work items

- [ ] 10.1 Add `argent` to `DESIGNS` in `tests/test_71_database_diagrams.sh`.
- [ ] 10.2 Bump that script's `CHANGELOG` and `TEST_VERSION`.
- [ ] 10.3 `mks` is clean. Andrew runs Test 71 and the result is recorded here.

### Done means

Test 71 exits 0 with `argent` in `DESIGNS`.

### Exit gate

`mks`, then Test 71. Andrew runs Test 71.

### Status

**Not started.**

### Handoff

Phase 11 adds `Argent.*` rows to `scripts` for the CRUD and posting tools listed in that phase. It does not add `H.http.request`. It does not implement confirm tokens. QueryRefs 400 and 401 already exist. Tools call them. They do not install a second copy.

---

## Phase 11 — MCP CRUD and posting

### Goal

Folly can create an organization, two posting ledgers, and one balanced transaction through MCP.

### Work items

- [ ] 11.1 Scripts for `ListOrganizations`, `UpsertOrganization`, `ListLedgers`, `GetLedger`, `UpsertLedger`, `UpsertLedgerTerms`, `UpsertContact`.
- [ ] 11.2 `PostTransaction` rejects a transaction whose per-currency line sum is not 0. It writes the header and lines when the sum is 0. Idempotency key is stored in `collection`.
- [ ] 11.3 `UpsertLedger` writes the opening transaction (kind 6) and `opening_txn_id`.
- [ ] 11.4 `AddTags`, `RemoveTags`, `AddAttachment`. `UpsertTaxCode` and `UpsertTaxRate`. `PostTransaction` posts companion tax lines when `tax_code_id` is set.
- [ ] 11.5 `GetTransaction`, `ListTransactions`, and `QueryBalances` (QueryRef 2000, and 2001 when parents are requested). Status values are lookup 2003 keys.
- [ ] 11.6 One script migration per tool, or one migration per tool group where the file stays under 1000 lines. `mcp_access=1`, group `Argent`. No second caller-facing QueryRef in a file that already installs one.
- [ ] 11.7 Andrew runs tests 32–39 for the new script migrations, then an MCP round-trip: create org, two ledgers, one balanced txn, one unbalanced txn rejected.

### Done means

The round-trip in 11.7 is recorded, including the rejected unbalanced transaction.

### Exit gate

Test 31, Test 98, payload regenerate, tests 32–39, then the MCP round-trip. Test 47's shape is the pattern. This phase does not add a new blackbox script unless 11.7 cannot be recorded any other way, and then only with Andrew's ask.

### Status

**Not started.**

### Handoff

Phase 12 adds `confirm_tokens` and the recon and edit tools. It leaves the Phase 11 scripts in place and calls them. A reconciled edit without a token must not write.

---

## Phase 12 — Confirm and reconciliation

### Goal

Dangerous edits do not write until the same body comes back with a confirm token. A reconciliation can be completed, including an override that carries a reason.

### Work items

- [ ] 12.1 Create `confirm_tokens` as specified. No HMAC.
- [ ] 12.2 `EditTransaction` and `RescindTransaction` implement the warn path and the knock-back to Recorded.
- [ ] 12.3 `PostStatement`, `PostPeriodClose`, `StartReconciliation`, `ClearLines`, `CompleteReconciliation`. Override without `override_reason` is rejected when the balances differ.
- [ ] 12.4 Andrew's fixture: edit a reconciled txn, observe `needs_confirm` and no mutation, retry with the token, observe Recorded.

### Done means

The fixture in 12.4 is recorded. An override without a reason is rejected.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply, Andrew's fixture report.

### Status

**Not started.**

### Handoff

Phase 13 is C, in `scripting_api_http.c`, `http_client.c`, and `http_pool.c`. It does not add a migration. It does not start the calendar worker. `H.http.get` and `H.http.post` stay as wrappers.

---

## Phase 13 — `H.http.request`

### Goal

Lua can send the CalDAV verbs. Non-2xx responses come back as data.

### Work items

- [ ] 13.1 `H.http.request(method, url, body, headers, opts)` and `H.http.request_sync`. Allowlist: `GET`, `POST`, `PUT`, `DELETE`, `PROPFIND`, `REPORT`, `MKCALENDAR`, `PROPPATCH`. Any other token, including `CONNECT` and `TRACE`, is an error.
- [ ] 13.2 `get` and `post` call the same path. A 207 or 412 returns `{ status, headers, body, elapsed_ms }`.
- [ ] 13.3 Unity tests for an allowed verb, a rejected verb, and a non-2xx body. No new `static` function. Prototypes in the existing headers.
- [ ] 13.4 Update [`LUA_GUIDE.md`](/docs/H/LUA_GUIDE.md) and [`lua_api.md`](/docs/H/core/subsystems/scripting/lua_api.md).
- [ ] 13.5 Andrew runs `mkq` (or `mkt` if a new `src/` file was added) and `mkp`. Named `mku` results are written here.

### Done means

The Unity tests pass and `mkp` is clean. No migration was added.

### Exit gate

`mkq` or `mkt`, then `mkp`, then the named Unity tests. Andrew runs them.

### Status

**Not started.**

### Handoff

Phase 14 adds the schedule and calendar tools. Lookup 2011 and the calendar columns already exist. The worker calls `H.http.request`. Credentials come from the environment. A failed HTTP call does not roll back the transaction.

---

## Phase 14 — Schedule generation and calendar sync

### Goal

Generate a month of Reserved rent, match an actual, and save a transaction while CalDAV is down.

### Work items

- [ ] 14.1 `UpsertSchedule`, `GenerateSchedule` through fiscal year end, `MatchReserved`. Do not add columns.
- [ ] 14.2 After a successful save, set calendar state pending (lookup 2011 key 2). The worker uses `H.http.request` with `ARGENT_CAL_*`. Failure increments `calendar_attempts`, stores `calendar_error`, and leaves the transaction saved.
- [ ] 14.3 `RetryCalendar`.
- [ ] 14.4 Andrew's fixture: a month of Reserved rows, one matched actual, one save with the calendar host unreachable. The Reserved status written is lookup 2003 key 1.

### Done means

The fixture in 14.4 is recorded. The unreachable host did not fail the save.

### Exit gate

Test 31, Test 98, payload regenerate, Andrew's apply, Andrew's fixture.

### Status

**Not started.**

### Handoff

Phase 15 installs QueryRefs 2002–2010, one per file, and the remaining read tools, plus `UpsertRate` and `GetBocRate`. It does not change the Phase 11 posting rules.

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

*End of Argent plan.*
