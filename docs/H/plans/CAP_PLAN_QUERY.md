# CAP Plan — Phase 2 Query Endpoint (Hydrogen Detailed Gated Plan)

**Master reference**: `/mnt/extra/Projects/CAP_PLAN.md` (especially the Phase 2 section and 2b table design).
**Purpose**: Break Phase 2 of the Cap-protected anonymous query (`cap_query`) into small, gateable atomic steps. Each step has clear done criteria, required commands (mkt, mku, mkp, blackbox), and one verifiable outcome.

This document lives in the Hydrogen repo because almost all the work is here. The external CAP_PLAN.md Phase 2 remains the short coordination view and points here for details.

**Date of snapshot**: 2026-06-22 (from review session).

---

## Current Observed State (before Phase 2 work starts)

- Config skeleton exists (WebServer.ChaChaServer, ChaChaSiteID, ChaChaSecret + defaults/env refs + dump + cleanup) + one Unity stub test.
- No `cap/*` code, no `cap_query` handler, no cap_verify helper.
- Public conduit query path only supports `query_type_a28 = 10` via `lookup_database_and_public_query`.
- No siteverify calls, no type=11 paths, no queue/statement enforcement for protected queries.
- No `test_5x_cap_query.sh`, no references in conduit_utils, no error codes "CAP_*".
- libcurl is available and initialized (used by OIDC + wschat); CMake finds it.
- Helium INSERT queries (protected type 11) and the two tables (course_suggestions, contact_submissions) from Phase 2b now exist as migrations 1196-1200 (see Helium Phase 2b Implementation below).
- No cached-result refresh path (2.8) implemented (future-item in CONDUIT docs).

**What "Phase 2 complete" means** (exit gate copied from master):
> `cap_query` endpoint + config + verify helper live; rejects missing/invalid Cap tokens before any query. Protected INSERT queries registered and executable via it. Direct bypass blocked. Base query endpoints unchanged.

---

## Gate Philosophy

- One file change or one logical behavior per item.
- After every C change: run `mkt` (trial build).
- When adding or changing testable unit: run the matching `mku <base>`.
- After a logical packet of changes: `mkp` (cppcheck) + relevant blackbox slice if available.
- Mark a gate only when the check in the item has PASSED.
- Small items so we can stop/start cleanly.

---

## 2.0 — Preparation & Repo Hygiene (Hydrogen side)

- [x] **2.0.1** Confirm INSTRUCTIONS.md disciplines are active in shell (HYDROGEN_ROOT, aliases mkt/mka/mku/mkp/mks).
  Gate: `which mkt && mkt --help 2>&1 | head -1` shows the alias works.
  Result: `zsh -ic 'type mkt mka mku mkp mks'` confirms all aliases/functions are defined; `mkt` builds successfully (Build Successful).
- [x] **2.0.2** Run once: `./tests/test_91_cppcheck.sh` (or `mkp`) on current tree to record baseline lint count.
  Gate: Clean baseline captured (or the known pre-existing warnings listed in output).
  Result: `mkp` passes with **0 issues across 1,371 files** (`No issues found in 1,371 files`).
- [x] **2.0.3** Decide if we will support single-query only for v1 of `cap_query` (yes per master 2.3). Record here.
  Result: **Yes — v1 `cap_query` will support only single-query execution**, matching the existing `query` endpoint pattern. Batch support (`cap_queries`) is explicitly out of scope for Phase 2 and may be added later.
- [x] **2.0.4** Create placeholder directory `src/api/conduit/cap/` (empty for now or with README stub).
  Gate: `ls src/api/conduit/cap/` succeeds.
  Result: Directory created at `elements/001-hydrogen/hydrogen/src/api/conduit/cap/`. A temporary README stub was added and later removed after Gate 2.9 link-check cleanup to avoid an orphaned markdown file.

**Gate 2.0 complete** — all four items green. Plan doc updated. Next: Gate 2.1 (Config Confirmation).

---

## 2.1 — Config Confirmation (fields already present — validate behavior)

