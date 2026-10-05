# Argent — phased implementation plan

**Date:** 2026-10-05 (PT)  
**Author:** Folly (for Andrew)  
**Status:** Plan only — implement after Phase 0 gate  
**Authority:** `/workspace/folly/hydrogen-bookkeeping-decisions.md` (33 decisions; supersedes the earlier `fin_*` / Acuranzo-extension proposal)  
**Design name:** Argent  
**Helium path:** `elements/002-helium/argent/` (new)  
**Yugabyte schema:** `argent`  
**Migration series:** `argent_5xxx.lua` (bootstrap `argent_5000.lua`; next free thousand after acuranzo 1xxx, gaius 2xxx, glm 3xxx, helium 4xxx)

---

## 0. How a separate Helium design is laid out (verified on Angrin)

Cited under `/mnt/extra/Projects/Philement`:

| Pattern | Evidence |
|--------|----------|
| One folder per design | `elements/002-helium/{acuranzo,gaius,glm,helium}/` each with `README.md` + `migrations/` |
| Own `database*.lua` copy | Every design ships `database.lua` + per-engine files (`database_postgresql.lua`, `database_sqlite.lua`, …) |
| Own migration numbering | Bootstrap `*_1000` / `*_2000` / `*_3000` / `*_4000` creates that design’s `queries` table |
| SQLite has empty `${SCHEMA}` | `MACRO_REFERENCE.md`: “SQLite and Firebird use an empty schema prefix.” Yugabyte/PG use `Schema` from Hydrogen config (e.g. `"Schema": "acuranzo"` / `"demo"`). |
| Designs do **not** share tables via FK | Zero `FOREIGN KEY` / `REFERENCES` across Acuranzo migrations. Cross-design identity is by integer id + Lua `H.altquery(database_name, …)`. |
| Payload / tests wire designs by name | `payloads/payload-generate.sh` and `tests/test_31_migrations.sh`: `DESIGNS=("helium" "acuranzo")`. Add `"argent"`. Config: `"Design"` / `"Migrations": "PAYLOAD:argent"` / `"Schema": "argent"`. |
| MCP tools live in that design’s `scripts` | Seeded like `System.Info` / `Mcp.Echo` (`acuranzo_1376.lua`, `acuranzo_1370.lua`): `group_name`, `script_name`, `mcp_access=1`, `mcp_schema`, `mcp_annotations`. |
| No Python | Lua migrations + Lua MCP scripts only. |

**Implication for Argent:** own schema on Yugabyte (`argent.*`); own SQLite file in tests (empty schema prefix). Unprefixed table names must not collide with Acuranzo’s list so a single-file SQLite dump of both remains safe (decision 22). Bootstrap tables `queries` / `lookups` / `scripts` are duplicated per design by design — they live in separate schemas/files, same as helium/glm/gaius.

**Acuranzo table names checked (do not reuse for Argent domain tables):**  
`account_access`, `account_canvas_alerts`, `account_canvas_links`, `account_contacts`, `account_oidc_identities`, `account_roles`, `accounts`, `account_settings`, `actions`, `catalog_events`, `connections`, `contact_submissions`, `convos`, `convo_segs`, `course_prices`, `courses`, `course_suggestions`, `dictionaries`, `documents`, `enrollment_events`, `languages`, `licenses`, `lists`, `lookups`, `mail_attempts`, `mail_events`, `mail_otp_codes`, `mail_queue`, `mail_routes`, `mail_templates`, `media_assets`, `notes`, `numbers`, `oauth_authorization_codes`, `oauth_clients`, `oauth_refresh_tokens`, `orders`, `queries`, `reports`, `roles`, `rules`, `scripts`, `sessions`, `systems`, `templates`, `tokens`, `user_enrollments`, `user_preferences`, `user_registration_meta`, `workflows`, `workflow_steps`.

---

## 1. Overview, goals, non-goals

### Goals

- Minimal but complete **double-entry** bookkeeping for Andrew personally and 500 Foods, productizable as Helium design **Argent**.
- Multicurrency from day one; each ledger has one fixed currency; mixed-currency transactions capture **implied rates by source**; BoC rates for views and premium comparison.
- Full reconciliation, tax codes, schedules → Reserved txns, non-blocking CalDAV sync, tags/attachments (liberal notes as text attachments).
- **MCP-first** API (`Argent.*`) with full read/write for Folly; warn-and-confirm on dangerous edits.
- Report **queries** now; a separate reporting tool renders later.
- Design must **not block** eventual QBO replacement (~2028): contact ledgers, invoice-as-transaction, tax summaries, permissions hooks — without building invoicing/payroll yet.

### Non-goals (now)

- Import pipelines, Plaid, bank feeds.
- Invoice generation / AR documents (QBO keeps invoices; Argent may later record them as transactions + PDF only).
- Payroll, CRA filings automation, QBO sync/import.
- Per-ledger permissions enforcement (schema stub only; Andrew + Folly full r/w).
- Pretty report UI.
- Overloading Acuranzo `accounts` / extending Acuranzo schema.

