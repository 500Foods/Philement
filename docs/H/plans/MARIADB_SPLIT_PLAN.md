# MariaDB Engine Split Plan

## Goal
Formally split MariaDB from MySQL in the codebase. Create a new `src/database/mariadb/` directory mirroring `src/database/mysql/`, add `DB_ENGINE_MARIADB` to the engine enum, and wire up all conditions that check for MySQL to also handle MariaDB.

## Status: COMPLETE

## Phases Complete
- Phase 1: Engine enum, function signatures, test params
- Phase 2: 16 source files mirroring mysql/
- Phase 3: Split conditions across 10 source files
- Phase 4: Engine registration (mariadb_get_interface, registry, engine_init)
- Phase 5: Migration execution (execute_mariadb_migration + switch case, forward decl in migration.h)
- Phase 6: Mock framework (mock_libmariadb.h/.c, CMakeLists-unity.cmake detection)
- Phase 7: Unit tests (22 test files mirroring mysql/, all passing)
- Phase 8: Build & verify (mkt, mkp, mks, mku all pass)

## Issues Resolved During Implementation
1. `mariadb_connect` macro conflict in `<mysql.h>` (line 932: `#define mariadb_connect(hdl, conn_str) ...`)
   — Added `#undef mariadb_connect` after `#include <mysql.h>` in `src/database/mariadb/query.c` and
   `tests/unity/src/database/mariadb/query_test_mariadb_bind_persist_shape.c`
2. Missing forward declaration for `execute_mariadb_migration` in `migration/migration.h`
3. Missing stub implementations for `mock_libmariadb_set_mysql_stmt_*` functions in `mock_libmariadb.c`
4. Test assertion expected `"mysql"` but interface name is `"mariadb"` in `interface_test_mariadb.c`
5. `database_engine.h` had stale `database_get_counts_by_type` signature (missing mariadb_count param)
6. `launch_database.c` callsite of `check_database_library_dependencies` had 9 args, needed 10 (added mariadb_count)
7. CMakeLists-unity.cmake: reordered mariadb detection BEFORE mysql detection to prevent false matches
8. CMake target name conflict: `query_test_additional_coverage.c` renamed to `query_test_mariadb_additional_coverage.c`

## Phases

### Phase 1: Engine Enum & Type Definitions
- Add `DB_ENGINE_MARIADB` to `DatabaseEngine` enum in `src/database/database_types.h` (after `DB_ENGINE_MSSQL`, before `DB_ENGINE_FIREBIRD`)
- Update `database.h`: add `database_get_counts_by_type` signature to include mariadb count parameter
- Update `database_types.h` to reflect new count

### Phase 2: Create mariadb/ Source Files
Create 16 files in `src/database/mariadb/` mirroring `src/database/mysql/`:
- `mariadb.h` / `mariadb.c` — engine version/info functions (`mariadb_engine_get_version`, `mariadb_engine_is_available`, `mariadb_engine_get_description`)
- `types.h` — function pointer typedefs and globals (prefixed `mariadb_`, loading `libmariadb.so` / `libmariadb.so.3`)
- `connection.h` / `connection.c` — connect/disconnect/health_check/reset/cancel/cache management (`mariadb_connect`, `mariadb_disconnect`, etc.)
- `interface.h` / `interface.c` — engine interface registration (`mariadb_get_interface`), with `.engine_type = DB_ENGINE_MARIADB`, `.name = "mariadb"`
- `query.h` / `query.c` — query execution (`mariadb_execute_query`, `mariadb_execute_prepared`)
- `query_helpers.h` / `query_helpers.c` — helper functions (`mariadb_process_direct_result`, etc.)
- `prepared.h` / `prepared.c` — prepared statement management (`mariadb_prepare_statement`, etc.)
- `transaction.h` / `transaction.c` — transaction management (`mariadb_begin_transaction`, etc.)
- `utils.h` / `utils.c` — utility functions (`mariadb_get_connection_string`, `mariadb_validate_connection_string`, etc.)

