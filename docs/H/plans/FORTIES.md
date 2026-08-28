# Forties — tests 40–47 under suite load

**Status:** in progress (Hydrogen 503/budget landed; suite gate still required).
**Code:** `elements/001-hydrogen/hydrogen/tests/test_4*.sh`, `tests/lib/group40_http.sh`, `src/api/auth/login/login.c`, `src/api/auth/auth_service_database.c`.
**Bar:** group 4 stays **parallel**. Concurrent suite load is deliberate: it finds Hydrogen races, not an excuse to serialize the batch.

Last full suite snapshot (2026-08-27 `181015`, build 2531): **7 fails total** (4101 / 4094). Group 4: **40** 4 fail (Postgres only); **43** CockroachDB default-DB incomplete lifecycle; **44** unresponsive after 1000/5000; **47** Postgres login HTTP 000. 41/42/45/46 green. Prior `170403` was 40:12 + 47:1.

---

## Intent

Two jobs at once:

1. Each 40-series script is a real blackbox of that feature (JWT, ASAN exercise, OIDC RP/IdP, scripting, conduit script, MCP).
2. Running them **together** (and 41/44 hammering the same live DBs) must make Hydrogen **slower and still correct**, not lie with 401 / lockout / “invalid credentials”.

Do not “fix” (2) by skipping engines, dropping concurrency, or unbounded retries.

---

## What we learned

### Isolation

- **Ports are not the flake.** `5<TT>x` / `15<TT>x` ranges already isolate listeners. 41 and 44 use different ports on purpose so they can run together.
- **Live demo DBs are the shared resource.** Postgres, MySQL, MariaDB, DB2, Cockroach, Yugabyte: one Acuranzo/demo per engine. Tests 40, 42, 43, 45, 46, 47 login and mutate auth/token/session rows while 41/44 fire hundreds/thousands of auths.
- **SQLite** is a file: WAL = concurrent readers, **serialized writers**. Copies in 40/45/46/47 avoid mutating `hydrodemo.sqlite` and AutoMigration holes. 42/43 still use the shared file. Copies are isolation, not “SQLite cannot do concurrency.” Leave copies for now.

### Why “slow” became “fail”

Hydrogen does not wait forever. Auth QueryRefs have a budget/watchdog. Under 41/44 that budget is often exceeded. The client then saw:

- HTTP **401 Invalid credentials** for a **cancelled or failed** password query (same as wrong password).
- `log_login_attempt` + `LOGINMAXATTEMPTS` lockout of `127.0.0.1`.
- curl `--max-time` 5–20s aborting while the server was still queued (`000`).
- Tests treating **401/404 as definitive** (no retry) and **invalid-login expecting 401** when the server returned 000/500/503.

Databases were not falling over. The **app mapped congestion to auth failure**.

### Test 47 standalone vs suite

Standalone flakes were **not** missing 90s waits. HTTP 200 with wrong JSON (ping/EchoStrict), or overlap Echo **401,200** (JWT/session under concurrent POST). Leftover `.body` files were later `{"error":"session not found"}` after DELETE — clobber hid the real payload.

Suite `170403`: 47 **login HTTP 000** on the same engines (Postgres, Cockroach) that **test 40 never got past STARTUP**. Feature cases after login were fine on the other five engines.

### 503 vs expected 401

`verify_password_and_status_code` now returns **-1** on query/transport failure → login **503** `Authentication service unavailable` (no failed-attempt row). Yugabyte **invalid-password** in that suite got **503**, so `LOGIN_INVALID` (expects 401) failed. Congestion on the **wrong-password** QueryRef is still a product/test mismatch: 503 is honest; the test must allow it, or #012 must complete.

---

## What we did

### Harness (`group40_http.sh`)

Shared wait policy for 40–47:

| Knob | Value |
| ------ | -------- |
| HTTP max-time | 90s |
| Connect | 10s |
| Startup | 90s (41 ASAN start still 180s) |
| Ready | 120s |
| Shutdown | 30s |
| Connect retries | 2, **000 only** |
| INFO delay | log when a call ≥ 2s |

Prefer **one long wait** over abort-and-retry-on-timeout. Retry **real** failures (empty connect, JSON mismatch) a few times (3), not cancelled-in-flight work.

Wired through 40, 46, 47, MCP helpers, OIDC IdP/RP curls, 43/45 startup, exercise scrape (90s, 2–3 tries). **41/44 flood stays 15s max-time** (500/5000 in-flight). 41/44 **Yugabyte remains disabled**.

### Test 47 scoring

- `mcp_expect_jq`: 3 tries, INFO + body excerpt, per-case files (no clobber).
- ping, EchoStrict, cursor, hidden tools, resources/read, prompts get/unknown.
- Overlap Echo: 3 tries on 401.
- MCP calls use 90s, not 10–20s.