### Gate philosophy (decision 20)

Every phase ends with an explicit **GATE**: automated Hydrogen tests + Folly checklist + **Andrew sign-off**. No production data until the gate passes. Development uses the Hydrogen test harness; production is a separate DB/connection.

---

## 2. Schema

Conventions (all new tables unless noted):

- `${COMMON_CREATE}`: `valid_after`, `valid_until`, `created_id`, `created_at`, `updated_id`, `updated_at`.
- Soft delete / close: `valid_until` and/or status lookups — no `deleted_at`.
- Lookup columns: `*_aN` where N is Argent’s own `lookups.lookup_id`.
- Money: `currency` `${VARCHAR_20}` lowercase ISO 4217; amounts `${INTEGER_BIG}` **minor units** (cents). Signs: assets/expenses positive natural balance as stored on the line’s `amount_cents` with line sign convention below.
- PKs: `${INTEGER}` / `${INTEGER_BIG}` assigned by Lua (house pattern; no `${SERIAL}` in classic creates).
- No DB foreign keys (Acuranzo style); integrity in Lua.
- Dates: `${DATE}` for calendar dates; `${TIMESTAMP_TZ}` for audit and sync timestamps.

**Line sign convention (implement in Lua, document in README):** each line’s `amount_cents` is signed so that **for every currency present in a transaction, Σ amount_cents = 0**. Debit-positive for asset/expense ledgers; credit-positive for liability/equity/income — encoded by ledger type when tools compose lines, not by separate debit/credit columns.

### 2.1 Bootstrap (every Helium design)

| Table | Purpose | Migration |
|-------|---------|-----------|
| `queries` | Migration + QueryRef metadata | `argent_5000` |
| `lookups` | Argent lookup families | `argent_5001` |
| `scripts` | Lua MCP tools + orchestrator hooks | `argent_5xxx` (create early; MCP columns if not in base create) |

Login identity **reuses Acuranzo `accounts`** (decision 19). Argent stores only integer `account_id` values in `created_id` / permission rows. Lua resolves display names via `H.altquery('Acuranzo', …)` when needed. No Argent copy of passwords.

### 2.2 `organizations`

| Column | Type | Notes |
|--------|------|-------|
| `organization_id` | `${INTEGER}` PK | |
| `status_aNN` | `${INTEGER}` | active / archived |
| `name` | `${TEXT}` | `Andrew Simard`, `500 Foods` |
| `fiscal_year_start_month` | `${INTEGER}` | 1–12; 500 Foods = 1 (Jan; FYE Dec 31) |
| `fiscal_year_start_day` | `${INTEGER}` | usually 1 |
| `default_currency` | `${VARCHAR_20}` | `cad` |
| `summary` | `${TEXT_BIG}` | |
| `collection` | `${JSON}` | CRA BN, legal name, etc. |
| `${COMMON_CREATE}` | | |

### 2.3 `ledgers`

Five types (lookup): **asset, liability, equity, income, expense**. Optional `parent_id`. **Posting ledgers only** receive lines; parents are roll-up views (decision 24–25; confirm at Phase 0).

| Column | Type | Notes |
|--------|------|-------|
| `ledger_id` | `${INTEGER}` PK | |
| `organization_id` | `${INTEGER}` NOT NULL | |
| `parent_id` | `${INTEGER}` NULL | Nesting; parents have `is_posting=0` |
| `status_aNN` | `${INTEGER}` | open / closed / archive |
| `ledger_type_aNN` | `${INTEGER}` | asset/liability/equity/income/expense |
| `is_posting` | `${INTEGER_SMALL}` | 1 = accepts lines; 0 = parent roll-up only |
| `name` | `${TEXT}` | |
| `currency` | `${VARCHAR_20}` NOT NULL | **Fixed** for life of ledger |
| `opening_on` | `${DATE}` NOT NULL | Per-ledger opening (decision 14) |
| `opening_balance_cents` | `${INTEGER_BIG}` NOT NULL | Usually 0; opening txn also posted |
| `opening_txn_id` | `${INTEGER}` NULL | Link to opening balance transaction |
| `latest_reconciliation_id` | `${INTEGER}` NULL | |
| `latest_reconciled_on` | `${DATE}` NULL | Denormalized for warn logic |
| `calendar_url` | `${TEXT}` NULL | CalDAV calendar assignment |
| `calendar_id` | `${VARCHAR_100}` NULL | Provider id if distinct from URL |
| `mask` | `${VARCHAR_50}` NULL | Last4 only — never full PAN |
| `external_ref` | `${VARCHAR_100}` NULL | Future Plaid/QBO id |
| `summary` | `${TEXT_BIG}` | |
| `collection` | `${JSON}` | folder_path, quirks, sanctioned OD flag, etc. |
| `${COMMON_CREATE}` | | |

Counterparties (DO, Jason/rent, child support, Netflix, …) are ordinary ledgers (usually liability or expense+AP style as chosen) so **arrears = balances** (decision 2).

