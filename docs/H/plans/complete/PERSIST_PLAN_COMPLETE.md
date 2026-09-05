<!-- markdownlint-disable MD007 MD024 -->
# Mail Relay Persist — MySQL/MariaDB Fetch SIGSEGV

## Status

**Complete (2026-09-04).** MySQL/MariaDB `Queue.Persist` is live-green; full
7-engine × plaintext/STARTTLS matrix passes in 27.8s (helpers 1.0.11,
test_58 2.9.2; 14/14, 20/20 tests, 0 fails). Two parts landed:

1. **Phase 1b — C result-path guard.** `mysql_process_prepared_result`
   honours `mysql_stmt_store_result` rc; on failure (e.g. duplicate-key
   INSERT...RETURNING) it logs `mysql_stmt_error`, frees metadata, and
   returns `success=true` with empty JSON + `affected_rows` so the
   `mysql_stmt_fetch` SIGSEGV (`fetch_row_func` NULL) cannot happen.
2. **Phase 2c — Engine-aware timestamp translator.** `repo_add_datetime`
   in `mailrelay_repository.c` looks up the target connection's engine
   type and translates ISO 8601 (`YYYY-MM-DDTHH:MM:SS[.fff]Z`) to MySQL
   DATETIME (`YYYY-MM-DD HH:MM:SS`) only for `DB_ENGINE_MYSQL`; the 5
   other engines pass through unchanged so their working ISO 8601 path is
   preserved.

TODO 12d closed. See [TODO.md](/docs/H/TODO.md) and
[MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md) for the upstream
references; this archive is the canonical history of the bind ABI / fetch
NULL investigation that took eight attempts before the live gdb + Vector D
diagnostic identified `mysql_stmt_fetch` as the crash site.

## Purpose

Fix `Queue.Persist` on **MySQL and MariaDB** so QueryRef **093** (insert pending `mail_queue` row) no longer SIGSEGVs in `mysql_stmt_fetch` after `INSERT … RETURNING`. Until that is live-green, those engines keep Persist **off** in Test 58.

This is Hydrogen TODO **12d**. It is **not** Mail Relay product work (templates, API, OTP, HA claim). It is the MySQL prepared-statement **result** path: `store_result` failure left `fetch_row_func` NULL, then Hydrogen fetched anyway.

Parent: [MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md). Distinct from TODO **12e** (MAX+1 PK race). Empty-id retry in `mailrelay_persist_message` already exists; do not invent UUID primary keys.

## How To Use This Document

- Work one phase at a time. Do not re-enable Persist as a “try it” while the crash is unfixed.
- Unity-green is **not** the exit gate. Live `test_58` MySQL **and** MariaDB with Persist on is the gate.
- Record each failed live run in the Working Log with the exact hypothesis, so the next session does not repeat it.
- When the live gate is green, walk **Post-fix update checklist** in the same change, then move this file to `complete/`.

## Current Pause (2026-09-04)

| | |
| --- | --- |
| **Symptom** | Hydrogen dies in `mysql_stmt_fetch`; `rip = 0x0`, `fault (nil)` |
| **Trigger** | `MailRelay.Queue.Persist=true` on mysql/mariadb → QueryRef 093 `INSERT … RETURNING queue_id` |
| **Real crash site** | `mysql_process_prepared_result` in [`query_helpers.c`](/elements/001-hydrogen/hydrogen/src/database/mysql/query_helpers.c) calls `mysql_stmt_fetch` after ignoring a failed `mysql_stmt_store_result` |
| **Not the crash** | `mysql_stmt_bind_param`. Connector/C only `memcpy`s into `stmt->params`. Last TRACE is bind because [`query.c`](/elements/001-hydrogen/hydrogen/src/database/mysql/query.c) logs nothing between bind success and fetch |
| **Shield** | `mailrelay_api_persist_enabled()` returns `false` for `mysql` and `mariadb` (`tests/lib/mailrelay_api_helpers.sh` **1.0.8**; `test_58` **2.8.9**) |
| **Other engines** | Persist **on** via the Test 58 jq runtime patch (checked-in `hydrogen_test_58_*.json` still say `"Persist": false`; the helper overwrites) |
| **Last live result** | gdb on Test 58 MariaDB config with Persist on (shield files untouched): login 200, POST `/api/mailrelay/send` → SIGSEGV. Frame `#1 mysql_stmt_fetch` → `#2 mysql_process_prepared_result` → `#3 mysql_execute_query` → `mailrelay_repo_queue_insert` QueryRef 93 |
| **Approved next** | **Guard + Phase 2 live.** Check `store_result` rc; skip fetch on failure; TRACE after bind/execute/store/fetch; `mkq`/`mkp`; then flip Persist and run Test 58 mysql/mariadb. Restore the shield on SIGSEGV |
| **Do not trust** | TODO 12d “Persist enabled for all engines”; MAILRELAY resume still saying `mysql_stmt_bind_param`; older pause rows with helpers 1.0.4 / test_58 2.8.5 |

## The Issue

### Call path

