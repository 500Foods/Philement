# Hydrogen Testing Framework

This folder contains the comprehensive testing framework for the Hydrogen Server.
It includes test scripts, configuration files, support utilities, and detailed documentation for validating the functionality, performance, and reliability of the Hydrogen system.

## Overview

The Hydrogen testing framework is designed to ensure the robustness and correctness of the server through a structured suite of blackbox (aka integration) tests. These tests cover various aspects including compilation, startup/shutdown sequences, API functionality, system endpoints, and code quality checks.

NOTE: A separate document covers the [Unity Unit Test Framework](/docs/H/tests/TESTING_UNITY.md).

## Core Components

### Test Suite Runner (test_00_all.sh)

**test_00_all.sh** is the central orchestration script for executing the entire test suite. It manages the execution of all test scripts, either in parallel batches or sequentially, and generates a comprehensive summary of results.

- **Version**: the `TEST_VERSION` line at the top of `tests/test_00_all.sh`. Do not copy a version number into this page.
- **Key Features**:
  - Runs one tens-digit group at a time (0x, then 1x, then 2x, and so on). Tests inside a group run in parallel. Group 6 is sequential by default.
  - `--sequential` runs every test one after another. Naming tests on the command line also runs those tests sequentially.
  - Supports skipping test execution for quick summary generation or README updates.
  - Automatically updates the project README.md with test results and repository statistics using the `cloc` tool.
  - Provides detailed output with test status, subtest counts, pass/fail statistics, and elapsed time.
- **Usage**:

  ```bash
  # Run the suite. Groups run in order. Tests inside a group run in parallel.
  ./test_00_all.sh

  # Run all tests sequentially
  ./test_00_all.sh --sequential

  # Run specific tests (always sequential)
  ./test_00_all.sh 01_compilation 17_startup_shutdown

  # Skip test execution and update README
  ./test_00_all.sh --skip-tests
  ```

## Test Scripts