### 2.4 `ledger_terms` (dated)

Effective-dated commercial terms; current row = latest `effective_on` ≤ as-of.

| Column | Type | Notes |
|--------|------|-------|
| `ledger_term_id` | `${INTEGER}` PK | |
| `ledger_id` | `${INTEGER}` NOT NULL | |
| `effective_on` | `${DATE}` NOT NULL | |
| `credit_limit_cents` | `${INTEGER_BIG}` NULL | |
| `od_limit_cents` | `${INTEGER_BIG}` NULL | |
| `apr_purchase_bps` | `${INTEGER}` NULL | 2699 = 26.99% |
| `apr_cash_bps` | `${INTEGER}` NULL | |
| `annual_fee_cents` | `${INTEGER_BIG}` NULL | |
| `statement_close_day` | `${INTEGER}` NULL | or sentinel in collection |
| `payment_due_offset_days` | `${INTEGER}` NULL | |
| `summary` | `${TEXT}` | |
| `collection` | `${JSON}` | fee schedules, holiday rules |
| `${COMMON_CREATE}` | | |
| | | Unique `(ledger_id, effective_on)` |

### 2.5 `contacts`

People attached to a ledger with roles (customer rep, billing, shipping, landlord, …). Many per ledger.

| Column | Type | Notes |
|--------|------|-------|
| `contact_id` | `${INTEGER}` PK | |
| `ledger_id` | `${INTEGER}` NOT NULL | |
| `status_aNN` | `${INTEGER}` | |
| `role_aNN` | `${INTEGER}` | billing / shipping / primary / other |
| `name` | `${TEXT}` NOT NULL | |
| `email` | `${TEXT}` NULL | |
| `phone` | `${TEXT}` NULL | |
| `summary` | `${TEXT_BIG}` | |
| `collection` | `${JSON}` | address, etc. |
| `${COMMON_CREATE}` | | |

### 2.6 `currencies`

| Column | Type | Notes |
|--------|------|-------|
| `currency_code` | `${VARCHAR_20}` PK | `cad`, `usd`, … |
| `status_aNN` | `${INTEGER}` | |
| `name` | `${TEXT}` | |
| `minor_units` | `${INTEGER}` | 2 for CAD/USD |
| `summary` | `${TEXT}` | |
| `collection` | `${JSON}` | |
| `${COMMON_CREATE}` | | |

Seed at least `cad`, `usd`.

### 2.7 `rates`

Keyed by **pair + source + date** (decision 26).

| Column | Type | Notes |
|--------|------|-------|
| `rate_id` | `${INTEGER}` PK | |
| `base_currency` | `${VARCHAR_20}` NOT NULL | |
| `quote_currency` | `${VARCHAR_20}` NOT NULL | |
| `source_aNN` | `${INTEGER}` NOT NULL | boc / paypal / vendor / bank / implied / manual |
| `as_of` | `${DATE}` NOT NULL | |
| `rate_n` | `${INTEGER_BIG}` NOT NULL | Numerator in fixed scale |
| `rate_d` | `${INTEGER_BIG}` NOT NULL | Denominator (e.g. 1_000_000) — integer-only FX |
| `ledger_id` | `${INTEGER}` NULL | Set when source is implied from a txn |
| `txn_id` | `${INTEGER}` NULL | Implied-rate provenance |
| `summary` | `${TEXT}` | |
| `collection` | `${JSON}` | raw BoC payload snippet |
| `${COMMON_CREATE}` | | |
| | | Unique `(base, quote, source, as_of)` (implied rows may include txn_id in uniqueness via collection or partial unique — document in migration) |

### 2.8 `tax_codes` + `tax_rates`

| `tax_codes` | Type | Notes |
|-------------|------|-------|
| `tax_code_id` | `${INTEGER}` PK | |
| `organization_id` | `${INTEGER}` NOT NULL | |
| `status_aNN` | `${INTEGER}` | |
| `code` | `${VARCHAR_50}` | e.g. `GST`, `PST-BC`, `EXEMPT` |
| `name` | `${TEXT}` | |
| `target_ledger_id` | `${INTEGER}` NOT NULL | Tax payable/receivable / expense ledger |
| `summary` | `${TEXT_BIG}` | |
| `collection` | `${JSON}` | |
| `${COMMON_CREATE}` | | |

| `tax_rates` | Type | Notes |
|-------------|------|-------|
| `tax_rate_id` | `${INTEGER}` PK | |
| `tax_code_id` | `${INTEGER}` NOT NULL | |
| `effective_on` | `${DATE}` NOT NULL | |
| `rate_bps` | `${INTEGER}` NOT NULL | 500 = 5.00% |
| `summary` | `${TEXT}` | |
| `collection` | `${JSON}` | |
| `${COMMON_CREATE}` | | |

### 2.9 `transactions` (header)