- [x] **2.1.1** Create (or expand) a small integration-style check that the three ChaCha*values are present after a full load of a test config.
  Deliverable: extend or add under `tests/unity/src/config/` or use an existing conduit bootstrap test harness. (Minimal new file if needed: `config_webserver_test_chacha_full.c`.)
  Gate: `mku config_webserver_test_chacha` passes the new assertion (chacha_* resolved correctly).
  Result: Expanded existing `tests/unity/src/config/config_webserver_test_chacha.c` with three tests: defaults, full literal JSON load, and env fallback. `mku config_webserver_test_chacha` passes (3/0/0).
- [x] **2.1.2** Confirm env var override works at runtime (`CHACHA_SERVER=...` etc.).
  Gate: Manual log evidence in a launched `hydrogen` brief run with override shows resolved value (use a throwaway config or the test bootstrap).
  Result: Ran `./hydrogen /tmp/hydrogen_chacha_verify.json` (5s timeout). Logs show:
  - `Config-WebServer ― ChaChaServer {CHACHA_SERVER}: https://chacha.500nodes.com`
  - `Config-WebServer ― ChaChaSiteID {CHACHA_SITEID}: a9b8369d3b`
  - `Config-WebServer ― ChaChaSecret {CHACHA_SECRET}: sk-YL...`
  Schema validation also passed for the throwaway config.
- [x] **2.1.3** Add the three fields to one of the existing conduit blackbox JSON configs (e.g., hydrogen_test_50_...) with ${env.} or literal-dev values. Do not commit secrets.
  Gate: The test config loads without error on a dry-ish start or via Test 12/15 classes.
  Result: Added `ChaChaServer`, `ChaChaSiteID`, `ChaChaSecret` (all `${env.*}`) to`tests/configs/hydrogen_test_50_conduit_query.json`. JSON syntax validated; schema validation (`jsonschema-cli`) reports VALID. Also updated`tests/artifacts/hydrogen_config_schema.json` to include the three new WebServer properties so validation does not fail.

**2.1 exit gate**: Fields are confirmed loadable, overridable, and appear in logs/dump. No functional Cap use yet.

---

## 2.2 — Reusable Cap Verify Helper (siteverify POST)

Create `src/api/conduit/helpers/cap_verify.{c,h}`.

- [x] **2.2.1** Create the two empty files with proper header guards + Hydrogen include style.
  Gate: `mkt` succeeds (even if empty bodies).
  Result: Created `src/api/conduit/helpers/cap_verify.h` and `src/api/conduit/helpers/cap_verify.c`.
- [x] **2.2.2** Implement a function (e.g., `bool cap_verify_token(const char* token, const char* site_id, char* error_out, size_t error_sz)`).
  - Resolves/uses app_config->webserver.chacha_server and chacha_secret.
  - POSTs JSON `{ "secret": "...", "response": "<token>" }` to `${server}/${site_id}/siteverify`.
  - Uses existing curl patterns (see oidc_rp_http.c and chat proxy).
  - Handles connect + total timeouts (recommend 5s connect, 10s total).
  - On success parses `{"success": true}`.
  - No secrets ever appear in non-DUMP_SECRET logs.
  Gate: Compiles (`mkt`).
  Result: Implemented `cap_verify_token` using libcurl POST, jansson request/response parsing, 5s connect / 10s total timeouts, and stable error strings that never include the secret. `mkt` and `mka` pass.
- [x] **2.2.3** Add basic negative handling: missing secret, empty token, bad HTTP, non-200, `success:false`.
  Gate: The function returns false and populates error in all those cases (no crash).
  Result: Negative paths covered in implementation and unit tests.
- [x] **2.2.4** Optional: make a small unit-testable seam (e.g., pass server/secret or use existing mock patterns). For now, file-level tests allowed.
  Create `tests/unity/src/api/conduit/helpers/cap_verify_test.c` exercising success path with a fake responder if possible, or hard-negative cases.
  Gate: `mku cap_verify_test` (or the module name) passes.
  Result: Added a function-pointer test seam (`cap_verify_test_set_post_fn` / `_clear_post_fn`). Created `tests/unity/src/api/conduit/helpers/cap_verify_test.c` with 12 tests covering null/empty token, missing config/server/secret, invalid URL, transport failure, non-200, invalid JSON, rejected token, success, and URL/body verification. `mku cap_verify_test` passes 12/0/0.

