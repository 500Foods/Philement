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
- Helium INSERT queries (protected type 11) and the two tables (course_suggestions, contact_submissions) from Phase 2b do not exist yet.
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

- [ ] **2.0.1** Confirm INSTRUCTIONS.md disciplines are active in shell (HYDROGEN_ROOT, aliases mkt/mka/mku/mkp/mks).
  Gate: `which mkt && mkt --help 2>&1 | head -1` shows the alias works.
- [ ] **2.0.2** Run once: `./tests/test_91_cppcheck.sh` (or `mkp`) on current tree to record baseline lint count.
  Gate: Clean baseline captured (or the known pre-existing warnings listed in output).
- [ ] **2.0.3** Decide if we will support single-query only for v1 of `cap_query` (yes per master 2.3). Record here.
- [ ] **2.0.4** Create placeholder directory `src/api/conduit/cap/` (empty for now or with README stub).
  Gate: `ls src/api/conduit/cap/` succeeds.

**Gate 2.0 complete** when all four are green and plan doc is committed locally.

---

## 2.1 — Config Confirmation (fields already present — validate behavior)

- [ ] **2.1.1** Create (or expand) a small integration-style check that the three ChaCha*values are present after a full load of a test config.
  Deliverable: extend or add under `tests/unity/src/config/` or use an existing conduit bootstrap test harness. (Minimal new file if needed: `config_webserver_test_chacha_full.c`.)
  Gate: `mku config_webserver_test_chacha` passes the new assertion (chacha_* resolved correctly).
- [ ] **2.1.2** Confirm env var override works at runtime (`CHACHA_SERVER=...` etc.).
  Gate: Manual log evidence in a launched `hydrogen` brief run with override shows resolved value (use a throwaway config or the test bootstrap).
- [ ] **2.1.3** Add the three fields to one of the existing conduit blackbox JSON configs (e.g., hydrogen_test_50_...) with ${env.} or literal-dev values. Do not commit secrets.
  Gate: The test config loads without error on a dry-ish start or via Test 12/15 classes.

**2.1 exit gate**: Fields are confirmed loadable, overridable, and appear in logs/dump. No functional Cap use yet.

---

## 2.2 — Reusable Cap Verify Helper (siteverify POST)

Create `src/api/conduit/helpers/cap_verify.{c,h}`.

- [ ] **2.2.1** Create the two empty files with proper header guards + Hydrogen include style.
  Gate: `mkt` succeeds (even if empty bodies).
- [ ] **2.2.2** Implement a function (e.g., `bool cap_verify_token(const char* token, const char* site_id, char* error_out, size_t error_sz)`).
  - Resolves/uses app_config->webserver.chacha_server and chacha_secret.
  - POSTs JSON `{ "secret": "...", "response": "<token>" }` to `${server}/${site_id}/siteverify`.
  - Uses existing curl patterns (see oidc_rp_http.c and chat proxy).
  - Handles connect + total timeouts (recommend 5s connect, 10s total).
  - On success parses `{"success": true}`.
  - No secrets ever appear in non-DUMP_SECRET logs.
  Gate: Compiles (`mkt`).
- [ ] **2.2.3** Add basic negative handling: missing secret, empty token, bad HTTP, non-200, `success:false`.
  Gate: The function returns false and populates error in all those cases (no crash).
- [ ] **2.2.4** Optional: make a small unit-testable seam (e.g., pass server/secret or use existing mock patterns). For now, file-level tests allowed.
  Create `tests/unity/src/api/conduit/helpers/cap_verify_test.c` exercising success path with a fake responder if possible, or hard-negative cases.
  Gate: `mku cap_verify_test` (or the module name) passes.

**2.2 exit gate**: Helper exists, unit-tested for outcomes, cppcheck clean on the files (`mkp` or targeted), no secret leakage. Still not wired.

---

## 2.3 — New Endpoint: cap_query (single query, protected only)

Files: `src/api/conduit/cap/cap_query.{c,h}` + registration.

