# Test 41: Memory Exercise ASAN

## Overview

[`test_41_exercise_asan.sh`](/elements/001-hydrogen/hydrogen/tests/test_41_exercise_asan.sh) runs multi-engine authentication under **AddressSanitizer / LeakSanitizer**. A single `hydrogen_debug` instance is started with six database engines (YugabyteDB disabled), then 500 concurrent auth requests exercise DB-heavy login paths. After shutdown, the server log is scanned for LSAN direct/indirect leaks.

Native RSS measurement is a separate test: [test_44_exercise_native.md](/docs/H/tests/test_44_exercise_native.md). The suite runs 41 and 44 in parallel (ports **5410** and **5444**).

## Purpose

- Exercise concurrent multi-engine auth under ASAN
- Authoritative leak detection via LeakSanitizer on DB queue / auth paths
- Complement `test_11` (ASAN without DBs) and `test_44` (native RSS)

## Configuration

| Field | Value |
|-------|--------|
| Test Name | Exercise ASAN |
| Abbreviation | EXA |
| Number | 41 |
| Version | 4.0.0 |
| Binary | `hydrogen_debug` (required) |
| Port | 5410 |
| Requests | 500 |
| Concurrency | 28 default (`CONCURRENCY` env) |
| Config | `hydrogen_test_41_exercise.json` |

### Engines

Six enabled: PostgreSQL, MySQL, SQLite, DB2, MariaDB, CockroachDB. **YugabyteDB** is present but `Enabled: false` (covered by tests 38/40; remote migrate too slow for exercise).

### Shared library

[`lib/exercise_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/exercise_helpers.sh): `scrape_metrics`, `get_metric`, `run_auth_request`, `run_auth_batch`.

## Flow

1. Locate binaries; require ASAN-capable `hydrogen_debug`
2. Validate env + config; report **enabled** DB count (not connection array length)
3. Start server with `ASAN_OPTIONS=detect_leaks=1:leak_check_at_exit=1:...`
4. Ready-list from migration markers; 500 concurrent auth batches
5. RSS snapshots (diagnostic only under ASAN)
6. Shutdown; LSAN scan of server log

## Success criteria

- ASAN binary available and server starts
- At least one DB ready
- 500 auth requests complete
- **Direct=0 and Indirect=0** from LeakSanitizer
- Clean shutdown

RSS growth under ASAN is expected (shadow/quarantine) and does **not** fail the test.

## Related

- [test_44_exercise_native.md](/docs/H/tests/test_44_exercise_native.md)
- [test_11_leaks_like_a_sieve.md](/docs/H/tests/test_11_leaks_like_a_sieve.md)
- [TESTING.md](/docs/H/tests/TESTING.md)

## Changelog (script)

```text
# 4.0.0 - 2026-07-09 - Split from combined exercise:
#                     - ASAN-only test (native moved to test_44)
#                     - Renamed test_41_exercise.sh -> test_41_exercise_asan.sh
#                     - Shared lib/exercise_helpers.sh; CONCURRENCY default 28
#                     - Enabled-DB count in config validation; fail if no ASAN binary
# 3.14.0 and earlier - see git history of pre-split test_41_exercise.sh
```