| Column | Type | Notes |
|--------|------|-------|
| `txn_id` | `${INTEGER}` PK | |
| `organization_id` | `${INTEGER}` NOT NULL | Primary org label; multi-org allowed via lines |
| `status_aNN` | `${INTEGER}` | **Reserved / Reviewable / Recorded / Reconciled / Rescinded** |
| `kind_aNN` | `${INTEGER}` | transfer, purchase, payment, fee, interest, opening, **statement**, **period_close**, invoice_record, adjustment, other |
| `txn_on` | `${DATE}` NOT NULL | |
| `description` | `${TEXT}` NOT NULL | |
| `memo` | `${TEXT}` NULL | Short memo on the header |
| `source_aNN` | `${INTEGER}` NULL | manual / folly / plaid_future / qbo_future |
| `external_id` | `${VARCHAR_128}` NULL | Future feed idempotency |
| `calendar_state_aNN` | `${INTEGER}` | **not_set / pending / set / failed** |
| `calendar_event_id` | `${VARCHAR_100}` NULL | |
| `calendar_error` | `${TEXT}` NULL | Last error |
| `calendar_attempts` | `${INTEGER}` NOT NULL DEFAULT 0 | |
| `calendar_synced_at` | `${TIMESTAMP_TZ}` NULL | |
| `schedule_id` | `${INTEGER}` NULL | If generated from a schedule |
| `replaces_txn_id` | `${INTEGER}` NULL | Actual replacing a Reserved |
| `summary` | `${TEXT_BIG}` | |
| `collection` | `${JSON}` | invoice #, due date, confirm tokens, etc. |
| `${COMMON_CREATE}` | | |

Zero-amount **statement** and **period_close** kinds carry attachments (PDF / generated report) and participate in reconciliation / close warn logic (decisions 6, 27).

### 2.10 `lines`

| Column | Type | Notes |
|--------|------|-------|
| `line_id` | `${INTEGER}` PK | |
| `txn_id` | `${INTEGER}` NOT NULL | |
| `line_seq` | `${INTEGER}` NOT NULL | Order within txn |
| `ledger_id` | `${INTEGER}` NOT NULL | Must be posting ledger |
| `amount_cents` | `${INTEGER_BIG}` NOT NULL | In **that ledger’s** currency |
| `tax_code_id` | `${INTEGER}` NULL | |
| `tax_cents` | `${INTEGER_BIG}` NULL | Explicit tax portion if split onto this line / companion |
| `tax_manual` | `${INTEGER_SMALL}` | 1 = user overrode computed tax |
| `cleared` | `${INTEGER_SMALL}` | Ticked in a reconciliation |
| `reconciliation_id` | `${INTEGER}` NULL | |
| `statement_txn_id` | `${INTEGER}` NULL | Statement txn this line cleared against |
| `memo` | `${TEXT}` NULL | Short per-line memo |
| `collection` | `${JSON}` | |
| `${COMMON_CREATE}` | | |
| | | Unique `(txn_id, line_seq)` |

Tax split may add extra lines to `target_ledger_id` (decision 12); tools own that composition.

### 2.11 `reconciliations`

| Column | Type | Notes |
|--------|------|-------|
| `reconciliation_id` | `${INTEGER}` PK | |
| `ledger_id` | `${INTEGER}` NOT NULL | |
| `statement_txn_id` | `${INTEGER}` NOT NULL | Zero-amount statement transaction |
| `reconciled_on` | `${DATE}` NOT NULL | Usually statement date |
| `statement_balance_cents` | `${INTEGER_BIG}` NOT NULL | |
| `book_balance_cents` | `${INTEGER_BIG}` NOT NULL | |
| `override_reason` | `${TEXT}` NULL | Required if balances differ and override used |
| `adjustment_txn_id` | `${INTEGER}` NULL | Optional adjustment txn |
| `status_aNN` | `${INTEGER}` | open / completed / voided |
| `summary` | `${TEXT_BIG}` | |
| `collection` | `${JSON}` | |
| `${COMMON_CREATE}` | | |

Completing a reconciliation sets cleared flags on lines, bumps `ledgers.latest_reconciliation_id` / `latest_reconciled_on`, and may move txn headers to Reconciled when all their lines for that ledger are cleared (Lua rule — exact policy in Phase 3).

### 2.12 `schedules`

| Column | Type | Notes |
|--------|------|-------|
| `schedule_id` | `${INTEGER}` PK | |
| `organization_id` | `${INTEGER}` NOT NULL | |
| `status_aNN` | `${INTEGER}` | active / paused / ended |
| `name` | `${TEXT}` | |
| `from_ledger_id` | `${INTEGER}` NOT NULL | |
| `to_ledger_id` | `${INTEGER}` NOT NULL | |
| `amount_cents` | `${INTEGER_BIG}` NOT NULL | |
| `currency` | `${VARCHAR_20}` NOT NULL | Must match both ledgers’ currency or tools split/FX |
| `tax_code_id` | `${INTEGER}` NULL | |
| `rrule` | `${TEXT}` NOT NULL | iCal RRULE or Argent JSON subset |
| `anchor_on` | `${DATE}` NOT NULL | |
| `end_on` | `${DATE}` NULL | |
| `estimate_flag` | `${INTEGER_SMALL}` | 1 = estimate; actuals replace |
| `horizon_mode_aNN` | `${INTEGER}` | through_fye / fixed_days / manual |
| `summary` | `${TEXT_BIG}` | |
| `collection` | `${JSON}` | |
| `${COMMON_CREATE}` | | |