- [ ] **2.3.1** Create the .h + .c skeletons following `query/query.{c,h}` layout and prototypes.
  Gate: `mkt` green.
- [ ] **2.3.2** Implement handler `handle_cap_query_request(...)` that:
  - Does the same buffering + method validation as public query.
  - Uses existing `handle_request_parsing_with_buffer`, `handle_field_extraction`.
  - After extraction, pulls chachaToken + chachaSite from body.
  - Calls cap_verify helper.
  - On any verify failure → return clear 4xx **CAP_VERIFY_* style response (see 2.6)** and stop.
  - On success → proceed identically to the public path for the rest of the pipeline.
  Gate: `mkt` + hand inspection of flow.
- [ ] **2.3.3** Register lookup for **protected queries** only (type 11).
  Add or reuse a helper:
  `bool lookup_database_and_protected_query(DatabaseQueue**, QueryCacheEntry**, const char*, int)`
  applying the `query_cache_lookup_by_ref_and_type(..., 11, ...)` filter.
  Gate: `mkt` + the call is present in the cap handler where public would have used type-10.
- [ ] **2.3.4** Add the new endpoint include in `api_service.c`.
  Gate: `mkt`.
- [ ] **2.3.5** Add routing: `"conduit/cap_query"` arm in the big if-chain + update the JSON expect list and the log list.
  Gate: after `mkt`, grep for the string appears; no dispatch to old path.

**2.3 exit gate**: `/api/conduit/cap_query` is reachable (even if it rejects every query due to missing type-11 items). Handler logs its entry. Rejected paths return quickly.

---

## 2.4 + 2.4b — Protected Type and Queue/Statement Guarding

- [ ] **2.4.1** Extend `lookup_database_and_protected_query` to also assert the cache_entry's query_type == 11; hard 400 otherwise (or 404 if not found).
  Gate: Direct test via unit or quick blackbox (modify a type-10 query temporarily — do not leave it).
- [ ] **2.4.2** In relevant queue selection or submission helpers, add explicit statement-type restrictions:
  - Type 10 (public) may only execute statements starting with SELECT (or as per existing cache rule).
  - Type 11 (protected) may be INSERT/UPDATE/... but land on slow queue only.
  Gate: Unit or small blackbox asserts the wrong type/queue combination is rejected at registration or submit time.
- [ ] **2.4.3** Ensure queue selection for type 11 always prefers/uses the slow queue explicitly (`queue_type = "slow"` or equivalent).
  The cache_entry already carries `queue_type` — make sure cap path enforces it.
  Gate: Evidence that a hypothetical type-11 INSERT query would choose slow.

**Note**: Actual registration of the two protected INSERT queries (QueryRef 85/86) is in Helium/Phase 2b. We will test against them once they exist. Here we only build the gates and negative enforcement.

---

## 2.5 — Fallback Policy (Cap outage / verify hard-fail)

Per master 0.4: accept + flag for review + distinguishable response.

- [ ] **2.5.1** Decide on exact success envelope difference (new top-level field? `cap_fallback: true` + review_status inside row?).
  Record decision in this plan.
- [ ] **2.5.2** Implement the path inside cap_verify (or a wrapper) and in the handler: on permanent failure or timeout policy, return a success-shaped response containing the flag.
  The insert itself still happens (the query runs).
  Gate: Functional proof (you can force a verify failure by using bad secret and see a query still accepted but marked).
- [ ] **2.5.3** Ensure the flag surfaces in the final response (and later in the row if the INSERT returns the id).
  Gate: JSON shape documented under 2.6 matches implementation.

**2.5 exit gate**: Fallback exercised (unit or scripted); normal success vs fallback success can be told apart without looking at Cap logs.

---

## 2.6 — Error Contract (Frontend can branch cleanly)

