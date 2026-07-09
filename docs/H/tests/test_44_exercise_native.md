# Test 44: Memory Exercise Native

## Overview

[`test_44_exercise_native.sh`](/elements/001-hydrogen/hydrogen/tests/test_44_exercise_native.sh) runs the long **native (non-ASAN) RSS** multi-engine auth exercise. A single `hydrogen_release` instance uses six database engines (YugabyteDB disabled) and issues **5000** concurrent authentication requests while scraping Prometheus for steady-state memory growth.

ASAN / LeakSanitizer coverage is [test_41_exercise.md](/docs/H/tests/test_41_exercise.md). Different ports allow **suite-parallel** execution with test 41.

## Purpose

- Authoritative RSS growth measurement (no ASAN shadow/quarantine noise)
- Sustained multi-engine auth load for leak heuristics
- Server-health under load (abort if Prometheus stops responding)

## Configuration

| Field | Value |
|-------|--------|
| Test Name | Exercise Native |
| Abbreviation | EXN |
| Number | 44 |
| Version | 1.0.0 |
| Binary | `hydrogen_release` (required, non-ASAN) |
| Port | **5444** |
| Requests | 5000 |
| Concurrency | 50 default (`CONCURRENCY` env) |
| Snapshot interval | every 500 completed requests |
| Leak threshold | 1 KB/request steady-state |
| Config | `hydrogen_test_44_exercise_native.json` |
| Log level | `HYDROGEN_LOG_LEVEL=STATE` during run |

### Engines

Same six enabled engines as test 41. YugabyteDB entry present but disabled.

### Shared library

[`lib/exercise_helpers.sh`](/elements/001-hydrogen/hydrogen/tests/lib/exercise_helpers.sh)

## Flow

1. Locate `hydrogen_release` (reject ASAN-instrumented binaries)
2. Start multi-DB server on port 5444
3. 5000 concurrent auth batches (round-robin DBs, even/odd valid/invalid password)
4. Snapshots ~every 500 requests; abort after 2 consecutive scrape failures
5. Steady-state analysis: second half growth vs 1 KB/req threshold
6. Footer title: `growth: N B/req` when analysis succeeds

## Success criteria

- Release binary available and server starts
- At least one DB ready
- Server stays responsive through the run
- Steady-state growth ≤ 1 KB/request

Sub-page rates (e.g. ~10 B/req) are normal RSS noise after warmup.

## Related

- [test_41_exercise.md](/docs/H/tests/test_41_exercise.md)
- [test_11_leaks_like_a_sieve.md](/docs/H/tests/test_11_leaks_like_a_sieve.md)
- [TESTING.md](/docs/H/tests/TESTING.md)

## Changelog (script)

```text
# 1.0.0 - 2026-07-09 - Split from combined test_41:
#                     - Native-only 5000-request RSS exercise on port 5444
#                     - CONCURRENCY default 50; suite-parallel with ASAN test 41
```
