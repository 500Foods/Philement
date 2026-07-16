# Test 43: Scripting Subsystem — End-to-End Validation

## Overview

The [`test_43_scripting.sh`](/elements/001-hydrogen/hydrogen/tests/test_43_scripting.sh) script provides black-box testing for the Lua Scripting subsystem. It validates the complete Orchestrator lifecycle across all seven supported database engines, including Orchestrator startup, scheduled job execution, and clean shutdown.

## Purpose

This script validates:

- Orchestrator loading from the `scripts` table
- Database connectivity for script execution
- Scoreboard job tracking and completion
- Clean shutdown with Lua state cleanup
- Graceful handling of disabled or missing Orchestrator rows

## Test Configuration

- **Test Name**: Scripting End-to-End
- **Test Abbreviation**: SCR
- **Test Number**: 43
- **Version**: 2.1.0

## Test Architecture

### Phases Covered

| Phase | Description | Test Focus |
|-------|-------------|------------|
| Phase 11f | Real Orchestrator DB load | Orchestrator loads from `scripts` table via QueryRef #087 |
| Phase 11g | DB-backed `require` | Orchestrator can require modules from database |
| Phase 12 | Waiter signaling | Jobs complete and signal waiters |
| Phase 13 | Query functions | `H.query` / `H.altquery` can run from Orchestrator |
| Phase 16 | HTTP calls | `H.http.get/post` work from Orchestrator |
| Phase 17 | Async HTTP | Background HTTP worker pool functions |
| Phase 18 | LLM calls | `H.llm.call` works (if models configured) |

### Configuration Files

The test uses 14 configuration files, two per engine (with/without `DefaultDatabase`):

- `hydrogen_test_43_scripting_postgres.json` and `hydrogen_test_43_scripting_postgres_no_default.json`
- `hydrogen_test_43_scripting_mysql.json` and `hydrogen_test_43_scripting_mysql_no_default.json`
- `hydrogen_test_43_scripting_sqlite.json` and `hydrogen_test_43_scripting_sqlite_no_default.json`
- `hydrogen_test_43_scripting_db2.json` and `hydrogen_test_43_scripting_db2_no_default.json`
- `hydrogen_test_43_scripting_mariadb.json` and `hydrogen_test_43_scripting_mariadb_no_default.json`
- `hydrogen_test_43_scripting_cockroachdb.json` and `hydrogen_test_43_scripting_cockroachdb_no_default.json`
- `hydrogen_test_43_scripting_yugabytedb.json` and `hydrogen_test_43_scripting_yugabytedb_no_default.json`

### Port Assignment

All test configs use dedicated ports in the `1543x` / `1544x` range. These ports are below Linux's default ephemeral client-port range (`32768-60999`), preventing unrelated full-suite connections from temporarily occupying a Test 43 listener port.

| Engine | With DefaultDB | Without DefaultDB |
|--------|----------------|-------------------|
| PostgreSQL | 15430 | 15440 |
| MySQL | 15431 | 15441 |
| SQLite | 15432 | 15442 |
| DB2 | 15433 | 15443 |
| MariaDB | 15434 | 15444 |
| CockroachDB | 15435 | 15445 |
| YugabyteDB | 15436 | 15446 |

## Test Subtests

### Subtest 1: Orchestrator Starts and Runs

1. Start Hydrogen with scripting enabled and a real database
2. Wait for `"READY FOR REQUESTS"` in the log
3. Assert the log contains `"Orchestrator: started Orchestrators.Orchestrator"`
4. Assert the log contains `"Orchestrator: started"` (Lua-side)
5. Assert at least one `"Orchestrator: tick"` line appears
6. Assert no `"Orchestrator: failed"` or `"Lua error"` markers

### Subtest 2: Clean Shutdown

1. Send SIGTERM to the Hydrogen process
2. Assert the process exits within 15 seconds
3. Assert the log contains `"Orchestrator: destroyed"`
4. Assert the log contains `"Orchestrator: shutdown requested"`

### Subtest 3: Orchestrator Row Disabled

1. Update the `scripts` row to `status = Disabled` via direct database update
2. Restart Hydrogen
3. Wait for `READY FOR REQUESTS`
4. Assert the log contains `"continuing without one"`
5. Send SIGTERM, assert clean shutdown
6. Restore the row to `Enabled` at teardown

### Subtest 4: No DefaultDatabase Configured

1. Use the `_no_default.json` config variant
2. Restart Hydrogen
3. Wait for `READY FOR REQUESTS`
4. Assert the log contains `"using the single configured database"` (fallback) or `"no DefaultDatabase configured"`
5. Send SIGTERM, assert clean shutdown

## Expected Results

- All 14 engine/variant runs complete the full Orchestrator lifecycle
- Each run produces `LIFECYCLE_COMPLETE` result
- No memory leaks (verified by Test 11)
- Scoreboard is visible via authenticated `/api/system/info` endpoint

## Test Endpoints

This test does not use REST endpoints directly; it validates the Scripting subsystem through:

- Database `scripts` table for Orchestrator source
- Scoreboard for job tracking
- System logs for lifecycle verification

## Diagnostic Output

- **Server logs**: Located in `logs/test_43_scripting_<engine>.log`
- **Result files**: `results/test_43_<engine>_<variant>.result` with `LIFECYCLE_COMPLETE` or `FAILFAST`
- **SQLite artifact**: `tests/artifacts/database/sqlite/scripting_t43.sqlite` (optional, for fresh DB runs)

## Integration

This test is part of the Hydrogen test suite and runs automatically with:

```bash
# Run this specific test
./tests/test_43_scripting.sh

# Run as part of full test suite
./tests/test_00_all.sh
```

## Related Documentation

- [LUA_PLAN_COMPLETE.md](/docs/H/plans/LUA_PLAN_COMPLETE.md) - Scripting implementation roadmap
- [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) - Lua host API reference
- [Scripting README](/docs/H/core/subsystems/scripting/README.md) - Subsystem overview
- [TESTING.md](/docs/H/tests/TESTING.md) - Test framework overview
- [test_40_auth.md](/docs/H/tests/test_40_auth.md) - Parallel auth test structure

## Configuration Notes

### Required Database State

The test assumes the database has been migrated with scripts 1201-1210:

- `scripts` table with `Orchestrators.Orchestrator` row
- Lookup #061 (Script Types: orchestrator=0, script=1, snippet=2)
- Lookup #062 (Script Status: disabled=0, enabled=1)

If migrations are missing, the test will `FAILFAST` with a clear message.

### Single-Database Fallback

When `DefaultDatabase` is not configured but exactly one database is configured, the Orchestrator uses that database automatically. This allows the "without DefaultDatabase" variants to exercise the fallback path.

## Known Limitations

- Fresh SQLite databases may take >5 minutes for migrations on first run
- The 6 external engine databases (PostgreSQL, MySQL, etc.) require the scripting migrations to be applied
- Test uses the shared `hydrodemo.sqlite` fixture; if it lacks scripting migrations, the test will fail fast