### 2.13 `tags` + `tag_links` (polymorphic)

| `tags` | Type | Notes |
|------|------|-------|
| `tag_id` | `${INTEGER}` PK | |
| `organization_id` | `${INTEGER}` NULL | Null = global |
| `name` | `${TEXT}` NOT NULL | `chequing`, `savings`, `media`, … |
| `summary` | `${TEXT}` | |
| `collection` | `${JSON}` | |
| `${COMMON_CREATE}` | | |

| `tag_links` | Type | Notes |
|-------------|------|-------|
| `tag_link_id` | `${INTEGER}` PK | |
| `tag_id` | `${INTEGER}` NOT NULL | |
| `entity_type_aNN` | `${INTEGER}` NOT NULL | Argent entity-type lookup |
| `entity_id` | `${INTEGER}` NOT NULL | |
| `${COMMON_CREATE}` | | |
| | | Unique `(tag_id, entity_type, entity_id)` |

### 2.14 `attachments` (Argent-native; files **or** plain notes)

Argent-native `attachments` table (mirrors Acuranzo `documents` pattern — revisioned rows, optional bytes — **not** a cross-schema link to `acuranzo.documents`). Decision 33: **no separate memos/notes table**. Liberal notes are text attachments; an attachment may be a file, a plain note, or both (name + body text, with or without `file_data`). Polymorphic `entity_type` + `entity_id` links to any Argent entity (txn, ledger, recon, schedule, org, contact, tag, …). Multiple attachments per transaction (decision 7). Short memos live on `transactions.memo` and `lines.memo` instead.

| Why | Detail |
|-----|--------|
| Productization | Decision 21: keep blobs/notes in-schema |
| SQLite safety | Avoids colliding with Acuranzo `documents` / `notes` |
| Decision 7 + 33 | DB-stored attachments; notes are text attachments |
| Decision 31 | Never put CalDAV secrets in attachment/collection JSON |

| Column | Type | Notes |
|--------|------|-------|
| `attachment_id` | `${INTEGER}` NOT NULL | |
| `rev_id` | `${INTEGER}` NOT NULL | PK `(attachment_id, rev_id)` |
| `txn_id` | `${INTEGER}` NULL | Convenience when entity is a transaction |
| `entity_type_aNN` | `${INTEGER}` NOT NULL | Polymorphic target (decision 33) |
| `entity_id` | `${INTEGER}` NOT NULL | |
| `att_status_aNN` | `${INTEGER}` | |
| `att_type_aNN` | `${INTEGER}` | **note** / pdf / image / report / other |
| `mime_type` | `${VARCHAR_100}` NULL | Null or `text/plain` for notes |
| `file_name` | `${TEXT}` NULL | Null for pure notes |
| `file_data` | `${TEXT_BIG}` NULL | Bytes (base64); **null for plain notes** |
| `file_text` | `${TEXT_BIG}` NULL | Note body and/or extracted PDF text (searchable) |
| `byte_len` | `${INTEGER}` NULL | 0/null for notes |
| `name` | `${TEXT}` NOT NULL | Title |
| `summary` | `${TEXT_BIG}` | Optional short blurb |
| `collection` | `${JSON}` | |
| `${COMMON_CREATE}` | | |

### 2.15 `permissions` (stub — Phase later)

| Column | Type | Notes |
|--------|------|-------|
| `permission_id` | `${INTEGER}` PK | |
| `account_id` | `${INTEGER}` NOT NULL | Acuranzo login id |
| `ledger_id` | `${INTEGER}` NOT NULL | |
| `can_read` | `${INTEGER_SMALL}` | |
| `can_write` | `${INTEGER_SMALL}` | |
| `${COMMON_CREATE}` | | |

MVP: table exists; tools ignore and grant full r/w to Andrew/Folly.

### 2.16 Parent roll-up **views** (not tables)

- `v_ledger_balance` — posting ledger balance as-of date (sum of lines on **Recorded/Reconciled** only; **Reserved and Rescinded excluded by default** — decision 32).
- `v_ledger_rollup` — parent ledger in **parent’s fixed reporting currency**, converting children via `rates` (default source BoC) at as-of; exposes `rate_id` / rate used per child.
- Implementation: `${engine}`-specific view DDL in migrations (PG/YB primary); SQLite views for tests. Document rate fallback (missing rate → null balance + warning column).

### 2.17 Lookups to seed (Argent-local ids; illustrative)

Ledger type, ledger status, txn status (5 states), txn kind, calendar state (4), rate source, tax code status, schedule status/horizon, entity type (org, ledger, txn, line, recon, schedule, attachment, contact, tag, …), contact role, attachment type/status (includes note), permission unused flags, etc.