Below is the full list of `tests/test_*.sh` scripts, in the same order as [`/docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md). Add or rename a script in both files together.

Test 01 builds the targets. Test 10 runs the Unity executables, which call project functions directly. That suite is described in [`/docs/H/tests/TESTING_UNITY.md`](/docs/H/tests/TESTING_UNITY.md). Test 11 is the AddressSanitizer leak run. Test 31 lints migration SQL on disk and does not launch the server. The other scripts are blackbox tests: they run the server and observe it from outside.

### Test Suite Management - Orchestration

- **[test_00_all.sh](/docs/H/tests/test_00_all.md)**: Test suite runner for orchestrating all tests (described above).

### Compilation and Static Analysis

- **[test_01_compilation.sh](/docs/H/tests/test_01_compilation.md)**: Verifies successful compilation and build processes.
- **[test_02_secrets.sh](/docs/H/tests/test_02_secrets.md)**: Checks validity of key environment variables
- **[test_03_shell.sh](/docs/H/tests/test_03_shell.md)**: Validates environment variables used in Hydrogen configuration
- **[test_04_check_links.sh](/docs/H/tests/test_04_check_links.md)**: Validates links in documentation files.

### Core Functional Tests

- **[test_10_unity.sh](/docs/H/tests/test_10_unity.md)**: Integrates Unity testing framework for unit tests.
- **[test_11_leaks_like_a_sieve.sh](/docs/H/tests/test_11_leaks_like_a_sieve.md)**: AddressSanitizer leak run against the debug build. The script header comment still says Valgrind; the body sets `ASAN_OPTIONS`.
- **[test_12_env_variables.sh](/docs/H/tests/test_12_env_variables.md)**: Tests that configuration values can be provided via environment variables.
- **[test_13_crash_handler.sh](/docs/H/tests/test_13_crash_handler.md)**: Tests that the crash handler correctly generates and formats core dumps.
- **[test_14_library_dependencies.sh](/docs/H/tests/test_14_library_dependencies.md)**: Checks for required library dependencies.
- **[test_15_json_error_handling.sh](/docs/H/tests/test_15_json_error_handling.md)**: Tests JSON configuration error handling.
- **[test_16_shutdown.sh](/docs/H/tests/test_16_shutdown.md)**: Tests the shutdown functionality of the application with a minimal configuration.
- **[test_17_startup_shutdown.sh](/docs/H/tests/test_17_startup_shutdown.md)**: Validates complete startup and shutdown lifecycles.
- **[test_18_signals.sh](/docs/H/tests/test_18_signals.md)**: Tests signal handling (e.g., SIGINT, SIGTERM, SIGHUP).
- **[test_19_socket_rebind.sh](/docs/H/tests/test_19_socket_rebind.md)**: Tests socket rebinding behavior.

### Server Tests

- **[test_20_api_prefix.sh](/docs/H/tests/test_20_api_prefix.md)**: Validates API prefix configurations.
- **[test_21_system_endpoints.sh](/docs/H/tests/test_21_system_endpoints.md)**: Tests system endpoint functionality.
- **[test_22_swagger.sh](/docs/H/tests/test_22_swagger.md)**: Verifies Swagger documentation and UI integration.
- **[test_23_websockets.sh](/docs/H/tests/test_23_websockets.md)**: Tests WebSocket server functionality and integration.
- **[test_24_uploads.sh](/docs/H/tests/test_24_uploads.md)**: Tests uploading a file to the server
- **[test_26_terminal.sh](/docs/H/tests/test_26_terminal.md)**: Tests terminal functionality, payload serving, and WebSocket connections
- **[test_27_reporting_image_scale.sh](/docs/H/tests/test_27_reporting_image_scale.md)**: Tests the Reporting image_scale endpoint with ImageMagick for image conversion and scaling
- **[test_48_mdns.sh](/docs/H/tests/test_48_mdns.md)**: Tests mDNS server and client functionality and integration

### Database Tests

- **[test_30_database.sh](/docs/H/tests/test_30_database.md)**: All Engines Parallel Operational Test (PostgreSQL, MySQL, SQLite, DB2, MariaDB, Firebird, YugabyteDB, MSSQL, plus a combined config)
- **[test_31_migrations.sh](/docs/H/tests/test_31_migrations.md)**: SQL-lints Helium migration files on disk with sqruff. Does not launch Hydrogen
- **[test_32_postgres_migrations.sh](/docs/H/tests/test_32_postgres_migrations.md)**: PostgreSQL migration performance test
- **[test_33_mysql_migrations.sh](/docs/H/tests/test_33_mysql_migrations.md)**: MySQL migration performance test
- **[test_34_sqlite_migrations.sh](/docs/H/tests/test_34_sqlite_migrations.md)**: SQLite migration performance test
- **[test_35_db2_migrations.sh](/docs/H/tests/test_35_db2_migrations.md)**: DB2 migration performance test
- **[test_36_mariadb_migrations.sh](/docs/H/tests/test_36_mariadb_migrations.md)**: MariaDB migration performance test
- **[test_37_firebird_migrations.sh](/docs/H/tests/test_37_firebird_migrations.md)**: Firebird migration performance test
- **[test_38_yugabytedb_migrations.sh](/docs/H/tests/test_38_yugabytedb_migrations.md)**: YugabyteDB migration performance test
- **[test_39_mssql_migrations.sh](/docs/H/tests/test_39_mssql_migrations.md)**: MSSQL migration performance test

### API

- **[test_40_auth.sh](/docs/H/tests/test_40_auth.md)**: Authentication endpoints testing (JWT tokens across multiple database engines)
- **[test_41_exercise_asan.sh](/docs/H/tests/test_41_exercise.md)**: Memory exercise under ASAN/LSAN (500 concurrent auths, 7 enabled databases; YugabyteDB disabled; WebServer port 5410)
- **[test_42_oidc_rp.sh](/docs/H/tests/test_42_oidc_rp.md)**: OIDC Relying Party - complete authentication flow testing
- **[test_43_scripting.sh](/docs/H/tests/test_43_scripting.md)**: Scripting subsystem end-to-end testing (WebServer ports 1543x)
- **[test_44_exercise_native.sh](/docs/H/tests/test_44_exercise_native.md)**: Native RSS exercise (5000 concurrent auths, 7 enabled databases; YugabyteDB disabled; WebServer port 5444; suite-parallel with test 41)
- **[test_45_oidc_idp.sh](/docs/H/tests/test_45_oidc_idp.md)**: OIDC Identity Provider across eight engines (WebServer ports 5450–5458): discovery, JWKS, authorize/login, PKCE token, userinfo, refresh
- **[test_46_conduit_script.sh](/docs/H/tests/test_46_conduit_script.md)**: Conduit script invoke (`POST/GET /api/conduit/script`, JWT + Api.Echo, WebServer ports 1546x)
- **[test_47_mcp.sh](/docs/H/tests/test_47_mcp.md)**: MCP Streamable HTTP (JWT initialize/tools/call, WebServer 1547x, MCP 1548x)
- **[test_50_conduit_query.sh](/docs/H/tests/test_50_conduit_query.md)**: Single public `/api/conduit/query` (WebServer port 5500)
- **[test_51_conduit_queries.sh](/docs/H/tests/test_51_conduit_queries.md)**: Conduit multiple-queries endpoint (`hydrogen_test_51_conduit_queries.json`, WebServer port 5510)
- **[test_52_conduit_auth_query.sh](/docs/H/tests/test_52_conduit_auth_query.md)**: Authenticated conduit query testing (WebServer port 5520)
- **[test_53_conduit_auth_queries.sh](/docs/H/tests/test_53_conduit_auth_queries.md)**: Authenticated conduit batch query testing (WebServer port 5530)
- **[test_54_conduit_alt_query.sh](/docs/H/tests/test_54_conduit_alt_query.md)**: Alternative conduit query testing (WebServer port 5540)
- **[test_55_conduit_alt_queries.sh](/docs/H/tests/test_55_conduit_alt_queries.md)**: Alternative conduit batch query testing (WebServer port 5550)
- **[test_56_cap_query.sh](/docs/H/tests/test_56_cap_query.md)**: Conduit `/api/conduit/cap_query` (WebServer port 5560)
- **[test_57_mailrelay_outbound.sh](/docs/H/tests/test_57_mailrelay_outbound.md)**: Mail Relay outbound delivery testing
- **[test_58_mailrelay_api.sh](/docs/H/tests/test_58_mailrelay_api.md)**: Mail Relay authenticated API, plaintext and STARTTLS (WebServer ports 15800–15835)
- **[test_59_auth_chat.sh](/docs/H/tests/test_59_auth_chat.md)**: Authenticated chat REST and WebSocket against a local mock LLM (WebServer 15900, mock LLM 15901, WebSocket 15902)
- **[test_60_performance.sh](/docs/H/tests/test_60_performance.md)**: Performance benchmarking (WebServer port 5600)
- **[test_61_mailrelay_inbound.sh](/docs/H/tests/test_61_mailrelay_inbound.md)**: Mail Relay inbound SMTP listener (mailval sink 5580, inbound SMTP 5581)
- **[test_62_nats.sh](/docs/H/tests/test_62_nats.md)**: NATS client against a local broker (broker port 5620, WebServer ports 5621–5623), plus the `/api/nats/status` and `/api/nats/instances` JWT endpoints
- **[test_63_conduit_webhook.sh](/docs/H/tests/test_63_conduit_webhook.md)**: Conduit webhook ingress (`POST /api/conduit/webhook/{hook}` + `/webhook/{hook}` alias, HMAC sha256/sha256-timestamp, 8 engines, WebServer ports 1563x)
- **[test_64_queryref_transactions.sh](/docs/H/tests/test_64_queryref_transactions.md)**: Runtime QueryRef transaction blocks on `/api/conduit/alt_query` (QueryRefs 156–158, eight engines, WebServer port 5640)

### Deliverables

- **[test_70_installer.sh](/docs/H/tests/test_70_installer.md)**: Bash script for end-user installation
- **[test_71_database_diagrams.sh](/docs/H/tests/test_71_database_diagrams.md)**: Database diagram generation
- **[test_72_schemahelper.sh](/docs/H/tests/test_72_schemahelper.md)**: SchemaHelper fixture validation and luacheck (no live DB required)
- **[test_73_argent_mcp.sh](/docs/H/tests/test_73_argent_mcp.md)**: Argent MCP tools and validation variants (8 engines, WebServer 1573x, MCP 1574x)

### Static Analysis & Code Quality Tests

- **[test_89_coverage.sh](/docs/H/tests/test_89_coverage.md)**: Unity and blackbox coverage.
- **[test_90_markdownlint.sh](/docs/H/tests/test_90_markdownlint.md)**: Lints Markdown documentation using markdownlint.
- **[test_91_cppcheck.sh](/docs/H/tests/test_91_cppcheck.md)**: Performs C/C++ static analysis using cppcheck.
- **[test_92_shellcheck.sh](/docs/H/tests/test_92_shellcheck.md)**: Validates shell scripts using shellcheck with exception justification checks.
- **[test_93_jsonlint.sh](/docs/H/tests/test_93_jsonlint.md)**: Validates JSON file syntax and structure.
- **[test_94_eslint.sh](/docs/H/tests/test_94_eslint.md)**: Lints JavaScript files using eslint.
- **[test_95_stylelint.sh](/docs/H/tests/test_95_stylelint.md)**: Validates CSS files using stylelint.
- **[test_96_htmlhint.sh](/docs/H/tests/test_96_htmlhint.md)**: Validates HTML files using htmlhint.
- **[test_97_xmlstarlet.sh](/docs/H/tests/test_97_xmlstarlet.md)**: Validates XML/SVG files using xmlstarlet.
- **[test_98_luacheck.sh](/docs/H/tests/test_98_luacheck.md)**: Lua code analysis with luacheck
- **[test_99_code_size.sh](/docs/H/tests/test_99_code_size.md)**: Analyzes code size metrics and file distribution. The page is `test_99_code_size.md`. There is no `test_99_cleanup.md`.

## Configuration Files

Checked-in configs live in [`/elements/001-hydrogen/hydrogen/tests/configs/`](/elements/001-hydrogen/hydrogen/tests/configs/). The script names the file it launches. Read `WebServer.Port` from that file.

## Port Numbering Scheme

`test_00_all.sh` runs one tens-digit group at a time. Tests in a group run in parallel, except group 6, which is sequential by default. A `WebServer.Port` must not match another test in the same group. Later groups may reuse a port, because the groups do not overlap.

Do not invent a port from the test number. Several tests already sit outside a `5<test-number>x` block, and some use one Hydrogen process for every engine.

Test 40 is one config per engine. These are the `WebServer.Port` values:

| Engine | Port | Configuration file |
| --- | --- | --- |
| PostgreSQL | 5400 | `hydrogen_test_40_postgres.json` |
| MySQL | 5401 | `hydrogen_test_40_mysql.json` |
| SQLite | 5402 | `hydrogen_test_40_sqlite.json` |
| DB2 | 5403 | `hydrogen_test_40_db2.json` |
| MariaDB | 5406 | `hydrogen_test_40_mariadb.json` |
| Firebird | 5407 | `hydrogen_test_40_firebird.json` |
| YugabyteDB | 5408 | `hydrogen_test_40_yugabytedb.json` |
| MSSQL | 5409 | `hydrogen_test_40_mssql.json` |

Tests 50–56 each launch one server: 5500, 5510, 5520, 5530, 5540, 5550, and 5560. Test 51's config is `hydrogen_test_51_conduit_queries.json`. There is no `hydrogen_test_51_postgres.json`.

These WebServer ports are already assigned and should be left as they are:

- 41 is 5410. 44 is 5444. 45 is 5450–5458. 60 is 5600. 64 is 5640.
- 43 is 15430–15447. 46 is 15460–15467. 47 is 15470–15478, with the MCP listener on 1548x.
- 58 is 15800–15835. 59 is 15900. 62 is 5621–5623, and the local broker listens on 5620.
- 63 is 15630–15639. 73 is 15730–15738, with the MCP listener on 1574x.

A new test takes a free port inside its tens-digit group, written to `WebServer.Port` in its config.

## Linting Configurations

Linting behavior is controlled by configuration files to ensure code quality:

- **.lintignore**: Global exclusion patterns for all linting tools.
- **.lintignore-c**: C/C++ specific configurations for `cppcheck`, defining enabled checks and suppressions.
- **.lintignore-markdown**: Markdown linting rules for documentation consistency.

## Test Output Directories

`tests/lib/framework.sh` sets `TESTS_DIR` to `${BUILD_DIR}/tests`. Each run writes there, and the harness cleans that tree at the start:

- **build/tests/logs/**: Execution logs for each test run.
- **build/tests/results/**: Summaries and result data.
- **build/tests/diagnostics/**: Diagnostic data for the run.

Paths are relative to [`/elements/001-hydrogen/hydrogen/`](/elements/001-hydrogen/hydrogen/). The suite does not write `tests/logs/`, `tests/results/`, or `tests/diagnostics/`.

## Documentation

Per-test pages and the library guide live in [`/docs/H/tests/`](/docs/H/tests/):

- **test_00_all.md**: The suite runner.
- One page per script. Code size is [`test_99_code_size.md`](/docs/H/tests/test_99_code_size.md). There is no `test_99_cleanup.md`.
- **[LIBRARIES.md](/docs/H/tests/LIBRARIES.md)**: Library scripts in `tests/lib/`.
- Unity tests: [`/docs/H/tests/TESTING_UNITY.md`](/docs/H/tests/TESTING_UNITY.md).

## Developing New Tests

When creating new test scripts, adhere to the following standards:

1. **Naming Convention**: Use `test_NN_descriptive_name.sh` following the numbering system.
2. **Headers**: Include version, title, and change history at the top of the script.
3. **Shellcheck Compliance**: Ensure scripts pass `shellcheck` validation for quality and consistency.
4. **Integration**: Ensure compatibility with `test_00_all.sh` for automatic discovery and execution.
5. **Documentation**: Add a page under `/docs/H/tests/` when asked, and update this file and [`/docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md). Do not create a test script unless asked.
6. **`TEST_COUNTER` is library-owned.** Call `print_subtest` (it increments) and pass `"${TEST_NUMBER}" "${TEST_COUNTER}"` into `print_result` / `print_message` / `print_command`. Never assign or increment `TEST_COUNTER` in a test script.
7. **TEST + PASS/FAIL cadence.** Every `print_subtest` (TEST line) must be followed by exactly one `print_result` (one PASS or one FAIL). INFO/DATA/EXEC/WARN may sit between them. Do not emit consecutive TEST lines or consecutive PASS/FAIL lines. `print_subtest` auto-closes an unpaired TEST as FAIL; an extra `print_result` with no open TEST is a WARN, not a second PASS/FAIL.