### Hydrogen (auth)

- `verify_password_and_status_code`: **1** match, **0** mismatch, **-1** query fail.
- Login **-1** → **503**, no `log_login_attempt` / lockout.
- Wrong password still **401**.

Rebuild after this C change: `mkt` then `mkp` (do not share the CMake dir with a live suite).

---

## Tests in the 40s

| # | Role | Engines | Notes |
| --- | ------ | --------- | -------- |
| 40 | JWT login/renew/logout/register | 7 | Shared live DBs; SQLite copy |
| 41 | ASAN 500 concurrent auths | 6 | Yugabyte off; port 5410 |
| 42 | OIDC RP | SQLite-centric configs | Shared `hydrodemo.sqlite` |
| 43 | Scripting orchestrator | 7 ×2 variants | Log-based; 90s/30s |
| 44 | Native 5000 auths RSS | 6 | Yugabyte off; port 5444 |
| 45 | OIDC IdP | 7 | SQLite copy; group40 curls |
| 46 | Conduit `/api/conduit/script` | 7 | SQLite copy + seed |
| 47 | MCP Streamable HTTP | 7 + disabled | SQLite copy; 1547x/1548x |

---

## Latest suite evidence (`181015`)

Root cause on the remaining nicks is **Postgres `FATAL: sorry, too many clients already`**, then Hydrogen **SIGSEGV** (`heartbeat.c` `MUTEX EXP` 500ms on `connection_lock`). 503 on API-key timeout worked; the process then died, so later curls are 000/empty.

**Test 40** (47/43): MySQL/MariaDB/DB2/SQLite/Cockroach/Yugabyte full login/invalid/renew/logout/register. **Postgres:** login 503 `Authentication service unavailable`, then crash; invalid-login empty; register fail.

**Test 43:** CockroachDB default-DB orchestrator incomplete lifecycle (ND variant passed).

**Test 44:** metrics scrape failed at 500 and 1000; aborted as unresponsive.

**Test 47:** Postgres login HTTP 000 after the same too-many-clients + Signal 11. Other engines including Cockroach logged in.

## Previous suite evidence (`170403`)

### Test 40

- MySQL, MariaDB, DB2, SQLite: full login/invalid/renew/logout/register OK.
- **Postgres, Cockroach:** `STARTUP_SUCCESS` only — no login lines (ready/auth never completed).
- **Yugabyte:** `LOGIN_INVALID_FAILED`; body `Authentication service unavailable` / 503.

### Test 47

- Five engines full CASE_PASS including overlap/ping/EchoStrict.
- **Postgres, Cockroach:** `LOGIN_FAILED` `LOGIN_HTTP=000` (never connected / curl empty after 90s×2).

Same two live engines stalling in 40 and 47 is the next product lead, not more test retries.

---

## Landed (heartbeat crash)

- Heartbeat held `connection_lock` then called `check_connection` → `handle_connection_success` re-locked the same non-recursive mutex (500ms MUTEX EXP).
- On timeout it cleaned up the new handle (frees config) then `check_connection` freed config again → SIGSEGV (`181015` Postgres 40/47).
- Reconnect now runs **after** unlock. Attempt return: **1** stored, **0** no handle (caller frees config), **-1** handle already cleaned up.

## Landed (Hydrogen)

- Login holds a **45s query budget** (`auth_query_begin_deadline`). Stacked 20s waits no longer outlive MHD 60s (HTTP 000).
- `lookup_account_code` / `verify_api_key_code`: **1 / 0 / -1**. Query fail → **503**, not 401.
- Empty lookup row is **not found** (401), not an empty account that then 503s on #012.
- `jwt_token_store_status`: query fail → `JWT_ERROR_UNAVAILABLE` → **503** on renew/logout/conduit/MCP (overlap 401 was this).
- Test 40 invalid-login: **401 or 503** with `Authentication service unavailable`.

## Left to do

1. **Full suite gate** after this binary — PG/CR must *answer* (200 or 503), not 000. 200 under 41/44 is still the bar.
2. **Do not** enable Yugabyte on 41/44 until 40/47 are boring on the six engines that already run there.
3. **Do not** drop SQLite copies until 47 is green on the shared fixture like 42.
4. **Do not** add retry loops for timeouts that would have completed; keep INFO delay lines.

---

## How to run

```bash
zsh -ic 'mkt'    # after C changes
zsh -ic 'mkp'    # cppcheck
# Test 47 standalone, several times, then:
./tests/test_00_all.sh          # group 4 parallel (default)
```

Confidence: standalone 47 ×10 is necessary but not sufficient. The **full suite** is the gate.

---

## Reproduction results (2026-08-28)