---

## 3. Business rules (Lua)

All live in Argent `scripts` (MCP tools and internal modules). Prefer pure functions + `H.query_sync` / `H.altquery_sync`.

| Rule | Behaviour |
|------|-----------|
| **Balance per currency** | On every write of lines: group by ledger currency; each group Σ `amount_cents` must be 0. Reject otherwise (unless `confirm` on an explicit imbalance tool — default reject). |
| **Implied rate capture** | Mixed-currency txn: derive rate from line totals per currency; upsert `rates` with `source=implied`, link `txn_id`. |
| **BoC lookup/cache** | Tool fetches BoC rate for pair+date if missing; cache row `source=boc`. Failures soft-fail for views; hard-fail only if caller required BoC. |
| **Tax split** | Given `tax_code_id` + net/gross flag: compute tax from dated `tax_rates`; post companion line(s) to `target_ledger_id`. Manual `tax_cents` allowed; if drift vs computed > threshold (e.g. 1 cent or configurable bps), return **warning** requiring `confirm`. |
| **Warn-and-confirm** | Edits/deletes touching Reconciled txns, or `txn_on` ≤ `ledgers.latest_reconciled_on`, or period_close boundaries → response `{ needs_confirm, warning, confirm_token }`. Retry with `confirm=token` applies change and **knocks status back to Recorded** (and clears recon links as needed). Same for edits before a period_close statement txn. |
| **Schedule generation** | Expand RRULE through horizon (default **through org FYE**); create Reserved txns; skip dates that already have a matching Reserved/actual. |
| **Reserved → actual** | Posting an actual with `replaces_txn_id` or match key (schedule_id + date + amount window): mark Reserved Rescinded or superseded; estimates replaced by actuals. |
| **CalDAV sync** | After successful save, enqueue calendar_state=pending; async Lua/orchestrator pushes create/update using **env only** `ARGENT_CAL_USER`, `ARGENT_CAL_PASS`, `ARGENT_CAL_HTTP` (base URL) — **never stored in the DB** (decision 31). Never fails the save. Retries increment `calendar_attempts`; Failed after N with `calendar_error`. `Argent.RetryCalendar` + sync-problems query. |
| **Report inclusion** | Balances and most reports **exclude Reserved and Rescinded** by default. Forecast/projected tools (`QueryDue`, etc.) include Reserved only when `include_reserved=true` (decision 32). |
| **Parent roll-up** | Read-only views/tools; conversion uses selected rate source (default BoC); never posts FX. |
| **Opening** | Creating a ledger posts opening balance txn (kind=opening) on `opening_on`; moving opening earlier is a deliberate tool that may insert history. |
| **Idempotency** | Write tools accept `idempotency_key` stored in `collection` / unique external_id where applicable. |

**Confirm flow (MCP):**

1. Client calls write tool.  
2. If safe → apply; return entity.  
3. If guarded → HTTP/MCP result with `needs_confirm=true`, human-readable `warning`, short-lived `confirm_token` (HMAC or row in collection / temp table). **No mutation.**  
4. Client recalls same tool with `confirm=<token>` (and same payload hash) → apply + knock-back rules.  
5. Tokens expire (~15 minutes); mismatch → error.

---

## 4. MCP toolset (`Argent.*`)

Register in Argent `scripts`: `mcp_access=1`, `invokable=0`, annotations per tool. Actor from `params._hydrogen` / JWT → `created_id`.

### Read (`readOnlyHint` + `idempotentHint` true)

| Tool | Params | Returns |
|------|--------|---------|
| `Argent.ListOrganizations` | — | orgs |
| `Argent.ListLedgers` | `organization_id?`, `type?`, `tag?`, `include_non_posting?` | ledger summaries + balances optional |
| `Argent.GetLedger` | `ledger_id`, `as_of?` | ledger, terms, contacts, latest recon, balance |
| `Argent.GetTransaction` | `txn_id` | header, lines, tags, attachment meta, calendar state |
| `Argent.ListTransactions` | filters: ledger, org, date from/to, status, kind | headers |
| `Argent.QueryBalances` | `as_of`, `organization_id?`, `rate_source?`, `include_reserved?` (default false) | posting + rollup; excludes Reserved/Rescinded unless flagged |
| `Argent.QueryDue` | `from`, `to`, `organization_id?`, `include_reserved?` (default **true** — forecast) | schedule projections + Reserved when flagged |
| `Argent.QueryReconciliationStatus` | `ledger_id?` | last recon, uncleared count |
| `Argent.QueryLedgerHistory` | `ledger_id`, `from`, `to`, `include_reserved?` | lines + running balance; default excludes Reserved/Rescinded |
| `Argent.QueryTaxSummary` | `organization_id`, `from`, `to` | per tax_code |
| `Argent.QueryIncomeExpense` | `organization_id`, `from`, `to`, `rate_source?`, `include_reserved?` | totals; default excludes Reserved/Rescinded |
| `Argent.QueryCalendarView` | `from`, `to`, `ledger_id?` | txn calendar fields |
| `Argent.QueryFxPremium` | `from`, `to`, `pair`, `compare_source` | vs BoC |
| `Argent.QuerySyncProblems` | — | txns calendar_state=failed/pending aged |
| `Argent.Search` | `q`, `types[]?` | attachment note/file_text, descriptions, names |
| `Argent.ListRates` | pair, source, from/to | rates |
| `Argent.GetBocRate` | pair, `as_of` | rate (fetch+cache) |