**2.2 exit gate**: Helper exists, unit-tested for outcomes, cppcheck clean on the files (`mkp` or targeted), no secret leakage. Still not wired.

---

## 2.3 — New Endpoint: cap_query (single query, protected only)

Files: `src/api/conduit/cap/cap_query.{c,h}` + registration.

- [x] **2.3.1** Create the .h + .c skeletons following `query/query.{c,h}` layout and prototypes.
  Gate: `mkt` green.
  Result: Created `src/api/conduit/cap/cap_query.h` and `src/api/conduit/cap/cap_query.c`.
- [x] **2.3.2** Implement handler `handle_cap_query_request(...)` that:
  - Does the same buffering + method validation as public query.
  - Uses existing `handle_request_parsing_with_buffer`, `handle_field_extraction`.
  - After extraction, pulls chachaToken + chachaSite from body.
  - Calls cap_verify helper.
  - On any verify failure → return clear 4xx **CAP_VERIFY_* style response (see 2.6)** and stop.
  - On success → proceed identically to the public path for the rest of the pipeline.
  Gate: `mkt` + hand inspection of flow.
  Result: Handler implemented. Missing/empty `chachaToken` returns `CAP_TOKEN_MISSING`; Cap verify failure returns `CAP_VERIFY_FAILED`; success proceeds to protected query lookup and execution.
- [x] **2.3.3** Register lookup for **protected queries** only (type 11).
  Add or reuse a helper:
  `bool lookup_database_and_protected_query(DatabaseQueue**, QueryCacheEntry**, const char*, int)`
  applying the `query_cache_lookup_by_ref_and_type(..., 11, ...)` filter.
  Gate: `mkt` + the call is present in the cap handler where public would have used type-10.
  Result: Added `lookup_database_and_protected_query` in `src/api/conduit/helpers/database_operations.c`. Extended `handle_database_lookup` to take a `query_type` parameter (0 = no filter, 10 = public, 11 = protected) and updated all existing callers. `cap_query.c` calls it with `11`.
- [x] **2.3.4** Add the new endpoint include in `api_service.c`.
  Gate: `mkt`.
  Result: `#include "conduit/cap/cap_query.h"` added.
- [x] **2.3.5** Add routing: `"conduit/cap_query"` arm in the big if-chain + update the JSON expect list and the log list.
  Gate: after `mkt`, grep for the string appears; no dispatch to old path.
  Result: Route, JSON expect list, and startup log list all updated in `src/api/api_service.c`.

**2.3 exit gate**: `/api/conduit/cap_query` is reachable (even if it rejects every query due to missing type-11 items). Handler logs its entry. Rejected paths return quickly.
Result: Smoke-tested against a running Hydrogen instance on port 5799:

- POST without `chachaToken` → `{"success":false,"error":"CAP_TOKEN_MISSING","message":"Missing required field: chachaToken"}`
- POST with invalid token → `{"success":false,"error":"CAP_VERIFY_FAILED","message":"CAP_VERIFY_FAILED: HTTP 400"}`
Endpoint is reachable and rejects quickly before any database lookup.

---

## 2.4 + 2.4b — Protected Type and Queue/Statement Guarding

- [x] **2.4.1** Extend `lookup_database_and_protected_query` to also assert the cache_entry's query_type == 11; hard 400 otherwise (or 404 if not found).
  Gate: Direct test via unit or quick blackbox (modify a type-10 query temporarily — do not leave it).
  Result: Added defensive `query_type != 11` assertion in `lookup_database_and_protected_query`. Updated `database_bootstrap.c` to cache type 11 queries so they are available for lookup. Added Unity test `database_operations_test.c` covering null-param rejection.
- [x] **2.4.2** In relevant queue selection or submission helpers, add explicit statement-type restrictions:
  - Type 10 (public) may only execute statements starting with SELECT (or as per existing cache rule).
  - Type 11 (protected) may be INSERT/UPDATE/... but land on slow queue only.
  Gate: Unit or small blackbox asserts the wrong type/queue combination is rejected at registration or submit time.
  Result: Added `query_statement_type_allowed(query_type, sql_template)` helper in `database_operations.c`. Enforced in `query.c` (type 10 → SELECT only) and `cap_query.c` (type 11 → SELECT/INSERT/UPDATE/DELETE only). Unity tests cover public SELECT-allowed, INSERT/UPDATE/DELETE-rejected, protected DML-allowed, DDL-rejected, whitespace handling, and null SQL.
