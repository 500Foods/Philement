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
| Version | 1.2.0 |
| Binary | `hydrogen_release` (required, non-ASAN) |
| Port | **5444** |
| Requests | 5000 |
| Concurrency | 50 default (`CONCURRENCY` env) |
| Snapshot interval | every 500 completed requests |
| Leak threshold | 1 KB/request steady-state RSS |
| Heap monitor | on (`EXERCISE_NATIVE_HEAPMON=0` skips the preload) |
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
6. Heap analysis of that same window: mapping RSS, glibc in-use, and callers that still hold live bytes
7. Footer title: `growth: N B/req` when analysis succeeds

## Heap analysis

RSS says the process got larger. It does not say whether those pages are still live objects. At the midpoint and at the end the test records two snapshots:

- `/proc/<pid>/smaps`, summed by mapping, so the report can name where resident pages moved (`[heap]`, anonymous mappings, a specific library).
- A preload ([`heapmon.c`](/elements/001-hydrogen/hydrogen/tests/heapmon/heapmon.c)) that counts public `malloc` / `free` / `realloc` / `strdup` callers. The report lists callers whose **still-live** bytes grew in the second half, plus glibc `mallinfo2` in-use and free totals.

The pass/fail number stays the RSS slope. The heap section is the explanation.

Read it this way:

- Live glibc bytes flat while RSS is not: the slope is retained address space (free pages the allocator kept, or a mapping that is not a public `malloc`). The mapping list is the where. Callers that grew by a much smaller amount are leftovers, not the slope.
- Live glibc bytes grew and the same callers grew: those callers allocated and had not freed by the end of the run. Lifetime alloc/free counts are on each line.
- Live glibc bytes grew and the tracked callers did not: the bytes were allocated inside libc or by a private arena that does not call public `malloc`.

`hydrogen_release` is stripped, so a site inside Hydrogen prints as `hydrogen_release+offset`. Shared libraries keep their exported names. `EXERCISE_NATIVE_DIAG=1` runs `hydrogen_perf` (`-g -rdynamic`) and names Hydrogen functions. That binary is not the packed release build, so its RSS figure is not the release number.

The preload's own tables are faulted in at startup. They add a constant to warmup RSS and are not part of the per-request slope. `EXERCISE_NATIVE_HEAPMON=0` skips the preload; the mapping delta still runs.

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
# 1.2.0 - 2026-09-23 - Heap analysis on the steady-state half: smaps mapping
#                     delta, glibc in-use versus RSS, and callers that still
#                     hold live bytes. EXERCISE_NATIVE_HEAPMON=0 skips the preload.
# 1.0.0 - 2026-07-09 - Split from combined test_41:
#                     - Native-only 5000-request RSS exercise on port 5444
#                     - CONCURRENCY default 50; suite-parallel with ASAN test 41
```
