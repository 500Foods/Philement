# Mail Relay Persist — MySQL/MariaDB Bind SIGSEGV

## Purpose

Fix `Queue.Persist` on **MySQL and MariaDB** so QueryRef **093** (insert pending `mail_queue` row) no longer SIGSEGVs inside `mysql_stmt_bind_param`. Until that is live-green, those engines keep Persist **off** in Test 58.

This is Hydrogen TODO **12d**. It is **not** Mail Relay product work (templates, API, OTP, HA claim). It is the MySQL prepared-statement bind ABI.

Parent: [MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md). Distinct from TODO **12e** (MAX+1 PK race). Empty-id retry in `mailrelay_persist_message` already exists; do not invent UUID primary keys.

## How To Use This Document

- Work one phase at a time. Do not re-enable Persist as a “try it” while an ABI hypothesis is unproven.
- Unity-green is **not** the exit gate. Live `test_58` MySQL **and** MariaDB with Persist on is the gate.
- Record each failed live run in the Working Log with the exact hypothesis, so the next session does not repeat it.
- When the live gate is green, walk **Post-fix update checklist** in the same change, then move this file to `complete/`.

## Current Pause (2026-09-04)

| | |
| --- | --- |
| **Symptom** | Hydrogen dies in MariaDB `mysql_stmt_bind_param`; `fault (nil)` |
| **Trigger** | `MailRelay.Queue.Persist=true` on mysql/mariadb → QueryRef 093 |
| **Shield** | `mailrelay_api_persist_enabled()` returns `false` for `mysql` and `mariadb` (`tests/lib/mailrelay_api_helpers.sh` **1.0.4**; `test_58` **2.8.5**) |
| **Other engines** | Persist **on** via the Test 58 jq runtime patch (checked-in `hydrogen_test_58_*.json` still say `"Persist": false`; the helper overwrites) |
| **Last live result** | After indicator-pointer + `MYSQL_TYPE_NULL` work, Test 58 MySQL/MariaDB **still** SIGSEGV → shield put back the same day |
| **Do not trust** | TODO 12d “Persist enabled for all engines” / older MAILRELAY pause “enabled; live matrix not run” — those are the gap between the Unity-green bind patch and the re-crash |

## The Issue

### Call path

1. `mailrelay_enqueue` / `mailrelay_persist_message` (`src/mailrelay/mailrelay.c`)
2. `mailrelay_repo_queue_insert` (`src/mailrelay/mailrelay_repository.c`) — 12 named params
3. QueryRef **093** (`acuranzo_1223.lua`) — `INSERT` into `mail_queue` with `${INSERT_KEY_*}`
4. MySQL engine `mysql_bind_single_parameter` then `mysql_stmt_bind_param` (`src/database/mysql/query.c`)
5. Crash **inside the client `.so`**, after Hydrogen TRACE has logged all 12 binds

Params on 093: `MESSAGE_UUID`, `PRIORITY`, `TEMPLATE_KEY`, `FROM_ADDR`, `REPLY_TO`, `RECIPIENTS_JSON`, `SUBJECT`, `BODY_TEXT`, `BODY_HTML`, `HEADERS_JSON`, `IDEMPOTENCY_KEY`, `NEXT_ATTEMPT_AT`. Optional strings are often JSON `null` because `repo_add_string(..., NULL)` writes `null`.

### Why Persist dies and `/api/auth/login` often does not

`repo_add_string` with a NULL C pointer → JSON `null` → `TypedParameter.is_null`. QueryRef 093 commonly has several optional NULLs (`TEMPLATE_KEY`, `REPLY_TO`, `BODY_HTML`, `HEADERS_JSON`, …). Typical auth queries are a few non-null INTEGER/STRING binds. The crashing statement is **wide + mixed nulls**.

This is **not** a Mail Relay queue/worker logic bug. PG/SQLite/DB2/Cockroach/Yugabyte Persist paths already run in Test 58.

### Why Hydrogen is ABI-fragile here