Built `hydrogen_release` / `hydrogen_debug` / `hydrogen_coverage` via `ninja -C build` (all three succeeded). Environment: Postgres on `localhost:5432` (max_connections=100), MySQL alive, demo creds from env (`HYDROGEN_DEMO_USER_NAME` / `HYDROGEN_DEMO_USER_PASS` / `HYDROGEN_DEMO_API_KEY`).

### Focused replay of the Postgres regression

Started one `hydrogen_release` against `tests/configs/hydrogen_test_40_postgres.json` (port 5401) and replayed the failure shape (heavy concurrent `/api/auth/login` against Postgres):

- **Crash path:** the release binary (heartbeat fix compiled in) survived a 16-worker × 150 = 2400-login flood for ~300s with **no SIGSEGV / no abort**. The original double-free (heartbeat holding `connection_lock` while `handle_connection_success` re-locked) is gone.
- **Login under load:** 8 workers × 200 = 1600 concurrent logins + 60 logins sampled *during* the flood → **all 1660 returned HTTP 200**. No 000, no 503. Server stayed alive. This is the bar ("200 under 41/44") on the single-process Postgres path.

### Wrong-password path (secondary fix)

Pre-fix: wrong password returned **503** `Authentication service unavailable` even without congestion. Root cause: Postgres `execute_query` (`src/database/postgresql/query.c`) left `data_json = NULL` on the **0-row success** path, while SQLite and DB2 both set `data_json = strdup("[]")`. `json_loads(NULL)` in `verify_password_and_status_code` then failed → `-1` → 503 (conflating "no match" with "query failure").

- Fix: set `db_result->data_json = strdup("[]")` in the Postgres 0-row `else` branch (both `postgresql_execute_query` line ~558 and `postgresql_execute_prepared` line ~887), matching SQLite/DB2.
- Post-fix: wrong password → **401** `Invalid credentials`; correct password → **200**; flood (1600+60) → all **200**, server alive.

> Note: `/api/version` can answer 200 before the DB query cache is bootstrapped (migration not yet complete). Tests must wait for `Migration completed in` / `READY FOR REQUESTS` before auth calls — test_40 already does (changelog 1.5.0). A repro that only gated on `/api/version=200` races the cache and sees a legitimate transient 503.

### ASAN note

`hydrogen_debug` does **not** embed the payload (only `hydrogen` / `coverage` / `hydrogen_release` are embedded per `cmake/CMakeLists-targets.cmake`), so standalone ASAN runs can't populate the query cache (`QueryRef 1 not found in cache`) and can't exercise the auth path. The release binary (same C, heartbeat fix compiled in) is the validated carrier for this regression. ASAN gate should run through `mkt` (test_41), which wires the payload.

### Verdict

Postgres `000/empty` login is resolved: graceful 200 under load, 401 on bad creds, 503 only on genuine query failure — no crash. Next gate is the full suite (`./tests/test_00_all.sh` group 4).

---

## Suite checkpoint (full run — build 2531, 2026-08-27)

Results (`build/tests/results/results_data.json`; per-engine outcomes in `build/tests/diagnostics/test_40_.../*`):

- **Test 40:** 4 fails — **Postgres** + **Cockroach** only: `LOGIN_FAILED` + `REGISTER_FAILED` each. MySQL/MariaDB/DB2/SQLite/Yugabyte full pass (43/47).
- **Test 46:** 1 fail (17/18).
- **Test 47:** 1 fail (19/20).
- **Unity:** 10-UNT `10,353/10,353` passed (all unit tests green).

### Root cause (Postgres / Cockroach)

Both engines share the live cluster `localhost:5432` (`max_connections=100`); Group 4 runs 40/41/44/47 concurrently, contending the pool across processes. Postgres log at startup:
`FATAL: sorry, too many clients already` → `Database connection failed - no handle returned` → `QueryRef 1 not found in cache` → login **503** / register fail.

This is **connection-slot exhaustion at startup**, distinct from the single-process wrong-password `000` regression fixed above (that one: 200 under load, 401 on bad creds, no crash). Single-process bar met; suite gate still needs Postgres/Cockroach to *answer* (200 or 503) under true parallelism.

### Action direction

1. Size Postgres `max_connections` / shard per-engine instances so test_40 obtains a connection during the concurrent run, OR
2. Make the auth path **retry / reconnect on `too many clients`** (transient) in `src/database/postgresql/connection.c` instead of mapping it to 503.

---

## unity coverage gap: login.c

`src/api/auth/login/login.c` unity coverage = **48.56%** (118/243 lines; `build/unity/src/api/auth/login/login.c.gcov`). The 18 error-path tests (`login_test_error_paths.c`) cover input/buffer/early-exit branches but **not** the success path (lines 370–458: `auth_roles_from_database` → `generate_jwt` → `compute_token_hash` → `store_jwt` → 200 response), nor `api_key_code==0` (Invalid API key), nor the wrong-password→401/429 tail.