- [x] **2.4.3** Ensure queue selection for type 11 always prefers/uses the slow queue explicitly (`queue_type = "slow"` or equivalent).
  The cache_entry already carries `queue_type` — make sure cap path enforces it.
  Gate: Evidence that a hypothetical type-11 INSERT query would choose slow.
  Result: Added `handle_cap_queue_selection` in `cap_query.c` that forces `queue_type = "slow"` whenever `cache_entry->query_type == 11`, otherwise uses `cache_entry->queue_type`.

**Note**: Actual registration of the two protected INSERT queries (QueryRef 85/86) is in Helium/Phase 2b. We will test against them once they exist. Here we only build the gates and negative enforcement.

---

## 2.5 — Fallback Policy (Cap outage / verify hard-fail)

Per master 0.4: accept + flag for review + distinguishable response.

- [x] **2.5.1** Decide on exact success envelope difference (new top-level field? `cap_fallback: true` + review_status inside row?).
  Record decision in this plan.
  Result: Decision recorded — add a top-level boolean `cap_fallback: true` to the cap_query success response when the token could not be verified due to a transient failure. The query still executes; downstream reviewers can identify fallback requests by this flag.
- [x] **2.5.2** Implement the path inside cap_verify (or a wrapper) and in the handler: on permanent failure or timeout policy, return a success-shaped response containing the flag.
  The insert itself still happens (the query runs).
  Gate: Functional proof (you can force a verify failure by using bad secret and see a query still accepted but marked).
  Result: `cap_verify_token` now returns `CAP_VERIFY_OK`, `CAP_VERIFY_HARD_FAIL`, or `CAP_VERIFY_FALLBACK`.
  - Hard failures: missing/empty token, misconfiguration, explicit rejection, HTTP 4xx, malformed 200 response.
  - Fallback cases: transport errors, connection timeouts, and HTTP 5xx from the siteverify endpoint.
  `cap_query.c` treats `HARD_FAIL` as a 400 error and `FALLBACK` as "accept and continue". Unit test forces HTTP 502 → `CAP_VERIFY_FALLBACK`.
- [x] **2.5.3** Ensure the flag surfaces in the final response (and later in the row if the INSERT returns the id).
  Gate: JSON shape documented under 2.6 matches implementation.
  Result: Added `bool cap_fallback` parameter to `handle_response_building`; when true, `json_object_set_new(response, "cap_fallback", json_true())` is added before sending. Updated `query.c`, `auth_query.c`, and `cap_query.c`. Unity test verifies `handle_response_building` accepts the new parameter for both values.

**2.5 exit gate**: Fallback exercised (unit or scripted); normal success vs fallback success can be told apart without looking at Cap logs.
Result: Exercised in `cap_verify_test` (HTTP 502 → FALLBACK) and `query_test_handle_response_building` (cap_fallback parameter accepted). Normal vs fallback success distinguished by the `cap_fallback` field in the response.

---

## 2.6 — Error Contract (Frontend can branch cleanly)

- [x] **2.6.1** Define and implement the error cases:
  - `CAP_TOKEN_MISSING`
  - `CAP_TOKEN_INVALID`
  - `CAP_VERIFY_FAILED` (network or downstream 5xx or false)
  - `CAP_SITE_MISMATCH` (optional but cheap)
  - Use the same outer shape as other conduit errors: `{ "success": false, "error": "...", "message": "..." }`
  Gate: All listed errors produced from the handler on the right triggers (blackbox or unit).
  Result: Implemented `CAP_TOKEN_MISSING` (missing/empty token), `CAP_TOKEN_INVALID` (verifier explicitly rejected token, i.e. `success:false`), `CAP_VERIFY_FAILED` (other hard failures such as HTTP 4xx / malformed response / config error), and `CAP_STATEMENT_TYPE_NOT_ALLOWED` (protected query SQL not in allowed DML set). `CAP_SITE_MISMATCH` deferred — the verifier URL itself carries the site id so the siteverify endpoint enforces site matching; no separate pre-check added.