- Production code **does not include** `mysql.h`. `MYSQL_BIND` is hand-rolled in `query.c` and the client is `dlsym`’d (`mysql_stmt_bind_param_ptr` in `src/database/mysql/connection.c`).
- Official MariaDB layout: `/usr/include/mysql/mariadb_stmt.h` (`struct st_mysql_bind`). Oracle libmysql `MYSQL_BIND` is not the same struct across vendors/versions.
- `calloc(n, sizeof(hand-rolled MYSQL_BIND))` must match the **library** stride. If it does not, `bind_param` walks garbage after slot 0 even when every Hydrogen bind “succeeded”.
- MariaDB `bind_param` **dereferences** `is_null` and `error`. NULL pointers → SIGSEGV (`fault (nil)`). That class was patched; live still crashes, so another nil deref or a misaligned field-as-pointer remains.

## Attempts Already Burned

Do **not** present these as new work.

| # | Attempt | Result |
| --- | --- | --- |
| 1 | Shield Persist off mysql/mariadb | Workaround **still in place**. Not a fix. |
| 2 | INTEGER `MYSQL_TYPE_LONG` → `MYSQL_TYPE_LONGLONG` for 8-byte `long long` | Necessary. Not sufficient. |
| 3 | Hand-rolled `MYSQL_BIND` nudged toward MariaDB (`error` as `my_bool*`, `flags`, function-pointer arity) | Layout still duplicated. No `sizeof` proof vs the `.so`. |
| 4 | `is_null`/`error` were NULL; now `mysql_bind_attach_indicators()` points at `is_null_value`/`error_value`. SQL NULL → `MYSQL_TYPE_NULL` | Unity bind tests + `mkq`/`mkp` green. |
| 5 | Re-run live Test 58 after (4) | **Still SIGSEGV.** Shield restored (helpers 1.0.4 / test_58 2.8.5). “Confirm live” TODO text is from between (4) and (5). |

Also out of scope as a “fix”:

- Skipping MySQL/MariaDB in Test 58 forever
- Serializing tests to hide 12e races
- Changing MAX+1 / `INSERT_KEY_*` to UUIDs or `${SERIAL}`
- Treating `mku query_test_mysql_bind_single_parameter` as the production gate

## Fix Vectors (try in this order)

Hypothesis A is the cheap proof. Do not start C layout guessing until A is measured.

### A — Prove the ABI (do this first)

1. Print `sizeof(MYSQL_BIND)` and `offsetof` for `is_null`, `error`, `buffer_type`, `is_null_value` from the **real** client header (`mariadb_stmt.h` / `mysql.h`).
2. Print the same for the **hand-rolled** struct in `query.c` (temporary instrumentation or a tiny standalone that copies the typedef).
3. Resolve which `.so` `dlsym("mysql_stmt_bind_param")` loads (`readlink` / `dladdr` / `LD_DEBUG=bindings`). MariaDB Connector/C vs Oracle libmysql vs distro `libmysqlclient`.
4. If sizes or key offsets differ → stop guessing fields; the array stride is wrong and every later bind is garbage.

**Done means:** numbers in the Working Log (header sizeof, hand-rolled sizeof, `.so` path). No Persist re-enable yet.

### B — Stop hand-rolling the struct (preferred if A mismatches)

Include the real client `MYSQL_BIND` (or a checked-in layout compiled against the same header the `.so` was built from). Keep `dlsym` for functions if that is still the load model; the struct must come from the header, not a comment that says “simplified version to match libmysqlclient”.

Watch Unity: `tests/unity/mocks/mock_libmysqlclient.*` and tests that assume the hand-rolled type. A new `src/` file needs `mkt` (CMake glob). No new `static` functions in `src/`.

**Done means:** one struct definition, `mkq`/`mkp` green, bind Unity green. Still no Persist re-enable until Phase 3.

### C — Remaining nil derefs (only if A matches)

If sizeof/offsetof **match** and live still dies:

- BOOLEAN/FLOAT still set `length = NULL` (093 is STRING+INTEGER, so unlikely this crash; do not “fix” blindly).
- `store_param_func` / `fetch_result` left NULL — confirm whether this client’s `bind_param` calls them on the param path.
- `my_bool` width (`char` vs `int`) if the header and hand-roll disagree even when sizeof accidentally matches via padding.