### Unit-test work (2026-08-28)

Wrote `tests/unity/src/api/auth/login/login_test_success_paths.c` — a standalone `_test*.c` harness mirroring `login_test_error_paths.c`. Auto-discovered by the `*_test*.c` glob in `cmake/CMakeLists-unity.cmake:19`, producing a `login_test_success_paths` executable per the per-file wiring at `cmake/CMakeLists-unity.cmake:66-69`.

Harness design (inline weak mocks, same linkage model as error_paths — test .o weak defs register first, strong archive members skipped):

- Mocks: `validate_login_input`, `verify_api_key_code`, `check_license_expiry`, `api_get_client_ip`, `check_ip_whitelist`/`check_ip_blacklist`, `check_failed_attempts`, `handle_rate_limiting`, `lookup_account_code`/`lookup_account`, `verify_password_and_status_code`, `generate_jwt`, `compute_token_hash`, `store_jwt`, `free_account_info`, `api_buffer_post_data`, `api_send_json_response`, `mock_mhd_reset_all`.
- **New seams not in error_paths:** `auth_roles_from_database` (defined in `src/api/auth/oidc_rp/oidc_rp_roles.c:162`, declared `auth_service.h:159` — NOT mocked elsewhere in login tests) is stubbed weak so the success path links against controlled roles. `api_send_json_response` is a **capturing** mock (records status code + response JSON) so success assertions can check body fields (`success`, `token`, `user_id`, `username`, `email`, `roles`).

Six tests:

1. `test_handle_auth_login_success` — full happy path → HTTP 200, asserts response body fields.
2. `test_handle_auth_login_invalid_api_key` — `verify_api_key_code=0` → 401 "Invalid API key".
3. `test_handle_auth_login_wrong_password_unauthorized` — verify fails, no rate limit → 401 "Invalid credentials".
4. `test_handle_auth_login_wrong_password_rate_limited` — verify fails + rate limit → 429.
5. `test_handle_auth_login_roles_fallback_empty` — `auth_roles_from_database` NULL → `strdup("")` fallback → `roles=""`.
6. `test_handle_auth_login_jwt_generation_failure` — `generate_jwt` NULL → 500.

**Status:** file written. Verification pending: the `*_test*.c` glob is resolved at CMake configure-time, so a **re-configure** is required before the new file compiles. Build commands:

```bash
cmake -B build -S .        # re-configure to pick up the new globbed file
ninja -C build login_test_success_paths   # or: ninja -C build unity_tests
./build/unity/login_test_success_paths     # expect 6 pass / 0 fail
gcov -o build/unity/CMakeFiles/login_test_success_paths.dir/src/api/auth/login src/api/auth/login/login.c   # confirm success lines covered
```

Build config (from workspace inspection): Ninja generator, compiler `/usr/lib64/ccache/cc`, login.c compiled into `libhydrogen_unity.a` with `--coverage` + auth mock defines (`USE_MOCK_CRYPTO/_API_UTILS/_DBQUEUE/_AUTH_SERVICE_JWT/_LIBMICROHTTPD/_AUTH_CHAT_DEPS/_SYSTEM`) in `cmake/CMakeLists-unity.cmake`.

### Remaining product gap (suite gate)

Test 40/47 still fail on **Postgres + Cockroach only** under true Group-4 parallelism: `FATAL: sorry, too many clients already` at startup → `QueryRef 1 not found in cache` → login 503 / register fail (see "Action direction" below). Single-process bar met (200 under flood, 401 bad creds, 503 only genuine query failure); suite gate still needs connection-slot exhaustion resolved.

### Action direction (suite)

1. **DONE (infra, 2026-08-28):** raised Postgres `max_connections` 100 → **500** in `/var/lib/pgsql/data/postgresql.conf` + restart. Eliminates the startup-slot race for the 4-way concurrent suite (4 binaries × up to 16 PG slots ≪ 500).
2. (Hardening, optional) expose Hydrogen's `max_connections_per_database` as a config knob — see note below.

### Hydrogen connection demand cap (hardcoded)

`DatabaseSubsystem.max_connections_per_database` is hardcoded to **16` at `src/database/database.c:68` (field at `src/database/database.h:253`) and is **not** bound to hydrogen.json — unlike`Databases.ConnectionCount` / `Databases.BootstrapTimeoutSeconds` / `Databases.BootstrapRetries` which ARE read from config via `PROCESS_INT` in `src/config/config_databases.c:98-100`. Exposing`Databases.MaxConnectionsPerDatabase` (struct field + `PROCESS_INT` + propagate to subsystem at init) would let operators tune demand without a Postgres restart. Not required to unblock the suite now that `max_connections=500`, but a reasonable refactor.
