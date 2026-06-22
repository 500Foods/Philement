# Test 41: Memory Exercise - Single Server Auth Stress Test

## Overview

The [`test_41_exercise.sh`](/elements/001-hydrogen/hydrogen/tests/test_41_exercise.sh) script provides comprehensive memory stress testing for the Hydrogen server, focusing on exercising database-heavy authentication paths while monitoring for memory leaks. This test launches a single Hydrogen instance with all 7 database engines configured simultaneously and performs 500 authentication requests across them.

## Purpose

This test validates that the Hydrogen server:

- Correctly handles concurrent database connections across all 7 supported engines
- Maintains stable memory usage during sustained authentication load
- Properly cleans up resources (database queries, connections, JWT tokens)
- Survives extended exercise without memory growth indicative of leaks
- Integrates with AddressSanitizer (ASAN) when available for definitive leak detection

This complements `test_11_leaks_like_a_sieve.sh`, which performs leak detection but deliberately avoids database usage to isolate pure infrastructure memory issues.

## Test Configuration

- **Test Name**: Exercise
- **Test Abbreviation**: EXE
- **Test Number**: 41
- **Version**: 3.8.0

The footer title reports the native steady-state growth rate measured during the long 5,000-request native run, for example `41-EXE | Exercise  Growth: 1,512 B/req` (the growth segment is colorized).

The long native (hydrogen_release) measurement runs as a single subtest ("Native RSS Measurement"): a TEST header, INFO progress lines every 500 requests, and a single terminal PASS/FAIL.

## Key Features

### Exercise Parameters

- **Total Requests**: 500 authentication requests (configurable via `TOTAL_REQUESTS`)
- **Snapshot Interval**: Every 50 requests (configurable via `SNAPSHOT_INTERVAL`)
- **Metrics Delay**: 1 second between scrapes (configurable via `METRICS_DELAY`)
- **Leak Threshold**: 1 KB per request in steady-state (configurable via `LEAK_THRESHOLD_KB_PER_REQ`)

### Database Engines Tested

All 7 database engines are configured and exercised in a single Hydrogen instance:

| Engine | Database Name | Port | Notes |
|--------|---------------|------|-------|
| PostgreSQL | Demo_PG | 5410 | Full parameter support |
| MySQL | Demo_MY | 5410 | Full parameter support |
| SQLite | Demo_SQL | 5410 | No connection pooling |
| DB2 | Demo_DB2 | 5410 | Enterprise database |
| MariaDB | Demo_MR | 5410 | MySQL-compatible |
| CockroachDB | Demo_CR | 5410 | Distributed SQL |
| YugabyteDB | Demo_YB | 5410 | Distributed SQL |

### Authentication Mix

Requests cycle round-robin across ready databases with alternating credentials:

- **Even-numbered requests**: Valid login (correct password)
- **Odd-numbered requests**: Invalid login (wrong password)

This tests both success and failure paths of the authentication system.

## Leak Detection Methods

### RSS Growth Heuristic (Primary)

When no ASAN build is available, the test monitors Resident Set Size (RSS) memory growth:

1. **Warmup Phase**: First half of requests (initializes caches, JIT compilation)
2. **Steady-State Phase**: Second half of requests (cache-warmed state)
3. **Analysis**: Compares memory growth rate against threshold

The test expects steady-state memory growth of less than 1 KB per request. Growth above this threshold suggests unbounded memory allocation.

### ASAN LeakSanitizer (Secondary)

When `hydrogen_debug` (ASAN-enabled build) is found, the test:

1. Launches the exercise server under ASAN with `detect_leaks=1`
2. Captures stderr to the server log for leak analysis
3. Scans the log post-shutdown for `Direct leak` and `Indirect leak` reports
4. Reports exact leak counts if found

## Library Dependencies

The test uses modular libraries from the `lib/` directory:

- **`conduit_utils.sh`**: Server lifecycle management (`run_conduit_server`, `shutdown_conduit_server`)
- **`framework.sh`**: Test framework, result tracking, logging utilities

## Configuration Files

- **`hydrogen_test_41_exercise.json`**: Single configuration enabling all 7 database engines with the demo Acuranzo schema

## Required Prerequisites

### Environment Variables

- **`HYDROGEN_DEMO_USER_NAME`**: Demo user login name (must exist in demo schema)
- **`HYDROGEN_DEMO_USER_PASS`**: Demo user password
- **`HYDROGEN_DEMO_API_KEY`**: API key for authentication requests

### External Tools

- **`jq`**: JSON parsing for responses and metrics extraction
- **`sqlite3`**: For inspecting the demo database (optional, for verification)
- **`node`**: For mock Keycloak (used indirectly via configuration)