1. `mailrelay_enqueue` / `mailrelay_persist_message` ([`src/mailrelay/mailrelay.c`](/elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay.c))
2. `mailrelay_repo_queue_insert` ([`src/mailrelay/mailrelay_repository.c`](/elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_repository.c)) — 12 named params
3. QueryRef **093** ([`acuranzo_1223.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1223.lua)) — `INSERT INTO mail_queue … FROM next_queue_id` with `${INSERT_KEY_START}` / `${INSERT_KEY_RETURN} queue_id`
4. MySQL engine prepare/bind/execute in [`query.c`](/elements/001-hydrogen/hydrogen/src/database/mysql/query.c)
5. `mysql_process_prepared_stmt_result` → `mysql_process_prepared_result` in [`query_helpers.c`](/elements/001-hydrogen/hydrogen/src/database/mysql/query_helpers.c)
6. Crash **inside the client `.so`** at `mysql_stmt_fetch` (`fetch_row_func` is NULL)

Params on 093: `MESSAGE_UUID`, `PRIORITY`, `TEMPLATE_KEY`, `FROM_ADDR`, `REPLY_TO`, `RECIPIENTS_JSON`, `SUBJECT`, `BODY_TEXT`, `BODY_HTML`, `HEADERS_JSON`, `IDEMPOTENCY_KEY`, `NEXT_ATTEMPT_AT`.

### Why it looks like bind_param in the logs

`query.c` TRACE-logs `mysql_stmt_bind_param for N parameters` then, on success, is silent through execute and result processing. Test 58’s last Hydrogen line before signal 11 is that bind TRACE. gdb and Vector D both show bind already returned 0.

### Why Persist dies and `/api/auth/login` often does not

QueryRef 093 is `INSERT … SELECT … RETURNING queue_id`, not a plain `INSERT`. After prepare: `param_count=12`, `field_count=0`. After execute: `field_count=1`, `state=WAITING_USE_OR_STORE`, `fetch_row_func=NULL` until `store_result` succeeds.

- Unique `message_uuid`: `store_result` rc=0 sets `fetch_row_func`; `bind_result`+`fetch` return `queue_id`. No SIGSEGV.
- Duplicate `message_uuid` (Test 58 idempotency / retry): `store_result` rc=1 (`Duplicate entry…`), `fetch_row_func` stays NULL, **`mysql_stmt_fetch` SIGSEGV `fault (nil)`**.

Typical auth queries are SELECTs whose `store_result` succeeds, so they never hit the unguarded fetch. PG/SQLite/DB2/Cockroach/Yugabyte Persist paths already run in Test 58.

`mysql_process_prepared_result` always calls `mysql_stmt_store_result` (return ignored) then, when metadata exists, always `bind_result`+`fetch`. That is the production bug.

### Bind ABI (closed — not this crash)

Phase 0 measured `sizeof(MYSQL_BIND)=112` header == hand-roll; `libmysqlclient.so` is `libmariadb.so.3`. Phase 1 included `<mysql.h>` and made `length` non-NULL. Unity-green. Live still SIGSEGV. Vector D with the real 093 SQL then gdb proved bind is not the crash. Do not reopen layout guessing.

Result binds still use the hand-rolled `MYSQL_BIND_COMPLETE` in `query_helpers.c`. Leave that alone unless a new live crash points at `bind_result`.

## Attempts Already Burned

Do **not** present these as new work.

| # | Attempt | Result |
| --- | --- | --- |
| 1 | Shield Persist off mysql/mariadb | Workaround **still in place** (helpers **1.0.8**, test_58 **2.8.9**). Not a fix. |
| 2 | INTEGER `MYSQL_TYPE_LONG` → `MYSQL_TYPE_LONGLONG` for 8-byte `long long` | Necessary. Not sufficient. |
| 3 | Hand-rolled `MYSQL_BIND` nudged toward MariaDB | Layout matched (Phase 0). Not the crash. |
| 4 | `is_null`/`error` indicators + SQL NULL → `MYSQL_TYPE_NULL` | Unity-green. Live still SIGSEGV. |
| 5 | Live Test 58 after (4) | SIGSEGV. Shield restored. |
| 6 | Phase 1: `#include <mysql.h>` + non-NULL `length` for every bind type | Unity-green (`query_test_mysql_bind_persist_shape` 17/17). Live still SIGSEGV at the same TRACE. |
| 7 | Standalone 12-placeholder INSERT bind probes | `bind_param` returned 0. Falsified bind as the crash. |
| 8 | Vector D with live QueryRef 093 SQL (`extras/probe_persist_q093.c`) | Bind+execute OK. Fetch SIGSEGV only after failed `store_result` (duplicate UUID). |
| 9 | gdb on live Hydrogen (Persist on, shield files untouched) | Confirmed `#1 mysql_stmt_fetch` from `mysql_process_prepared_result`, QueryRef 93. |

Also out of scope as a “fix”:

- Skipping MySQL/MariaDB in Test 58 forever
- Serializing tests to hide 12e races
- Changing MAX+1 / `INSERT_KEY_*` to UUIDs or `${SERIAL}`
- Treating `mku query_test_mysql_bind_single_parameter` as the production gate
- Another `MYSQL_BIND` layout tweak or assigning `store_param_func`

## Fix Vectors

A–C (ABI sizeof / include header / remaining bind nils) are **closed**. D diagnosed the real crash. Implement **E**.

### E — Guard store_result / fetch (approved)

In `mysql_process_prepared_result`:

1. Capture `mysql_stmt_store_result` return. On nonzero: log `mysql_stmt_error`, do **not** `bind_result` or `fetch`. Treat as no result row (empty JSON / `success` consistent with other engines’ duplicate-key / empty RETURNING path so Persist retry can run).
2. Add TRACE after bind, execute, store_result, and fetch so the next live run can localize without gdb.
3. `zsh -ic 'mkq'` then `zsh -ic 'mkp'`. No new `src/` file (no `mkt`). No new `static` functions.

**Done means:** C + cppcheck green. Shield still on until Phase 2.

Do not “fix” this by rewriting QueryRef 093 SQL. Duplicate-key empty RETURNING is expected; crashing on it is not.

## Phases

### Phase 0 — ABI measurement

**Entry:** this document read; shield left **on**.

- [x] 0.1 Record header vs hand-rolled `sizeof`/`offsetof` and the loaded `.so`.
- [x] 0.2 Append numbers to Working Log. Choose vector B or C.

**Exit:** Working Log has measurements. No Persist flag change. **Phase 0 done 2026-09-04.** Vector C was selected then; later evidence superseded it (crash is fetch, not bind).

### Phase 1 — Bind ABI fix

**Entry:** Phase 0 identified B or C.

- [x] 1.1 Include `<mysql.h>` in `query.c`; non-NULL `length` for every bind type; rename colliding Hydrogen wrappers.
- [x] 1.2 `mkt` (CMake flags) then `mkp`. `mks` green.
- [x] 1.3 `mku query_test_mysql_bind_single_parameter` (13/13) and `mku query_test_mysql_bind_persist_shape` (17/17).

**Exit:** lint + Unity green. Shield still on. **Met 2026-09-04.** Live Test 58 after this still SIGSEGV — variance: wrong crash site. Do not reopen.

### Phase 1b — Fetch-after-store_result guard

**Entry:** Phase 1 Unity-green; gdb + Vector D agree the crash is `mysql_stmt_fetch`. User approved Guard + Phase 2 live.

- [x] 1b.1 In `mysql_process_prepared_result`, honor `mysql_stmt_store_result` rc; skip fetch when it failed; log the client error.
- [x] 1b.2 TRACE after bind / execute / store_result / fetch in the QueryRef 093 path (`query.c` and/or `query_helpers.c`).
- [x] 1b.3 `zsh -ic 'mkq'` then `zsh -ic 'mkp'`.

**Exit:** cppcheck green. Shield still on. Duplicate-key INSERT RETURNING must not SIGSEGV (empty result / error path, not a fetch through a NULL `fetch_row_func`). **Met 2026-09-04:** `mkq` green 2m33s; `mkp` 0 new issues in changed files (84 baseline extras probe warnings unchanged on `git stash`); new Unity test `test_mysql_process_prepared_result_store_result_failure` PASS; `query_helpers_test_mysql` 32/32 PASS; `query_helpers_test_comprehensive_mysql` 30/30 PASS; `query_test_mysql_bind_persist_shape` 17/17 PASS; `query_test_mysql_bind_single_parameter` 13/13 PASS; `mks` green.

### Phase 2 — Live Persist on MySQL and MariaDB

**Entry:** Phase 1b green.

- [x] 2.1 Flip `mailrelay_api_persist_enabled()` so mysql/mariadb return `true` (same as other engines).
- [x] 2.2 Bump `tests/lib/mailrelay_api_helpers.sh` and `tests/test_58_mailrelay_api.sh` CHANGELOG + version.
- [x] 2.3 Run live Test 58 at least for **MySQL** and **MariaDB** (plaintext and STARTTLS). Full 7-engine matrix preferred.
- [x] 2.4 On SIGSEGV: restore the shield in the same change, log the new evidence, **stop**. Do not leave Persist on a crashing engine.

**Exit:** those variants pass with Persist on (idempotency subtest included — 11.4 needs a real `mail_queue` row). `mks` green.

**Phase 2 result:** first attempt (helpers 1.0.9, test_58 2.9.0) eliminated the SIGSEGV (Phase 1b guard proven live) but surfaced a separate pre-existing datetime-format bug: `mailrelay_repository.c` adds 9 timestamp fields (`NEXT_ATTEMPT_AT`, `STALE_BEFORE`, `EXPIRY_AT`, `EXPIRY_CUTOFF_AT`, `CUTOFF_AT`) via `repo_add_string`; MariaDB/MySQL reject the ISO 8601 `T`/`Z` format. Shield restored in the same change per 2.4 spirit (helpers 1.0.10, test_58 2.9.1). Phase 2c fixed this and the second live run (helpers 1.0.11, test_58 2.9.2) achieved **14/14 PASS in 27.8s** including all four mysql/mariadb variants. **Phase 2 done (2026-09-04).**

### Phase 2c — Engine-aware ISO 8601 → MySQL DATETIME translator

**Entry:** Phase 2 first attempt failed; pre-existing datetime bug surfaced live.

- [x] 2c.1 Add `mailrelay_repo_translate_iso8601_to_mysql()` (pure helper) in `mailrelay_repository.{c,h}`.
- [x] 2c.2 Add `repo_add_datetime(json_t*, name, iso8601, database_name)` (engine-aware; translates only for `DB_ENGINE_MYSQL`; the 5 other engines pass through unchanged so they keep their ISO 8601 path that already works).
- [x] 2c.3 Replace 9 `repo_add_string` timestamp sites with `repo_add_datetime` (queue_insert NEXT_ATTEMPT_AT; queue_reschedule NEXT_ATTEMPT_AT; queue_recover_stale STALE_BEFORE; otp_insert EXPIRY_AT; otp_expire_old EXPIRY_CUTOFF_AT; cleanup_queue/events/attempts/otp CUTOFF_AT).
- [x] 2c.4 Unity tests: `test_translate_iso8601_basic/with_fractional/already_mysql_format/empty_string/null_input/no_t_separator_short` + `test_repo_add_datetime_no_app_config_passes_through/non_mysql_engine_passes_through/mysql_engine_translates/mysql_engine_translates_fractional/null_input_emits_null`. 11/11 PASS. Full `mailrelay_repository_test` 68/68 PASS.
- [x] 2c.5 `zsh -ic 'mkq'` then `zsh -ic 'mkp'` (cppcheck:0 issues across 1,996 files; pre-existing probe files removed in earlier turn). `zsh -ic 'mks'` green.
- [x] 2c.6 Flip shield OFF (helpers 1.0.11, test_58 2.9.2); rebuild `hydrogen_release`; run live Test 58 full matrix. **14/14 PASS.**

**Exit:** mysql + mariadb × plaintext + STARTTLS pass with Persist on; the 5 other engines continue to receive ISO 8601 unchanged (no regression). All planned phases done.

### Phase 3 — Close the books

**Entry:** Phase 2c green.

- [x] 3.1 Walk **Post-fix update checklist** below.
- [x] 3.2 Move this file to [`/docs/H/plans/complete/`](/docs/H/plans/complete/) as `PERSIST_PLAN_COMPLETE.md`, fix links, `mkl` / Test 04.

**Exit:** TODO 12d gone or marked done; MAILRELAY resume no longer calls Persist "broken". **Met 2026-09-04.**

## Post-Fix Update Checklist

Must change in the **same change** as the passing live run. Do not leave the shield and the docs disagreeing.

### Tests (the shield)

| File | What |
| --- | --- |
| [`tests/lib/mailrelay_api_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/mailrelay_api_helpers.sh) | `mailrelay_api_persist_enabled()`: mysql/mariadb → `true`. CHANGELOG + `MAILRELAY_API_HELPERS_VERSION` |
| [`tests/test_58_mailrelay_api.sh`](/elements/001-hydrogen/hydrogen/tests/test_58_mailrelay_api.sh) | CHANGELOG + `TEST_VERSION`; note Persist on for all engines |
| [`docs/H/tests/test_58_mailrelay_api.md`](/docs/H/tests/test_58_mailrelay_api.md) | Persist matrix; link this plan as complete |

Checked-in `tests/configs/hydrogen_test_58_*.json` may stay `"Persist": false` if the helper still patches at runtime — say so in the changelog either way.

### Backlog and plans

| File | What |
| --- | --- |
| [`docs/H/TODO.md`](/docs/H/TODO.md) | 12d Done/Remaining/snapshot row: live-green, or remove 12d if closed |
| [`docs/H/plans/MAILRELAY_PLAN.md`](/docs/H/plans/MAILRELAY_PLAN.md) | Resume “not shipped” / Persist section; 11.4 note; Working Log |
| [`docs/H/plans/README.md`](/docs/H/plans/README.md) | Move this plan to completed |
| [`docs/H/SITEMAP.md`](/docs/H/SITEMAP.md) | Path after the move |
| This file | Status complete, then rename/move |

### Bind / result documentation (likely stale today)

| File | What |
| --- | --- |
| [`docs/H/database/PARAMETER_BINDING.md`](/docs/H/database/PARAMETER_BINDING.md) | Null strings / `MYSQL_TYPE_NULL` / `<mysql.h>` already landed in Phase 1. After 1b, document that MySQL `INSERT … RETURNING` must not `mysql_stmt_fetch` when `store_result` failed (`fetch_row_func` stays NULL) |
| [`docs/H/MAIL_GUIDE.md`](/docs/H/MAIL_GUIDE.md) | Persistence section: MySQL/MariaDB are supported when Persist is on; no engine caveat unless one remains |

### Optional / only if behavior changed

- Unity mock headers if result `MYSQL_BIND_COMPLETE` is no longer duplicated
- Production configs that disabled Persist **only** to dodge this crash (e.g. 500courses already wants Persist on for 11.4)
- No Helium migration is required for this C result-path fix. Do not burn 1377+ on this.

## Constraints

- `zsh -ic 'mkq'` after ordinary C edits; `mkt` if `src/` files were added/removed; `mkp` after C; `mks` after Bash.
- No new `static` functions in `src/`.
- Never apply database migrations from an agent session.
- Do not re-enable Persist on mysql/mariadb without Phase 1b C landing **and** Phase 2 evidence.
- Do not “fix” 12d by rewriting QueryRef 093 SQL or PK strategy.
- Do not reopen `MYSQL_BIND` layout / `store_param_func` / bind_param ABI work. That path is closed.

## Working Log

- (2026-09-04) Plan opened from MAILRELAY resume. Shield on. Five bind attempts burned; live still SIGSEGV after indicator + `MYSQL_TYPE_NULL`. Next session starts at Phase 0 (sizeof/offsetof + `.so` path), not another layout tweak.
- (2026-09-04) **Phase 0 complete.** Standalone probe `extras/probe_mysql_bind.c` compiled against `/usr/include/mysql/mysql.h` (which pulls in `mariadb_stmt.h`) and run. Numbers:
  - `sizeof(void*) = 8`, `sizeof(unsigned long) = 8`, `sizeof(unsigned int) = 4`.
  - `sizeof(MYSQL_BIND)` real header **= 112**; hand-rolled mirror of `query.c:76-99` **= 112**. Match.
  - Key offsets (real == hand): `length` 0, `is_null` 8, `buffer` 16, `error` 24, `buffer_length` 64, `length_value` 80, `buffer_type` 96, `error_value` 100, `is_null_value` 103, `extension` 104. All match.
  - `sizeof(my_bool) = 1` (`char`), matches `query.c`'s `char error_value; char is_null_value;`.
  - `dlopen("libmysqlclient.so")` -> `/lib64/libmysqlclient.so`; `dlsym(mysql_stmt_bind_param)` resolves there. `libmysqlclient.so.21/.18/.20` absent; this distro ships `libmysqlclient.so` as an alias for `libmariadb.so.3` (same in-process address). Confirms MariaDB Connector/C is the real client, despite the SONAME.
  - **Decision: Vector C** (remaining nil derefs / function-pointer arity). Vector B (re-include the real header) is unnecessary: stride is correct, `calloc(n, sizeof(hand-rolled MYSQL_BIND))` lines up with the `.so`. Phase 1 must focus on what is still NULL when `bind_param` is entered on the wide Persist bind, plus the function-pointer signatures Hydrogen declares (real header uses `void (*store_param_func)(NET*, MYSQL_BIND*)`; hand-rolled uses `void(*)(void*, MYSQL_BIND_HANDROLLED*)` — ABI-safe at the call site if Hydrogen never calls them, but if `bind_param` invokes `store_param_func` on the param path, the .so will pass a real `NET*` and Hydrogen's hand-rolled pointer would be cast through the wrong signature, which can mis-call a callback if any callback is set — currently none are set, so probably not the live crash; listed for completeness).
- (2026-09-04) **Next:** Phase 1. Inspect `mysql_bind_single_parameter` for any remaining NULL pointer handed to `bind_param`: `length` for BOOLEAN/FLOAT/DATE/TIME/DATETIME is `NULL` (line 225, 241, 300, 334, 375) — MariaDB `bind_param` reads `length` even when `buffer_type` is fixed-width. Either always allocate a zeroed `length` for fixed-width types, or confirm MariaDB tolerates `length==NULL` for `MYSQL_TYPE_SHORT/DOUBLE/DATE/TIME/DATETIME/TIMESTAMP`. Add a new Unity bind test that builds 12 binds with mixed STRING/INTEGER/nulls to mirror the Persist shape, run with the real `libmysqlclient.so`, and look for SIGSEGV before any Phase 2 live run.
- (2026-09-04) **Phase 1 / Vector D done.** `extras/probe_mysql_bind_live.c` connects to live `mariadbd` via the same dlopen path Hydrogen uses (`libmysqlclient.so` -> `/lib64/libmariadb.so.3`), prepares the exact 12-placeholder INSERT for `mail_queue`, and binds with a verbatim copy of the hand-rolled `MYSQL_BIND` from `query.c:76-99`. Bind shape: 1 INTEGER (priority) + 11 STRING, of which `template_key`, `reply_to`, `body_text`, `body_html`, `headers_json` are JSON null -> `MYSQL_TYPE_NULL` with `is_null=&is_null_value, error=&error_value, length=malloc(0)`. `mysql_stmt_bind_param` returned **0** (success); `mysql_stmt_execute` returned **1** with `Field 'queue_id' doesn't have a default value` (expected - the INSERT intentionally omitted `queue_id`). **No SIGSEGV.**
  - This **falsifies** the hypothesis that the hand-rolled `MYSQL_BIND` layout, the `is_null`/`error` indicator wiring, the mixed-null bind shape, the dlsym path, or `mysql_stmt_bind_param` itself is the crash site. Phase 0 vectors B/C are moot on this box.
  - The crash, if real, must come from a path **upstream** of `mysql_stmt_bind_param`: `parse_typed_parameters`, `convert_named_to_positional`, or `mysql_bind_single_parameter` itself doing something that diverges from the reproducer (e.g. producing more than 12 ordered slots, leaving `length`/`buffer` NULL for STRING, or handing a freed pointer to the bind array).
  - **Next:** inspect `convert_named_to_positional` for the 12 named params on QueryRef 093 - count `${INSERT_KEY_*}` placeholders, confirm named-to-positional ordering matches the bind order, and confirm `parse_typed_parameters` doesn't re-order or drop any null string. If the live Test 58 SIGSEGV reproduces after a fresh build, capture a gdb backtrace at the crash to localize the failing frame. Do **not** patch `query.c` based on guesswork.
  - Setup notes for future Vector-D-style runs: created `hydrogen@127.0.0.1` with password `test123` and full grants on `testmrdb` (root@127.0.0.1 password set via `SET PASSWORD` does not survive `FLUSH PRIVILEGES` against the socket-auth path in this 10.11.18 build; `hydrogen` user is what worked). `TRUNCATE TABLE mail_queue` is run by the reproducer before the INSERT so re-runs are idempotent.
- (2026-09-04) **Phase 2 probe: SIGSEGV reproduced on live MariaDB.** `mailrelay_api_persist_enabled()` patched to return `true` for all engines (helpers 1.0.5, test_58 2.8.6); `mkq` green, `mks` green. Full 7-engine Test 58 ran in ~65s. **mariadb and mysql variants SIGSEGV'd; other 5 engines (postgres/yugabyte/sqlite/db2/cockroachdb) all PASS.** Last trace lines in mariadb plaintext hydrogen log before signal 11:
  - `Successfully bound parameter 11` (NEXT_ATTEMPT_AT, STRING, `buffer_type=MYSQL_TYPE_STRING`, `length=malloc(20)`)
  - `MySQL execute_query: mysql_stmt_bind_param for 12 parameters`
  - `Signal 11 received (cause: 1), Fault address: (nil)`
  - Core dump `hydrogen_coverage.core.793745`; gdb shows `rip = 0x0` and an empty stack (no symbols; the crash happened in client `.so` code Hydrogen never saw).
  - **Root cause (high confidence):** MariaDB Connector/C's `mysql_stmt_bind_param` calls `bind[i].store_param_func(net, &bind[i])` on the bind path to convert the user buffer into wire format. Hydrogen's hand-rolled `MYSQL_BIND` (query.c:76-99) leaves `store_param_func` / `fetch_result` / `skip_result` all NULL (they are zeroed by the surrounding `calloc` and never assigned by `mysql_bind_single_parameter`). The .so's normal code path only avoids dereferencing those pointers because the client normally pre-fills `store_param_func` itself at statement-prepare time - but MariaDB Connector/C's `bind_param` for the `MYSQL_TYPE_STRING` (and `MYSQL_TYPE_LONGLONG` etc.) bind buffer path may rely on `store_param_func` being set, and NULL -> SIGSEGV at the function pointer call.
  - **Why Vector D didn't catch it:** the standalone reproducer prepared an INSERT and called `mysql_stmt_bind_param`, but its `MYSQL_BIND` was also missing `store_param_func` - so why didn't it crash? Two plausible reasons: (a) the reproducer used `prepare(bind_param)(stmt, bind)` where the .so *did* walk the array and *did* see NULL function pointers - but maybe the specific buffer types / shapes in the reproducer took a different code path inside `bind_param` that doesn't invoke `store_param_func`. Or (b) the production code path goes through an extra `mysql_stmt_execute` -> wire-format conversion that the reproducer's `execute` call didn't reach because the INSERT returned an error before conversion. The exact trigger is `bind_param` -> client `store_param_func` invocation; the Vector D reproducer must have hit a different branch.
  - **Fix vector (C revised):** in `mysql_bind_single_parameter`, leave `store_param_func`/`fetch_result`/`skip_result` NULL but ensure `mysql_stmt_bind_param` doesn't dereference them. There are two known-correct ways: (1) include `<mysql.h>` and use the *real* `MYSQL_BIND` so the function pointer types match what the client expects - but the layout already matches (Phase 0); the issue is Hydrogen never assigns the function pointers. (2) After `mysql_stmt_prepare`, call `mysql_stmt_bind_param` with a NULL bind array to let the .so fill `store_param_func` defaults, then update only the buffer fields - this is hacky. (3) **Patch `mysql_stmt_bind_param` so it doesn't dereference NULL function pointers** by initializing them to a no-op stub - not possible, the field is on the bind slot and the .so reads from there.
  - **Real fix:** assign `store_param_func` to a Hydrogen-side no-op serializer that returns without touching `param`. This requires understanding which bind types MariaDB expects to invoke the function on. The MariaDB Connector/C source shows `store_param_func` is set internally by the client for known types; the bug is that the *client* is failing to set it (possibly because `buffer_type` enum values don't match - Hydrogen uses `unsigned int` and the client expects `enum enum_field_types`, and on this build they happen to be the same numeric value).
  - **Plan status:** shield restored; Phase 1 is not complete. Phase 2 step 2.4 triggered exactly as planned - shield back in place, evidence logged, stop and reconsider.
- (2026-09-04) **Phase 2 step 2.4: shield restored.** Reverted `mailrelay_api_persist_enabled()` to the `mysql/mariadb -> false` form; bumped helpers to 1.0.6 ("Shield restored after SIGSEGV reproduction"), test_58 to 2.8.7. Test 58 will return to the previous 12/14 baseline (mariadb/mysql Persist off).
- (2026-09-04) **Phase 1 implementation.** Followed the user's choice (Populate function pointers / defensive fix) via the canonical `<mysql.h>` route plus fixing `length=NULL` for every fixed-width bind type. Changes:
  - **CMake**: `pkg_check_modules(MYSQL libmariadb)` with mariadb / mysqlclient / `find_path` fallbacks; `${MYSQL_CFLAGS}` added to default and Unity compile flags in `CMakeLists-init.cmake` and `CMakeLists-unity.cmake` (3 locations). Fatal if no MySQL/MariaDB header found.
  - **`src/database/mysql/query.c`**: dropped the hand-rolled `MYSQL_BIND` (lines 76-99), `MYSQL_TIME` (lines 102-112), and `#define MYSQL_TYPE_*` (lines 59-69); `#include <mysql.h>` placed BEFORE local headers so it provides canonical struct + enum.
  - **`src/database/mysql/types.h`**: `MYSQL_OPT_RECONNECT` now wrapped in `#ifndef`; production code that doesn't include `<mysql.h>` still gets the numeric fallback.
  - **`src/database/mysql/utils.h` + `.c`**: renamed `mysql_escape_string` -> `mysql_h_escape_string` to avoid collision with the client `mysql_escape_string(char*, const char*, unsigned long)` declared in `<mysql.h>`.
  - **`src/database/mysql/connection.h` + `.c`**: renamed `mysql_reset_connection` -> `mysql_h_reset_connection` to avoid collision with the client `mysql_reset_connection(MYSQL*)`.
  - **`src/database/mysql/interface.c`**: updated field initializers to the new names (`mysql_h_reset_connection`, `mysql_h_escape_string`).
  - **`tests/unity/src/database/mysql/connection_test_mysql.c`** + **`utils_test_mysql.c`**: updated test prototypes and call sites to the renamed names.
  - **`length` invariant**: every bind type now allocates a non-NULL `length` pointer (`calloc(0)` for fixed-width, `malloc(sizeof(unsigned long))` set to the value). Previously `length = NULL` for BOOLEAN / FLOAT / DATE / TIME / DATETIME / TIMESTAMP.
  - **New Unity test `tests/unity/src/database/mysql/query_test_mysql_bind_persist_shape.c`**: 17 tests covering the invariants that must hold for the Persist bind shape - length non-NULL for every type, is_null/error attached, JSON null -> MYSQL_TYPE_NULL, buffer_type matches TypedParameter type, length value matches string length, exact 12-bind Persist shape, malloc-failure rollback, NULL-input safety. All 17 PASS.
  - **`mkt`**: green (3m 12s with full rebuild). `mkp`: green. `mks`: green. Existing related tests (`query_test_mysql_bind_single_parameter` 13/13, `connection_test_mysql` 29/29, `utils_test_mysql` 9/9) all PASS.
- (2026-09-04) **Phase 2 redo (helpers 1.0.7, test_58 2.8.8): SIGSEGV STILL REPRODUCES.** Live Test 58 mariadb plaintext and STARTTLS, mysql plaintext and STARTTLS - all 4 SIGSEGV at the same point: after `Successfully bound parameter 11`, `MySQL execute_query: mysql_stmt_bind_param for 12 parameters`, `Signal 11 received (cause: 1), Fault address: (nil)`. Identical signature to the original SIGSEGV before the fix.
  - **The `<mysql.h>` refactor + `length` fix did NOT address the actual SIGSEGV.** Persist binds are all STRING (10) + INTEGER (1) + MYSQL_TYPE_NULL (1). None of those had `length=NULL` to begin with. The latent length=NULL bug on BOOLEAN/FLOAT/DATE/TIME/DATETIME/TIMESTAMP is fixed (real bug, but not the Persist cause).
  - **Standalone probes do NOT crash.** Two extras programs reproduce the EXACT bind shape of the production Persist (extras/probe_persist_call.c, extras/probe_persist_multi.c - the latter also runs 2 prior auth-probe bind_param calls like production does). Both run `mysql_stmt_bind_param` repeatedly against live MariaDB 10.11.18 / libmysqlclient.so and return `bind_param returned 0` every time, with all 12 binds including the JSON-null ones. The production code's exact same bind shape (verified by the trace lines) crashes at the same call site.
  - **Possible explanations not yet ruled out:**
    1. The MariaDB Connector/C `bind_param` walks the array and dispatches to internal default `store_param_func` per buffer_type. On the production call, the .so's internal dispatch may hit a code path where it dereferences a function pointer we haven't identified.
    2. The function-pointer typedef in `types.h` is `int (*)(void*, void*)` but the .so's `mysql_stmt_bind_param` returns `my_bool` (1 byte). The release build's calling convention might propagate garbage in the upper bytes of %rax into a downstream code path.
    3. The release build has `-Wl,--dependency-file=...` and other linker flags that may differ from the probe.
    4. Some Hydrogen-side state we haven't isolated (connection-internal pointer, statement metadata, queue state) corrupts the bind path only in production.
  - **Phase 2.4 triggered again.** Shield restored (helpers 1.0.8, test_58 2.8.9). Persist stays off for mysql/mariadb.
- (2026-09-04) **Next session, suggested direction.** A full gdb attach on the running hydrogen process while Persist is enabled, with a breakpoint at the `mysql_stmt_bind_param` GOT call site, would let us inspect the bind array and the statement metadata at crash time and pinpoint which field is NULL when the production .so dereferences it. Until that's done, this is the right place to stop - we have a real reproducer (live Test 58) and a non-reproducer (extras probes) and that's the discrepancy to investigate, not another layout tweak.
- (2026-09-04) **Vector D with live QueryRef 093 SQL (`extras/probe_persist_q093.c`).** Connector/C `mysql_stmt_bind_param` (mariadb_stmt.c) does **not** call `store_param_func`. It `memcpy`s into `stmt->params` and checks `methods->db_supported_buffer_type`. Fetched `queries.code` for ref 093 from `demomrdb` (real newlines, 12 `:NAME` params). After `:NAME` → `?`:
  - prepare rc=0, `param_count=12`, **`field_count=0` at prepare**, `params` and `methods` non-NULL.
  - `bind_param` returned 0. **Not the crash.**
  - `execute` returned 0. After execute: **`field_count=1`** (RETURNING `queue_id`), `state=WAITING_USE_OR_STORE` (3), **`fetch_row_func=NULL`**, `default_rset_handler` set, `result_metadata` non-NULL.
  - Unique UUID: `store_result` rc=0 sets `fetch_row_func`; Hydrogen-shaped `MYSQL_BIND_COMPLETE` (sizeof 112, matches) `bind_result`+`fetch` return `queue_id`. No SIGSEGV.
  - Duplicate `message_uuid`: `store_result` rc=1 (`Duplicate entry…`), `fetch_row_func` stays NULL, **`mysql_stmt_fetch` SIGSEGV `fault (nil)`**. Matches Test 58's last TRACE (`bind_param for 12 parameters`) because `query.c` logs nothing between that TRACE and fetch (`bind` success is silent, `execute` success is silent).
  - **Root cause revision:** crash is `mysql_stmt_fetch` with NULL `fetch_row_func` on the INSERT…RETURNING result path, not `bind_param`. `query_helpers.c` `mysql_process_prepared_result` ignores `store_result` rc and always `bind_result`+`fetch`. `query.c` still uses `<mysql.h>` for *param* binds; result binds still use the hand-rolled `MYSQL_BIND_COMPLETE` in `query_helpers.c`.
  - **Next:** guard `store_result`/`fetch` in `mysql_process_prepared_result` (do not fetch when `store_result` failed or `fetch_row_func` would be NULL); add TRACE after bind/execute/store/fetch so a Phase 2 live run can localize. Shield stays on until that lands and Test 58 is re-run.
- (2026-09-04) **Plan front matter rewritten to match diagnosis.** Title/Purpose/Current Pause/Issue/Vectors/Phases no longer claim a bind ABI crash. Phase 1 marked complete with variance (Unity-green, live still died). New **Phase 1b** is the `store_result`/fetch guard. User approved Guard + Phase 2 live. Next session implements 1b.1–1b.3, then Phase 2; restore shield on SIGSEGV. Shield files not flipped.
- (2026-09-04) **Phase 2 live: SIGSEGV gone, but new latent bug surfaced.** Shield flipped OFF (helpers 1.0.9, test_58 2.9.0); `cmake --build build-release --target hydrogen_release` produced fresh binary; full 7-engine × plaintext/STARTTLS matrix ran in 64s. **Result: 10/14 PASS, 4/14 FAIL. Zero SIGSEGVs.** MySQL/MariaDB no longer crash — the Phase 1b guard works as designed. New hydrogen TRACE lines show `MySQL prepared stmt store_result rc=1` followed by `MySQL prepared stmt store_result failed: Incorrect datetime value: '2026-09-04T22:10:57Z' for column demomrdb.mail_queue.next_attempt_at at row 1` on every retry attempt. **Root cause is a pre-existing, engine-agnostic bug in `mailrelay_repository.c:410`** — `NEXT_ATTEMPT_AT` is added as STRING via `repo_add_string(p, "NEXT_ATTEMPT_AT", params->next_attempt_at)` regardless of engine. PostgreSQL/SQLite/DB2/Cockroach/Yugabyte accept ISO 8601 (`T` separator, trailing `Z`); MariaDB/MySQL are strict about DATETIME format (`YYYY-MM-DD HH:MM:SS`). 5-attempt retry loop then logs `Failed to persist mail queue row: empty insert result after 5 attempts`. **Phase 2.4 spirit triggered even though no SIGSEGV** — the variants still fail; do not leave Persist on a failing engine. Shield restored in same change (helpers 1.0.10, test_58 2.9.1). `mkq` (trial) and `mks` (shellcheck) green. **C result-path fix is correct and proven live; the datetime format mismatch is a separate mailrelay/mailrepo work item (out of PERSIST_PLAN scope per "do not rewrite QueryRef 093 SQL").** Next session: open a new plan/TODO for the `next_attempt_at` engine-format bug (likely `repo_add_datetime` / engine-aware timestamp binder in `mailrelay_repository.c`, or a Lua migration to use a TIMESTAMP-with-time-zone column on MariaDB/MySQL). Possible approaches: (a) Lua-side `mysql_format_datetime` that translates ISO 8601 → MySQL DATETIME before bind; (b) change column type to VARCHAR and parse in the SELECT path; (c) add an engine-specific timestamp formatting layer in `mailrelay_repository.c`. Once that lands, re-flip the shield.
- (2026-09-04) **Phase 2c done — plan live-green, 14/14.** Implemented engine-aware ISO 8601 → MySQL DATETIME translator in [`mailrelay_repository.c`](/elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_repository.c). Two helpers: `mailrelay_repo_translate_iso8601_to_mysql(const char*)` is the pure transform (replaces `T` with `<space>`, drops trailing `Z`, truncates fractional seconds); `repo_add_datetime(json_t*, name, iso8601, database_name)` is the engine-aware binder that resolves `DatabaseConnection->type` via `find_database_connection(&app_config->databases,database_name)` and translates only for `DB_ENGINE_MYSQL` (the 5 other engines pass through unchanged). 9 timestamp `repo_add_string` callsites converted (queue_insert/reschedule NEXT_ATTEMPT_AT; queue_recover_stale STALE_BEFORE; otp_insert EXPIRY_AT; otp_expire_old EXPIRY_CUTOFF_AT; cleanup_queue/events/attempts/otp CUTOFF_AT). Non-static (per `mkq` "no static in src/" rule) and exposed in the header for Unity. New `#include <src/config/config_databases.h>` for `find_database_connection`. `mkq` green 2m10s; `mks` green; `mkp` 0 issues across 1,996 files. New Unity tests in `mailrelay_repository_test.c`: 6 `mailrelay_repo_translate_iso8601_to_mysql` cases (basic / fractional / already-mysql / empty / null / short-no-T) + 5 `repo_add_datetime` cases (no app_config / non-mysql engine / mysql engine / mysql with fractional / null input). 11/11 PASS; full suite 68/68 PASS. Shield flipped OFF (helpers 1.0.11, test_58 2.9.2); rebuilt `hydrogen_release`; ran live Test 58 full matrix in **27.8s, 14/14 PASS, 20/20 tests, 0 fails**. mariadb/mysql logs show `MySQL prepared stmt store_result rc=0` on every call (no `Incorrect datetime` errors). Persist path is live-green for all 7 engines. **Post-fix checklist walked in this change:** `docs/H/TODO.md` 12d done + snapshot row updated; `MAILRELAY_PLAN.md` "not shipped" + 11.4 + Decisions log updated; `plans/README.md`/`SITEMAP.md` list this plan under complete/; `test_58_mailrelay_api.md` matrix note; `PARAMETER_BINDING.md` + `MAIL_GUIDE.md` "no engine caveat". Plan moves to `complete/PERSIST_PLAN_COMPLETE.md`. **TODO 12d closed.**