### Write (`readOnlyHint` false; `destructiveHint` true where knock-back / delete / rescind)

| Tool | Params (core) | Notes |
|------|----------------|-------|
| `Argent.UpsertOrganization` | name, FY fields, currency | |
| `Argent.UpsertLedger` | org, type, currency, opening_*, parent_id, calendar_*, mask… | Creates opening txn |
| `Argent.UpsertLedgerTerms` | ledger_id, effective_on, terms… | |
| `Argent.UpsertContact` | ledger_id, role, name, … | |
| `Argent.PostTransaction` | date, description, lines[], kind, tax opts, `confirm?`, `idempotency_key?` | Balancing + tax + warn path |
| `Argent.EditTransaction` | txn_id, patch, `confirm?` | Warn/knock-back |
| `Argent.RescindTransaction` | txn_id, reason, `confirm?` | → Rescinded |
| `Argent.PostStatement` | ledger_id, statement_on, balance, attachment?, `confirm?` | kind=statement, 0 amount |
| `Argent.PostPeriodClose` | organization_id, close_on, report attachment?, `confirm?` | kind=period_close |
| `Argent.StartReconciliation` | ledger_id, statement_txn_id, … | |
| `Argent.ClearLines` | reconciliation_id, line_ids[] | |
| `Argent.CompleteReconciliation` | reconciliation_id, override_reason?, adjustment?, `confirm?` | |
| `Argent.UpsertSchedule` | schedule fields | |
| `Argent.GenerateSchedule` | schedule_id?, org_id?, horizon? | Creates Reserved |
| `Argent.MatchReserved` | actual payload + replaces/match | |
| `Argent.AddTags` / `Argent.RemoveTags` | entity, tag names | |
| `Argent.AddAttachment` | entity_type, entity_id, name, `att_type` (note|pdf|…), `file_text?` (note body), file_name/mime/data? | File and/or plain note (decision 33); replaces AddMemo/AttachFile | |
| `Argent.RetryCalendar` | txn_id? or all failed | |
| `Argent.UpsertRate` | pair, source, as_of, rate | Manual |
| `Argent.UpsertTaxCode` / `Argent.UpsertTaxRate` | … | |

**Tool count (planned):** ~16 read + ~21 write ≈ **37** tools (may coalesce Upserts). Full r/w for Folly (decision 10).

---

## 5. Report queries (data only)

Exposed as read MCP tools and/or QueryRefs in `queries` for a future reporting tool. Return JSON rows only — no PDF/HTML in Argent MVP.

**Default filter (decision 32):** balances and most reports exclude **Reserved** and **Rescinded**. Due/projected (forecast) reports include Reserved when `include_reserved` is set (default true on `QueryDue` only).

| Report | Grain | Key columns |
|--------|-------|-------------|
| Balances | ledger / as-of | ledger_id, type, currency, balance_cents, …; Recorded/Reconciled only by default |
| Due / projected | date | schedule expansions + Reserved (forecast flag), amount, ledger, estimate_flag |
| Income & expense | period × ledger/tag | totals; excludes Reserved/Rescinded by default |
| Reconciliation status | ledger | last_reconciled_on, statement_txn_id, uncleared_count, drift |
| Ledger history | line | txn_on, description, amount, running_balance, status; default excludes Reserved/Rescinded |
| Tax summary | tax_code × period | taxable_cents, tax_cents, manual_override_count |
| Calendar view | day | txns with calendar_state/event_id |
| FX premium vs BoC | source × pair × day | implied/vendor rate vs boc, premium_bps |

---

## 6. Phases and GATES

Each GATE = (a) listed Hydrogen tests green on Argent design, (b) Folly verification notes, (c) **Andrew written sign-off** in chat or decisions log. No skipping ahead with production data.

### Phase 0 — Design sign-off

- Agree this plan (open questions 1–4 closed by decisions 25/31–33; parent views, CalDAV env, Reserved exclusion, notes-as-attachments).
- Freeze table list and lookup families.
- **GATE 0:** Andrew approves `/workspace/folly/argent-plan.md` + decisions file unchanged or amended.

### Phase 1 — Schema / migrations / seeds