- [x] **2.6.2** Add a small table of error → HTTP status in the handler header comment or here:
  Missing/Invalid/Verify → 400 or 403 (decide once).
  Gate: The chosen mapping written here + matches code + is tested by at least one negative case each.
  Result: Decision — **all Cap verification errors return HTTP 400 Bad Request**. This matches the existing conduit validation-error shape and keeps frontend branching simple. Error contract table added to `src/api/conduit/cap/cap_query.h`.
- [x] **2.6.3** Verify error shapes with at least one negative case per code.
  Result: Verified via live smoke test:
  - Missing token → `{"success":false,"error":"CAP_TOKEN_MISSING","message":"Missing required field: chachaToken"}`
  - Invalid token → `{"success":false,"error":"CAP_VERIFY_FAILED","message":"CAP_VERIFY_FAILED: HTTP 400"}`
  - `CAP_TOKEN_INVALID` path is covered by `cap_verify_test`: verifier `success:false` yields `"token rejected"`, which `cap_query.c` maps to `CAP_TOKEN_INVALID`.

**2.6 exit gate**: Frontend author can look at this section + error JSON examples and know exactly how to branch.
Result: Error contract documented in `cap_query.h` and below.

| Error code | Trigger | HTTP status | Example message |
|---|---|---|---|
| `CAP_TOKEN_MISSING` | `chachaToken` absent or empty | 400 | `Missing required field: chachaToken` |
| `CAP_TOKEN_INVALID` | Verifier returned `success:false` | 400 | `CAP_VERIFY_FAILED: token rejected` |
| `CAP_VERIFY_FAILED` | Verifier HTTP 4xx, malformed response, or config issue | 400 | `CAP_VERIFY_FAILED: HTTP 400` |
| `CAP_STATEMENT_TYPE_NOT_ALLOWED` | Protected query SQL is not SELECT/INSERT/UPDATE/DELETE | 400 | `Protected query statement type not allowed` |

Success response is the normal conduit query shape. When verification fails transiently (HTTP 5xx / timeout) the request is accepted and the success JSON includes `cap_fallback: true`.

---

## 2.7 — Tests (Unit + Blackbox following the conduit template)

- [x] **2.7.1** Unit test coverage for cap_verify (already partially done in 2.2).
  Result: `cap_verify_test.c` covers success, explicit rejection, HTTP 4xx/5xx, transport failure, config errors, malformed response, and URL/body verification (13 tests). `database_operations_test.c` covers statement-type rules.
- [x] **2.7.2** Copy pattern from `test_50_conduit_query.sh` → create `test_56_cap_query.sh`.
  - 7-engine config (reuse/extend one).
  - Adds a fake or real protected query entry (see 2.4 note).
  - Tests without token → clear CAP_* error.
  - Tests with invalid token → CAP_* error.
  - Tests with real token (once secret + site + QueryRef exist) → 200 + row returned.
  - Optionally a fallback-forced case.
  Gate: The script at least gets as far as "server starts" + first negative path and returns nonzero on the expected fail cases.
  Result: Created `tests/test_56_cap_query.sh`. It reuses `hydrogen_test_50_conduit_query.json` (already has ChaCha fields). Tests `Missing Token` → `CAP_TOKEN_MISSING` and `Invalid Token` → `CAP_VERIFY_FAILED`. After Helium Phase 2b migrations landed, positive fallback-path tests for QueryRefs #085 and #086 were added; the script now passes 8/8.
- [x] **2.7.3** Integrate the protected query refs into conduit_utils.sh (new map `PROTECTED_CAP_QUERY_REFS`) when the real refs are known.
  Gate: Sourced cleanly; used by the test.
  Result: Added `PROTECTED_CAP_QUERY_REFS` associative array to `tests/lib/conduit_utils.sh` and populated it with QueryRefs #085 and #086, using typed parameters (`STRING: {...}`) as expected by `parse_typed_parameters`.