- [ ] **2.6.1** Define and implement the error cases:
  - `CAP_TOKEN_MISSING`
  - `CAP_TOKEN_INVALID`
  - `CAP_VERIFY_FAILED` (network or downstream 5xx or false)
  - `CAP_SITE_MISMATCH` (optional but cheap)
  - Use the same outer shape as other conduit errors: `{ "success": false, "error": "...", "message": "..." }`
  Gate: All listed errors produced from the handler on the right triggers (blackbox or unit).
- [ ] **2.6.2** Add a small table of error → HTTP status in the handler header comment or here:
  Missing/Invalid/Verify → 400 or 403 (decide once).
  Gate: The chosen mapping written here + matches code + is tested by at least one negative case each.

**2.6 exit gate**: Frontend author can look at this section + error JSON examples and know exactly how to branch.

---

## 2.7 — Tests (Unit + Blackbox following the conduit template)

- [ ] **2.7.1** Unit test coverage for cap_verify (already partially done in 2.2).
- [ ] **2.7.2** Copy pattern from `test_50_conduit_query.sh` → create `test_56_cap_query.sh`.
  - 7-engine config (reuse/extend one).
  - Adds a fake or real protected query entry (see 2.4 note).
  - Tests without token → clear CAP_* error.
  - Tests with invalid token → CAP_* error.
  - Tests with real token (once secret + site + QueryRef exist) → 200 + row returned.
  - Optionally a fallback-forced case.
  Gate: The script at least gets as far as "server starts" + first negative path and returns nonzero on the expected fail cases.
- [ ] **2.7.3** Integrate the protected query refs into conduit_utils.sh (new map `PROTECTED_CAP_QUERY_REFS`) when the real refs are known.
  Gate: Sourced cleanly; used by the test.
- [ ] **2.7.4** Run the new blackbox on at least one engine after the server + queries are there.
  Gate: All cases explicitly expected (positive and two negatives) show PASS.
- [ ] **2.7.5** Add the test to the orchestration comment in `test_00_all.sh` (or the conduit grouping later). Do not wire it to full auto-run until Helium pieces exist.

**2.7 exit gate**: Negative cases are reliable and run in CI-friendly blackbox; positive path gated behind real protected refs.

---

## 2.8 — Cached Query Result Storage & Refresh (separate but listed here)

(This is called out in master 2.8 attached to the Phase 2 work. It is broader than Cap but will affect how type-11 results are served.)

- [ ] **2.8.1** Find the note in docs/H/plans/CONDUIT.md.
- [ ] **2.8.2** Identify precise files: database_bootstrap.c + database_cache.{c,h} + query_execution + worker dispatch.
- [ ] **2.8.3** Implement: on bootstrap for queue_type=="cache", run once, store compressed blob + timestamp in QueryCacheEntry.
- [ ] **2.8.4** Serve the cached blob until TTL; async refresh.
- Gate sequence: design sketch accepted → compile → basic unit or man-test.

Note: Decide with team if this is required before the first real cap-protected INSERT forms go live.

---

## 2.9 — Documentation

- [ ] **2.9.1** Add a Hydrogen doc page (under docs/H/api/conduit/ or similar): `cap_query.md`.
  Contents:
  - Exact payload with chacha fields.
  - Response shapes (success, fallback, error objects).
  - error codes table from 2.6.
  - Config keys and secret hygiene.
  - Siteverify URL example from the key record.
  - How to force a fallback for testing.
  Gate: `test_04_check_links.sh` still passes after add.
- [ ] **2.9.2** Update this plan's "as-built" section with final paths, exact error strings, HTTP codes chosen, and QueryRef numbers used.
- [ ] **2.9.3** Brief entry in CAP_PLAN.md Phase 2 (under the existing list) pointing to this document and the new Hydrogen doc.

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
5. **Cross-repo note**: When Helium 1197/1199 (or whichever final numbers) land and real QueryRefs are assigned, rerun the positive path of test_56.

At that point the Hydrogen part of CAP_PLAN.md Phase 2 can be marked complete and the frontend work (Phase 3) can proceed safely.

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
- Next update: after 2.2 or 2.3 first real passing gate.

**End of CAP_PLAN_QUERY.md for now.**