- Create `elements/002-helium/argent/` (README, `database*.lua` copied from Acuranzo/helium pattern, `migrations/`).
- `argent_5000` queries bootstrap; `5001` lookups; then tables in dependency order; diagram migrations; seed currencies CAD/USD; seed tax codes skeleton; seed entity-type lookups.
- Wire `DESIGNS` in `test_31_migrations.sh`, `payload-generate.sh`, and add Argent connection snippets for `test_38` / sqlite migration tests.
- **GATE 1:** `test_31` (static) + `test_34`/`test_38` (or Argent-targeted equivalents) apply clean forward+reverse on SQLite and Yugabyte; `test_71` diagrams generate; `test_98` luacheck clean; Andrew reviews ERD/README.

### Phase 2 — Core Lua + MCP CRUD + balancing

- Scripts: org/ledger/terms/contact CRUD; `PostTransaction` with per-currency balance check; tags; AddAttachment (note and/or file).
- MCP list/get tools.
- Unit-style Lua fixtures under Hydrogen scripting tests (`test_43` / `test_47` patterns with Argent DB).
- **GATE 2:** MCP `tools/list` shows `Argent.*`; create org+two ledgers+balanced txn round-trips; unbalanced txn rejected; Andrew posts one sample txn via Folly.

### Phase 3 — States, warnings, reconciliation

- State machine; statement txn; reconciliation clear/complete; warn-and-confirm + knock-back to Recorded; period_close kind.
- **GATE 3:** Automated cases: edit reconciled → needs_confirm; confirm → Recorded; recon override requires reason; Folly dry-run on a fake PC MC statement; Andrew sign-off.

### Phase 4 — Schedules + calendar sync

- Schedules + GenerateSchedule through FYE; Reserved matching; calendar_state machine; non-blocking sync worker; RetryCalendar; QuerySyncProblems.
- CalDAV against Folly’s Stalwart test calendar (or mock in harness); credentials only from `ARGENT_CAL_USER` / `ARGENT_CAL_PASS` / `ARGENT_CAL_HTTP` (never in DB).
- **GATE 4:** Generate month of Reserved rent; match actual; save txn while CalDAV down still succeeds with pending/failed; Andrew sign-off.

### Phase 5 — Rates / BoC / FX views

- rates table usage; implied-rate on mixed txn; GetBocRate cache; rollup views; QueryFxPremium.
- **GATE 5:** USD+CAD txn writes implied rate; rollup shows BoC conversion + rate_id; premium query returns rows; Andrew sign-off.

### Phase 6 — Report queries

- Implement remaining Query* tools; golden fixtures.
- **GATE 6:** Each report in §5 returns stable JSON on fixture DB (Reserved/Rescinded defaults verified); Andrew sign-off on shapes.

### Phase 7 — Production DB + seed from Folly tracker

- Provision Yugabyte schema `argent` + Hydrogen connection (separate from Acuranzo app DB as appropriate).
- Seed organizations, institutions-as-ledgers, real ledgers from `/workspace/folly/accounts.md` (balances as opening or statement snapshots — **manual Folly entry via MCP**, no bulk import pipeline).
- Dual-write period: Folly updates Argent + keeps `accounts.md` until trust.
- **GATE 7:** Production migrations applied; ~25 ledgers visible; Folly daily interview writes one statement through MCP; Andrew declares Argent source of truth for balances (markdown becomes backup).

### Later (explicitly out of current gates)

- **Permissions** enforcement on every tool.
- **Imports / Plaid** (`source` / `external_id` already on transactions).
- **QBO integration / replacement** (~2028): invoice docs, payroll exports, historical import — schema already has contact ledgers, attachments, tax summaries so this is not blocked.

---

## 7. Open questions

None remaining. Closed:
1. Parent ledgers — non-posting roll-up views (decision 25).
2. Notes — no `memos` table; liberal notes are text `attachments`; short `memo` on transactions/lines (decision 33).
3. CalDAV credentials — env vars `ARGENT_CAL_USER`, `ARGENT_CAL_PASS`, `ARGENT_CAL_HTTP` only; never in DB (decision 31).
4. Reserved/Rescinded — excluded from balances/most reports by default; forecast tools take an explicit include flag (decision 32).

`argent_5xxx` numbering and Argent-native `attachments` stand as plan defaults.

---

## 8. File / wiring checklist (implementers)

| Item | Action |
|------|--------|
| `elements/002-helium/argent/` | Create design tree |
| `elements/002-helium/README.md` | Link Argent schema |
| `docs/He/README.md` | List Argent under Schemas |
| `test_31_migrations.sh` `DESIGNS` + `DESIGN_SCHEMAS["argent"]=...` | Include argent |
| `payload-generate.sh` `DESIGNS` | Include argent |
| Hydrogen prod/test configs | Connection Name e.g. `Argent`, Schema `argent`, Migrations `PAYLOAD:argent` |
| Scripting `DefaultDatabase` / MCP | Point Folly MCP at Argent connection for `Argent.*` tools; keep Acuranzo for logins via `H.altquery` |
| `helium_update.sh` / migration_index | Regenerate argent README index |
| Hydrogen env (prod + Folly box) | Set `ARGENT_CAL_USER`, `ARGENT_CAL_PASS`, `ARGENT_CAL_HTTP` for calendar sync (decision 31) |

---

*End of Argent plan.*