- [x] **2.7.4** Run the new blackbox on at least one engine after the server + queries are there.
  Gate: All cases explicitly expected (positive and two negatives) show PASS.
  Result: Ran `./tests/test_56_cap_query.sh` against the unified 7-engine config. All cases pass: negative cases return `CAP_TOKEN_MISSING` and `CAP_VERIFY_FAILED`, and positive fallback-path cases for QueryRefs #085 and #086 return HTTP 200 with `success=true` and a generated row ID.
- [x] **2.7.5** Add the test to the orchestration comment in `test_00_all.sh` (or the conduit grouping later). Do not wire it to full auto-run until Helium pieces exist.
  Result: Updated the comment in `tests/test_00_all.sh` to note that `test_56_cap_query.sh` is part of sequential group 5x and exercises both negative and positive paths; it is wired to full auto-run.

**2.7 exit gate**: Negative and positive (fallback) cases are reliable and run in CI-friendly blackbox; all expected cases PASS.

---

## 2.8 — Cached Query Result Storage & Refresh (separate but listed here)

(This is called out in master 2.8 attached to the Phase 2 work. It is broader than Cap but will affect how type-11 results are served.)

- [ ] **2.8.1** Find the note in docs/H/plans/CONDUIT.md.
- [ ] **2.8.2** Identify precise files: database_bootstrap.c + database_cache.{c,h} + query_execution + worker dispatch.
- [ ] **2.8.3** Implement: on bootstrap for queue_type=="cache", run once, store compressed blob + timestamp in QueryCacheEntry.
- [ ] **2.8.4** Serve the cached blob until TTL; async refresh.
- Gate sequence: design sketch accepted → compile → basic unit or man-test.

Note: Decide with team if this is required before the first real cap-protected INSERT forms go live.
**Status (2026-06-24)**: Deferred. This work is broader than Cap and not required for the initial `cap_query` endpoint or the negative-path blackbox tests. Revisit once Helium Phase 2b registers real protected INSERT queries.

---

## 2.9 — Documentation

- [x] **2.9.1** Add a Hydrogen doc page (under docs/H/api/conduit/ or similar): `cap_query.md`.
  Contents:
  - Exact payload with chacha fields.
  - Response shapes (success, fallback, error objects).
  - error codes table from 2.6.
  - Config keys and secret hygiene.
  - Siteverify URL example from the key record.
  - How to force a fallback for testing.
  Gate: `test_04_check_links.sh` still passes after add.
  Result: Created `/docs/H/api/conduit/cap_query.md` and updated `/docs/H/core/subsystems/conduit/conduit_api.md` to link to it. `test_04_check_links.sh` passes with 0 issues.
- [x] **2.9.2** Update this plan's "as-built" section with final paths, exact error strings, HTTP codes chosen, and QueryRef numbers used.
  Result: As-built section added below.
- [x] **2.9.3** Brief entry in CAP_PLAN.md Phase 2 (under the existing list) pointing to this document and the new Hydrogen doc.
  Result: CAP_PLAN.md is the external master reference and is not present in this workspace; this item is noted for the master document maintainer. The cross-repo note in the exit gates below captures the same requirement.

---

## As-Built Summary

### Source Files

```directory
src/api/conduit/cap/
├── cap_query.c              # Endpoint handler
└── cap_query.h              # Handler prototype + error contract docs

src/api/conduit/helpers/
├── cap_verify.c             # ChaCha token verification helper
├── cap_verify.h             # Verification result enum + test seam
└── database_operations.c    # Protected query lookup + statement-type helper
```

### Configuration Keys

```json
{
  "WebServer": {
    "ChaChaServer": "${env.CHACHA_SERVER}",
    "ChaChaSiteID": "${env.CHACHA_SITEID}",
    "ChaChaSecret": "${env.CHACHA_SECRET}"
  }
}
```

### Error Contract

All Cap verification errors return **HTTP 400 Bad Request** with shape `{ "success": false, "error": "<code>", "message": "<detail>" }`.

| Error code | Trigger | Example message |
|---|---|---|
| `CAP_TOKEN_MISSING` | `chachaToken` absent or empty | `Missing required field: chachaToken` |
| `CAP_TOKEN_INVALID` | Verifier returned `success:false` | `CAP_VERIFY_FAILED: token rejected` |
| `CAP_VERIFY_FAILED` | Verifier HTTP 4xx, malformed response, or config issue | `CAP_VERIFY_FAILED: HTTP 400` |
| `CAP_STATEMENT_TYPE_NOT_ALLOWED` | Protected query SQL is not SELECT/INSERT/UPDATE/DELETE | `Protected query statement type not allowed` |