### Database Availability

All 7 database engines must be running and accessible. The test gracefully degrades when individual databases are unavailable.

## Test Flow

### Phase 1: Setup and Validation

1. **Locate Hydrogen Binary**: Find the appropriate binary (release, debug, or coverage build)
2. **Detect ASAN Build**: Check for `hydrogen_debug` with embedded payload
3. **Validate Environment**: Ensure all required env vars are set
4. **Validate Configuration**: Parse JSON config, extract port and database count

### Phase 2: Server Startup

1. **Launch Hydrogen**: Start with multi-database configuration
2. **Wait for Ready**: Poll for `STARTUP COMPLETE` in logs
3. **Database Migration**: Wait for QTC bootstrap to complete
4. **Collect Ready Databases**: Identify which databases successfully migrated

### Phase 3: Exercise Execution

1. **Initial Metrics**: Capture baseline RSS, queries, connections, FDs
2. **Run Auth Loop**: 500 requests cycling across ready databases
3. **Periodic Snapshots**: Every 50 requests, scrape Prometheus metrics
4. **Log Progress**: Report RSS, query count, and FD changes

### Phase 4: Analysis and Cleanup

1. **Final Metrics**: Capture end-state metrics
2. **Leak Analysis**: Compare steady-state growth rate
3. **ASAN Scan**: If applicable, grep logs for leak reports
4. **Server Shutdown**: Graceful termination with cleanup

## Metrics Tracked

| Metric | Description | Prometheus Endpoint |
|--------|-------------|---------------------|
| `hydrogen_process_resident_memory_bytes` | Process RSS memory | `/api/system/prometheus` |
| `hydrogen_database_queries_executed_total` | Query counter | `/api/system/prometheus` |
| `hydrogen_webserver_connections_current` | Active connections | `/api/system/prometheus` |
| `hydrogen_webserver_requests_total` | HTTP request counter | `/api/system/prometheus` |
| `hydrogen_process_open_fds` | Open file descriptors | `/api/system/prometheus` |
| `hydrogen_http_post_requests_total` | POST request count | `/api/system/prometheus` |
| `hydrogen_http_api_requests_total` | API request count | `/api/system/prometheus` |

## Timing Considerations

### ASAN Mode

ASAN instrumentation adds significant overhead. Timeouts are extended:

- **Startup Timeout**: Default 120s (via `CONDUIT_STARTUP_TIMEOUT`)
- **Webserver Ready Timeout**: Default 180s (via `CONDUIT_WEBSERVER_READY_TIMEOUT`)
- **Request Timeout**: 5s (unchanged)

### Database Readiness

Each database has individual migration status. The test reports readiness before proceeding:

```bash
# In result file:
DATABASE_READY_PostgreSQL=true
DATABASE_READY_MySQL=true
DATABASE_READY_SQLite=true
...
```

## Expected Results

### Success Criteria

- All prerequisites validated
- Hydrogen server starts successfully
- At least one database is ready for testing
- All 500 auth requests complete
- Steady-state memory growth < 1 KB/request
- No ASAN leaks detected (if ASAN mode used)
- Server shuts down cleanly

### Output Artifacts

- **`test_41_{timestamp}_exercise.result`**: Summary of tests passed/failed
- **`test_41_{timestamp}_metrics.log`**: Detailed metrics timeline
- **`test_41_{timestamp}_exercise.log`**: Server stdout/stderr

### Sample Output

```text
Request   RSS_MB      Delta_MB    Queries     Conns    HTTP_Req  FDs
=======   ========    ========    ========    ======== ========  ====
[50/500]  45.2 MB     +12.3 MB    1520        8        1600      24
[100/500] 46.1 MB     +13.2 MB    3040        8        3100      24
...

Warmup growth (first 250 reqs): 13.2 KB
Steady-state growth (last 250 reqs): 12.8 KB
Steady-state rate: 0.05 KB/request (threshold: 1 KB/req)
```

## Integration

This test runs as part of the full test suite:

```bash
# Run this specific test
./tests/test_41_exercise.sh

# Run as part of full test suite
./tests/test_00_all.sh
```

## Troubleshooting

### Common Issues

- **No databases ready**: Check database engine availability, connection strings in config
- **Memory threshold exceeded**: Investigate leak patterns in metrics log; review code changes
- **ASAN binary not found**: Build with `cmake -DCMAKE_BUILD_TYPE=Debug`. Note that the plain `hydrogen_debug` target may lack the embedded payload
- **Slow startup under ASAN**: Normal for debug builds; adjust `CONDUIT_STARTUP_TIMEOUT` if needed