### D — Reproduce without Mail Relay (optional, cheaper than full Test 58)

A Unity or extras program that prepares a 12-placeholder `INSERT` with mixed STRING + INTEGER + JSON nulls against live MySQL/MariaDB. If that SIGSEGVs, Mail Relay is out of the debug loop.

## Phases

### Phase 0 — ABI measurement

**Entry:** this document read; shield left **on**.

- [ ] 0.1 Record header vs hand-rolled `sizeof`/`offsetof` and the loaded `.so`.
- [ ] 0.2 Append numbers to Working Log. Choose vector B or C.

**Exit:** Working Log has measurements. No Persist flag change.

### Phase 1 — Bind ABI fix

**Entry:** Phase 0 identified B or C.

- [ ] 1.1 Implement the chosen vector in `src/database/mysql/query.c` (and mocks/tests as needed).
- [ ] 1.2 `zsh -ic 'mkq'` then `zsh -ic 'mkp'`. If a new `.c` was added: `mkt` first.
- [ ] 1.3 `mku query_test_mysql_bind_single_parameter` (and any new bind test). Add a case that binds **several JSON nulls plus an integer** if none exists — that is the Persist shape.

**Exit:** lint + Unity green. Shield still on.

### Phase 2 — Live Persist on MySQL and MariaDB

**Entry:** Phase 1 green.

- [ ] 2.1 Flip `mailrelay_api_persist_enabled()` so mysql/mariadb return `true` (same as other engines).
- [ ] 2.2 Bump `tests/lib/mailrelay_api_helpers.sh` and `tests/test_58_mailrelay_api.sh` CHANGELOG + version.
- [ ] 2.3 Run live Test 58 at least for **MySQL** and **MariaDB** (plaintext and STARTTLS). Full 7-engine matrix preferred.
- [ ] 2.4 On SIGSEGV: restore the shield in the same change, log the new evidence, **stop**. Do not leave Persist on a crashing engine.

**Exit:** those variants pass with Persist on (idempotency subtest included — 11.4 needs a real `mail_queue` row). `mks` green.

### Phase 3 — Close the books

**Entry:** Phase 2 live-green.

- [ ] 3.1 Walk **Post-fix update checklist** below.
- [ ] 3.2 Move this file to [`/docs/H/plans/complete/`](/docs/H/plans/complete/) as `PERSIST_PLAN_COMPLETE.md`, fix links, `mkl` / Test 04.

**Exit:** TODO 12d gone or marked done; MAILRELAY resume no longer calls Persist “broken”.

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

### Bind documentation (likely stale today)

| File | What |
| --- | --- |
| [`docs/H/database/PARAMETER_BINDING.md`](/docs/H/database/PARAMETER_BINDING.md) | Still says null strings bind as empty text when `is_null` cannot be set. After a real ABI fix, document `MYSQL_TYPE_NULL` + indicator pointers + “struct comes from client header” if that landed |
| [`docs/H/MAIL_GUIDE.md`](/docs/H/MAIL_GUIDE.md) | Persistence section: MySQL/MariaDB are supported when Persist is on; no engine caveat unless one remains |

### Optional / only if behavior changed

- Unity mock headers if `MYSQL_BIND` is no longer duplicated
- Production configs that disabled Persist **only** to dodge this crash (e.g. 500courses already wants Persist on for 11.4)
- No Helium migration is required for a C ABI fix. Do not burn 1377+ on this.

## Constraints

- `zsh -ic 'mkq'` after ordinary C edits; `mkt` if `src/` files were added/removed; `mkp` after C; `mks` after Bash.
- No new `static` functions in `src/`.
- Never apply database migrations from an agent session.
- Do not re-enable Persist on mysql/mariadb without Phase 2 evidence.
- Do not “fix” 12d by rewriting QueryRef 093 SQL or PK strategy.

## Working Log

- (2026-09-04) Plan opened from MAILRELAY resume. Shield on. Five bind attempts burned; live still SIGSEGV after indicator + `MYSQL_TYPE_NULL`. Next session starts at Phase 0 (sizeof/offsetof + `.so` path), not another layout tweak.