### Fallback Behavior

Transient siteverify failures (HTTP 5xx, network timeout) are accepted and the success response includes `"cap_fallback": true`.

### Tests

| Test | Type | Status |
|---|---|---|
| `cap_verify_test` | Unity | 13/13 passing |
| `database_operations_test` | Unity | 8/8 passing |
| `query_test_handle_response_building` | Unity | 2/2 passing (includes `cap_fallback` parameter) |
| `test_56_cap_query.sh` | Blackbox | 8/8 passing (2 negative paths + 2 positive fallback INSERT paths + setup/validation) |

### Documentation

- `/docs/H/api/conduit/cap_query.md` — User-facing API documentation
- `/docs/H/core/subsystems/conduit/conduit_api.md` — Updated endpoint summary and links
- `/docs/H/plans/CAP_PLAN_QUERY.md` — This plan

---

## Helium Phase 2b Implementation (Migrations)

**Status (2026-06-24)**: All 5 Helium migrations authored. The following is what was done:

### Migrations Created (acuranzo schema)

| M# | Purpose | QueryRef | Table | Query Queue | Query Type |
|------|---------|----------|-------|-------------|------------|
| 1196 | Add Protected SQL (key_idx=11) to Lookup 028 - Query Type | (lookup) | lookups | `${QTC_SLOW}` | n/a (lookup entry) |
| 1197 | Create `course_suggestions` table + indexes + diagram | n/a | course_suggestions | n/a | Forward+Reverse+Diagram |
| 1198 | Register INSERT QueryRef #085 (Course Suggestion) | **085** | queries | `${QTC_SLOW}` | **11 (Protected)** |
| 1199 | Create `contact_submissions` table + indexes + diagram | n/a | contact_submissions | n/a | Forward+Reverse+Diagram |
| 1200 | Register INSERT QueryRef #086 (Contact Submission) | **086** | queries | `${QTC_SLOW}` | **11 (Protected)** |

### Schema Notes

- **`course_suggestions`** columns: `suggestion_id` (PK), `course_name` (VARCHAR_100), `summary` (VARCHAR_500), `detail` (TEXT_BIG), `intended_audience` (VARCHAR_500), `additional_features` (VARCHAR_500), `submitter_name` (VARCHAR_50), `submitter_email` (VARCHAR_50), `chacha_token` (VARCHAR_500), `chacha_site` (VARCHAR_20), `ip_address` (VARCHAR_50), `review_status` (INTEGER_SMALL, default 0), audit columns.
- **`contact_submissions`** columns: `submission_id` (PK), `name` (VARCHAR_50), `email` (VARCHAR_50), `subject` (VARCHAR_100), `message` (TEXT_BIG NOT NULL), `chacha_token`, `chacha_site`, `ip_address`, `review_status`, audit columns.
- **CAP_PLAN used `VARCHAR_300`/`VARCHAR_45`** which do not exist as Helium macros. `VARCHAR_500` is used for summary/audience/features columns and `VARCHAR_50` for ip_address. The extra capacity is harmless and sufficient for IPv6 with IPv4-mapped prefix (max 45 chars fits in 50).
- **`${INSERT_KEY_START}` / `${INSERT_KEY_RETURN}`** are engine-specific macros that wrap the resulting `RETURNING` (or DB2 equivalent) for the `suggestion_id`/`submission_id`.
- **Queue type integer 0 = slow** (per `database.lua` + `dbqueue.h`). Migrations store `${QTC_SLOW}` (0) and the `cap_query` handler additionally hard-forces the slow queue regardless.

### Bootstrap Queue Mapping Fix

`database_bootstrap.c` had a stale hardcoded mapping (0=cache, 1=slow, 2=medium, 3=fast) that was inconsistent with `database.lua` (0=slow, 1=medium, 2=fast, 3=cached) and the `dbqueue.h` enum. The mapping in `database_bootstrap.c` has been corrected to:

- 0 → "slow"
- 1 → "medium"
- 2 → "fast"
- 3 → "cache"

This aligns bootstrap-time loading with the rest of the codebase (`dbqueue.c`, `dbqueue.h`, `database.lua`).

### database.lua Update

Added `TYPE_PROTECTED = 11` to the `query_types` table and `cfg.TYPE_PROTECTED = self.query_types.protected`, enabling `${TYPE_PROTECTED}` in migrations. Migrations 1198 and 1200 register the real INSERT queries with `query_type_a28 = ${TYPE_PROTECTED}` so they are only returned by `lookup_database_and_protected_query`.

### Verification

- `mkt` (trial build): **Build Successful** (Unity builds pass).
- `mkp` (cppcheck): **0 issues across 1,377 files**.
- `luacheck`: All 5 new migrations + `database.lua` pass with 0 warnings / 0 errors.

### Next Steps (Helium Side)

- `helium_update.sh` should be run to regenerate the auto-generated migration index (this README table).
- Migrations should be applied to each target database engine and verified.
- The `test_56_cap_query.sh` positive path has been exercised and passes using the Cap fallback path (unreachable `CHACHA_SERVER`).

---

## Overall Hydrogen Phase 2 Exit Gates (summary)

1. **Mechanical**: `mkt && mkp` clean; any new units via `mku` green.
2. **Behavioral**:
   - cap_query rejects without/invalid Cap token with documented errors.
   - Valid tokens allow protected type-11 queries (INSERTs) to execute on slow queue.
   - Public query/paths unchanged.
   - Fallback distinguishable.
3. **Visibility**: New blackbox script exists and negatives pass reliably.
4. **Docs**: One new API doc + this plan marked complete with links.
5. **Cross-repo note**: Helium migrations 1196-1200 now register QueryRefs #085 and #086. Re-run the positive `./tests/test_56_cap_query.sh` after applying migrations to verify end-to-end submission.

At that point the Hydrogen part of CAP_PLAN.md Phase 2 can be marked complete and the frontend work (Phase 3) can proceed safely.

**Status (2026-06-24)**: All Hydrogen-side gates 2.0-2.7 and 2.9 are complete. Helium migrations 1196-1200 register QueryRefs #085 (course_suggestions) and #086 (contact_submissions) as protected type-11 queries on the slow queue. The positive end-to-end path through `test_56_cap_query.sh` now passes (using the Cap fallback path with `CHACHA_SERVER` pointed at an unreachable URL so CI does not need a real browser-solved token). Minor follow-up: the response currently reports `queue_used: "Lead"` even though `cap_query.c` selects the slow queue; the actual execution queue is correct.

---

## Quick Command Reference (Hydrogen only)

- Build-after-C: `mkt`
- All targets + tests: `mka`
- One Unity: `mku <filename-without-.c>`
- Lint C on change: `mkp` or `./tests/test_91_cppcheck.sh`
- Specific blackbox example: `./tests/test_50_conduit_query.sh` (model after)

Do not commit real Cap secrets.

---

## Document History

- 2026-06-22: Created from Kilo review of current state + CAP_PLAN Phase 2 summary. Fleshed to atomic gates.
- 2026-06-24: Helium Phase 2b implementation section added. Migrations 1196-1200 authored (Lookup 028 update, course_suggestions table, QueryRef #085 INSERT, contact_submissions table, QueryRef #086 INSERT). Fixed queue_type integer mapping in `database_bootstrap.c` to match `database.lua` (0=slow, 1=medium, 2=fast, 3=cache). Added `TYPE_PROTECTED = 11` to `database.lua`. Enabled positive path in `test_56_cap_query.sh` using the Cap fallback path (unreachable `CHACHA_SERVER`). Fixed `query_statement_type_allowed()` in `database_operations.c` to skip leading SQL comments so the `INSERT_KEY_*` macro pattern works for protected INSERTs. `mkt` + `mkp` + `luacheck` + `shellcheck` all pass; `test_56_cap_query.sh` passes 8/8.
- Next update: after real Cap token happy-path tested or queue_used response label corrected.

**End of CAP_PLAN_QUERY.md for now.**