### Diagnostic Information

- Check server log for migration status messages
- Review metrics log for memory growth patterns
- ASAN logs contain detailed allocation traces for leak sources

## Related Documentation

- [test_11_leaks_like_a_sieve.md](/docs/H/tests/test_11_leaks_like_a_sieve.md) - Pure infrastructure leak detection
- [conduit_utils.md](/docs/H/tests/conduit_utils.md) - Conduit testing utilities
- [Test Framework](/docs/H/tests/TESTING.md) - Overall testing approach

## From the script

```bash
# Test: Memory Exercise - Single Server Auth Stress Test
# Starts one Hydrogen server with all 7 database engines configured,
# runs 500 auth requests cycling across databases, and scrapes
# Prometheus metrics to track memory growth for leak detection.
# When hydrogen_debug (ASAN) build is available, launches with it and
# performs post-shutdown LeakSanitizer analysis on the server log
# for definitive leak detection on DB queue / auth paths.
#
# This test runs a single server with PostgreSQL, MySQL, SQLite, DB2,
# MariaDB, CockroachDB, and YugabyteDB all simultaneously configured.
# Auth requests cycle round-robin across all databases while metrics
# are collected at regular intervals.
#
# Uses conduit_utils.sh library for server lifecycle management,
# matching the proven approach from tests 50 and 51.
# Purpose: exercise DB-heavy paths (unlike test_11 which avoids DBs)
# with both RSS growth heuristics and ASAN when available.

# FUNCTIONS
# scrape_metrics()
# get_metric()
# run_auth_request()

# CHANGELOG
# 3.9.0 - 2026-06-22 - Stop hand-rolling output in the native run:
#                     - Removed the bespoke per-100-request progress block (ETA/rate timing and the
#                       separate test_41_progress.log side-channel). That custom code was the only
#                       structural difference from the (clean) first exercise loop and was the source
#                       of the stray "INFO1"/"PASS1" / leading-digit artifacts on the native lines.
#                     - Periodic native snapshots are now emitted with print_output (DATA), the standard
#                       harness idiom for dumping captured data values, instead of a custom print_message.
#                     - Dropped NATIVE_PROGRESS_LOG / NATIVE_PROGRESS_INTERVAL (and their test_03 whitelist
#                       entries). Per-snapshot metrics are still written to the native metrics log file.
# 3.8.0 - 2026-06-22 - Harness-norms + presentation:
#                     - The long native (hydrogen_release) RSS run is now a single proper subtest
#                       ("Native RSS Measurement (5000 requests)"): one TEST header, INFO progress
#                       lines, and exactly one terminal PASS/FAIL per code path. Previously it
#                       emitted bare INFO/PASS calls hanging off the prior subtest counter.
#                     - Native server shutdown moved before the steady-state verdict so the PASS/FAIL
#                       is the last line of the subtest.
#                     - Footer growth title now colorized via {BLUE}...{RESET} to match tests 30/60/99,
#                       e.g. "Exercise  Growth: 1,512 B/req".
#                     - Requires framework.sh >= 3.1.0 / log_output.sh >= 4.3.0 which widen the elapsed
#                       column to 4-digit seconds so the collected-output sort stays aligned past 999.999s
#                       (this test is ~20m). (Note: that overflow was a separate latent issue; it was not
#                       the cause of the native-line artifacts addressed in 3.9.0.)
# 3.7.0 - 2026-06-22 - Reliability + reporting fixes:
#                     - scrape_metrics() now retries (SCRAPE_MAX_ATTEMPTS, SCRAPE_CURL_TIMEOUT, SCRAPE_RETRY_DELAY)
#                       and only accepts a response containing hydrogen_ metrics. Under ASAN the Prometheus
#                       endpoint could be momentarily unresponsive under heavy auth load, causing every snapshot
#                       and the final scrape to return "No metrics returned" and fail "Collect Final Metrics".
#                     - Fixed doubled timestamp in METRICS_LOG/RESULT_FILE/NATIVE_* filenames: LOG_PREFIX already
#                       embeds ${TIMESTAMP}, so the extra ${TIMESTAMP} produced paths like
#                       test_41_<ts><ts>_metrics.log. Now uses ${LOG_PREFIX}_... .
#                     - Footer title now reports the native steady-state growth rate in bytes/request from the
#                       long 5000-request native run, e.g. "Exercise (Growth: 1,512 B/req)".
# 3.6.0 - 2026-06-17 - Extended the native (hydrogen_release) RSS measurement run to 5000 iterations (from 500).
#                     Snapshots now every 500 requests (NATIVE_SNAPSHOT_INTERVAL) during the long run to distinguish
#                     warmup growth from steady-state or slow accumulation. Uses NATIVE_* constants for the extended
#                     run while preserving the original 500-iter run (with 50-iter snapshots) for ASAN + short RSS.
#                     Updated leak analysis, progress messages, and headers to use the native request count.
#                     For the long native run only: set HYDROGEN_LOG_LEVEL=STATE before launch (via run_conduit_server).
#                     TRACE/DEBUG output is not required for RSS measurement; it was producing 100s of MB–GB logs and
#                     unnecessary I/O. "STARTUP COMPLETE" and "Migration completed..." messages used by readiness checks
#                     are emitted at STATE level, so the native phase still waits correctly. ASAN/short first run keeps
#                     default (full) logging for LeakSanitizer detail.
# 3.4.0 - 2026-06-16 - RSS-based "leak suspected" no longer fails test under ASAN mode (heuristic is secondary);
#                     high steady-state RSS during ASAN exercise is expected (shadow memory + quarantine + allocator
#                     behavior) and does not indicate a userspace leak when LeakSanitizer reports clean. Still logs
#                     observed rates for diagnostics. Prevents false positive FAIL when ASAN is the authoritative check.
#                     Addresses misreporting in test 41 runs under hydrogen_debug.
# 3.3.0 - 2026-06-14 - Enhanced for thorough DB/auth leak detection (ASAN + RSS)
#                     - When hydrogen_debug (ASAN build) is available, launch server under it
#                       with ASAN_OPTIONS=detect_leaks=1:leak_check_at_exit=1:... during exercise
#                     - Post-shutdown LeakSanitizer analysis on the exercise server log (direct/indirect counts)
#                     - Complements existing RSS steady-state growth heuristic (now secondary)
#                     - Exercises real DB paths: execute_auth_query, DQM pending/await, QueryRefs for login,
#                       JWT store/revoke, rate limiting, account lookup, password verify across 7 engines
#                     - Fixed latent leaks in database_pending.c: late/expired QueryResult now always use
#                       database_engine_cleanup_result (was raw free + TODOs); orphan signals cleaned
#                     - Purpose: test_11 avoids DBs; test_41 is the DB stress + leak exercise
#                     - Increased startup/webserver-ready timeouts in conduit_utils.sh (120s default)
#                       and set higher (180s) via CONDUIT_*_TIMEOUT when forcing ASAN debug for 7DB case
#                     - Guarded ASAN leak analysis subtest: only report "no leaks" if server actually started
#                       and exercise ran; on startup failure under ASAN, skip rather than falsely pass
# 3.2.0 - 2026-03-20 - Tightened leak threshold from 2 KB/req to 1 KB/req
#                     - Observed steady-state rate ~0.51 KB/req across 500 requests
#                     - 1 KB/req gives ~2x safety margin while catching leaks earlier
# 3.1.0 - 2026-03-19 - Added memory leak detection with steady-state growth analysis
#                     - Compares last-half growth rate to detect unbounded leaks
#                     - Threshold: 2 KB/request in second half flags as leak
#                     - Reports warmup vs steady-state growth separately
# 3.0.0 - 2026-03-19 - Adopted conduit_utils.sh approach from tests 50/51
#                     - Uses run_conduit_server() for startup/migration/readiness
#                     - Uses DATABASE_NAMES from conduit_utils.sh (Demo_PG, etc.)
#                     - Uses check_database_readiness() for per-DB readiness
#                     - Uses shutdown_conduit_server() for clean shutdown
#                     - Config updated to match test 50 database naming pattern
#                     - Removed ad-hoc startup/migration handling
#                     - Removed HYDROGEN_LOG_LEVEL=STATE (was hiding migration logs)
# 2.1.0 - 2026-03-19 - Per-database migration awareness
#                     - Only tests databases that successfully complete migration
#                     - DB-specific log checking (30s fast-fail, 180s max timeout)
#                     - Reports ready vs total databases before exercising
#                     - All output routed through print_message (no raw tee)
# 2.0.0 - 2026-03-19 - Rewrote to use single server with all 7 databases
#                     - Fixed metrics URL from /metrics to /api/system/prometheus
#                     - Added API prefix extraction from config
#                     - Added delay between metric scrapes for stats to update
#                     - Created dedicated config hydrogen_test_41_exercise.json
#                     - Cycles auth requests round-robin across all databases
#                     - Follows test_40 wiring patterns for framework integration
# 1.1.0 - 2026-03-19 - Initial implementation
# 1.0.0 - 2026-03-19 - Created
```