### Workflow for New Tests

1. Copy an existing test script as a template.
2. Modify the script with necessary test logic and headers.
3. Validate with `shellcheck` to ensure compliance.
4. Test the script independently before integrating into the suite.
5. Document the test in `/docs/H/tests/` and add it to this file and `/docs/H/INSTRUCTIONS.md`.

## Usage Examples

### Running the Full Test Suite

```bash
# Groups run in order. Tests inside a group run in parallel.
./test_00_all.sh

# Run all tests sequentially
./test_00_all.sh --sequential
```

### Running Specific Tests

```bash
# Named tests always run sequentially. 17 is startup/shutdown. 22 is Swagger.
./test_00_all.sh 01_compilation 17_startup_shutdown
```

### Skipping Test Execution

```bash
# Skip test execution but update README with what would run
./test_00_all.sh --skip-tests
```

### Manual Testing with Configurations

From `elements/001-hydrogen/hydrogen/tests/`:

```bash
# Minimal and maximal startup configs used by Test 17
../hydrogen ./configs/hydrogen_test_17_startup_min.json
../hydrogen ./configs/hydrogen_test_17_startup_max.json
```

For more detailed information on the Hydrogen testing approach, refer to the [Testing Documentation](/docs/H/core/testing.md).

## Test Renumbering / Renaming

For various reasons, we sometimes renumber or rename tests. Often this is to ensure that the ordering in the main orchestration runs is optimal.
Sometimes it is because a test function changes. Or maybe due to just wanting the ordering to be more sensible. Whatever the reason,
this causes a number of issues that need to be resolved, mostly involving links to the file.

- [`/docs/H/STRUCTURE.md`](/docs/H/STRUCTURE.md) and [`/docs/H/SITEMAP.md`](/docs/H/SITEMAP.md) need the new path
- The page under [`/docs/H/tests/`](/docs/H/tests/) needs the new name
- [`/elements/001-hydrogen/hydrogen/tests/README.md`](/elements/001-hydrogen/hydrogen/tests/README.md) needs the new link
- [`/RELEASES.md`](/RELEASES.md) needs a release note when the rename ships
- [`/docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) and this file need the new script in the inventory