Key changes in mariadb files vs mysql mirror:
- `dlopen("libmariadb.so.3")` / `dlopen("libmariadb.so")` instead of `libmysqlclient.so.*`
- `DB_ENGINE_MARIADB` instead of `DB_ENGINE_MYSQL` in all engine_type checks
- Function names: `mariadb_*` instead of `mysql_*`
- Type names: `MariadbConnection` instead of `MySQLConnection`
- Connection string uses `mariadb://` prefix
- Library handle: `libmariadb_handle` instead of `libmysql_handle`
- Log messages: "MariaDB" instead of "MySQL"

### Phase 3: Split Conditions Across Source Files
Update all files that check `strcmp(..., "mysql") == 0` to also check `"mariadb"`:

1. `src/database/database_engine_registry.c` — line 45: add `else if (strcmp(engine_type, "mariadb") == 0)` branch; register mariadb engine interface
2. `src/database/database_manage.c` — lines 21, 39: add `"mariadb"` to string comparisons, add `mariadb_engine_get_description()` forward declaration
3. `src/database/database_connstring.c` — line 456: add `mariadb://` parsing (same as mysql://)
4. `src/database/dbqueue/heartbeat.c` — `database_queue_determine_engine_type()`: map `mariadb://` → `DB_ENGINE_MARIADB`; `database_queue_mask_connection_string()`: mask `mariadb://` passwords
5. `src/database/dbqueue/heartbeat.c` — connection failure logging: add `"mariadb://"` → "MariaDB" engine name
6. `src/database/migration/transaction.c` — add `case DB_ENGINE_MARIADB:` calling `execute_mariadb_migration()`
7. `src/database/migration/execute_helpers.c` — `normalize_engine_name()`: add `"mariadb"` → `"mariadb"` mapping
8. `src/database/migration/lua.c` — line 118: add `"mariadb"` to engines array
9. `src/database/database_engine_metrics.c` — line 57: add `"mariadb"` to mysql count check; update supported engines string
10. `src/database/database_manage.c` — `database_get_engine_interface()`: add `"mariadb"` → `DB_ENGINE_MARIADB`
11. `src/database/database_params.c` — line 286: add `DB_ENGINE_MARIADB` to `?` placeholder case
12. `src/launch/launch_database.c` — line 59: add `"mariadb"` to mysql count tracking
13. `src/config/config_databases.c` — line 679: add `"mariadb"` to mysql count tracking
14. `src/mailrelay/mailrelay_repository.c` — line 360: add `"mariadb"` to translation check

### Phase 4: Database Engine Registration
- `database_engine_registry.c`: forward declare `mariadb_get_interface()`, add mariadb_count counter, register mariadb engine in `database_engine_init()`

### Phase 5: Migration Execution
- `migration/transaction.c`: add `execute_mariadb_migration()` function (mirrors `execute_mysql_migration`, using mariadb function pointers) and add case in switch

### Phase 6: Mock Framework
- Create `tests/unity/mocks/mock_libmariadb.h` / `mock_libmariadb.c` — mirror of `mock_libmysqlclient.h` / `.c` with `mariadb_*` names
- Update `cmake/CMakeLists-unity.cmake`:
  - Add `mock_libmariadb.c` to `UNITY_MOCK_SOURCES`
  - Add `string(FIND "${SOURCE_FILE}" "mariadb" IS_MARIADB_SOURCE)` detection
  - Add mock defines for mariadb source files (`-DUSE_MOCK_LIBMARIADB`)
  - Add test detection for mariadb test files

### Phase 7: Unit Tests
- Create `tests/unity/src/database/mariadb/` directory with mirrored test files from `tests/unity/src/database/mysql/`
- 22 test files mirroring the mysql tests, with `mariadb_*` function names
- Add `IS_MARIADB_TEST` detection in CMakeLists-unity.cmake

### Phase 8: Build & Verify
- `mkt` — full trial build, verify no new static functions, dead code check
- `mkp` — cppcheck lint
- `mks` — shellcheck lint
- `mku <mariadb_test_name>` — run mariadb unity tests

## Exit Gate
All phases complete, `mkt` builds cleanly, `mku` mariadb tests pass, `mkp` and `mks` are clean.
