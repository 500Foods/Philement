# RECIPE FOR SUCCESS

Hydrogen is a C project that implements a comprehensive suite of capabilities.

- REST API with Swagger provided by embedded payload
- Web, WebSocket, SMTP, mDNS, and other services are included
- Blackbox (Integration) and Unity unit tests with gcov for coverage reporting

All directories are relative to the repository root
The base for the project is in /elements/001-hydrogen/hydrogen/
The source code is in /elements/001-hydrogen/hydrogen/src/
The cmake files are in /elements/001-hydrogen/hydrogen/cmake/
The tests are in /elements/001-hydrogen/hydrogen/tests/
The documentation for the project is in /docs/H/

## ⚠️ CRITICAL INSTRUCTIONS

- Review /docs/H/tests/TESTING.md before continuing
- Aliases and other env vars are defined in ~/.zshrc
- After ordinary C edits (existing `.c`/`.h` already in the CMake glob), run alias `mkq` (quick trial: skip clean and cmake reconfigure) - can be run from any directory
- Use alias `mkt` (full trial: clean + cmake + build) when there is no usable `build/` tree, after CMake/preset changes, or after **adding/removing** `src/` files (the source glob is resolved at configure time, so `mkq` will not see a new `.c`)
- After ANY C coding change that passes `mkq` or `mkt`, run alias `mkp` (cppcheck / test_91) before considering the change done
- The trial build outputs minimal text - typically only error messages - greatly reducing AI token usage
- Once test build succeeds, run alias `mka` to perform build against all targets - can be run from any directory
- Working with unit tests, after alias `mkq` (or `mkt` if the tree was just configured) use alias `mku <base test name without .c>` to build and run the test from any directory
- A new or renamed file under `tests/unity/src/` is picked up only after `mkt`. CMake globs `*_test*.c` at configure time, so `mkq` keeps the previous file list
- When making only script changes, there is no need to run alias `mkq`/`mkt` - that is for changes to C code only
- After ANY Bash/script change, run alias `mks` (shellcheck / test_92) before considering the change done
- After a docs-only change, run alias `mkl` (Test 04, markdown link check)
- Assume all env vars (like HYDROGEN_ROOT) and all aliases are properly defined and declared

**IMPORTANT NOTE:**  these aliases are not loaded by default into the regular AI model environment, you can run them by using a zsh instance like this:

```bash
zsh -ic 'mkq' 2>&1
zsh -ic 'mkt' 2>&1
zsh -ic 'mku mutex_test_stats' 2>&1
```

## ⚠️ ADDITIONAL GUIDANCE

- When searching code, use tools like grep to find patterns efficiently
- Follow the C and Bash coding requirements strictly to avoid linting errors detected by cppcheck and shellcheck
- Use the repository structure diagram to quickly understand the codebase organization
- Only create new test scripts when specifically asked to create test scripts

## PROJECT STRUCTURE - from elements/001-hydrogen/hydrogen

```directory
build/          Temporary build artifacts
cmake/          cmake/CMakeLists.txt for all builds, including Unity
configs/        JSON configuration files
examples/       Example code
extras/         Helpful scripts and code snippets
payloads/       Payload definitions
src/
 ├─api/          API endpoints
 ├─config/       Config management
 ├─database/     Database engines and queues
 ├─handlers/     Signal and crash handlers
 ├─landing/      Shutdown handlers
 ├─launch/       Startup handlers
 ├─logging/      Logging system
 ├─mailrelay/    Mail relay
 ├─mcp/          MCP server
 ├─mdns/         Service discovery
 ├─mirage/       Mirage
 ├─mutex/        Mutex helpers
 ├─nats/         NATS client
 ├─network/      Network interfaces
 ├─oidc/         OpenID Connect
 ├─payload/      Payload management
 ├─print/        Print subsystem
 ├─queue/        Thread-safe queues
 ├─registry/     Subsystem registry
 ├─reporting/    Reporting
 ├─resources/    Resource caches
 ├─scripting/    Lua scripting
 ├─state/        System state
 ├─status/       Status reporting
 ├─swagger/      Swagger support
 ├─terminal/     Terminal
 ├─threads/      Thread management
 ├─utils/        Utilities
 ├─webserver/    HTTP server
 ├─websocket/    WebSocket
 ├─globals.c
 ├─globals.h
 ├─hydrogen.c    Main entry
 ├─hydrogen.h    Umbrella header, included first in every .c
 └─hydrogen_not.c
tests/          Test framework
```

## TESTS

This list is every `tests/test_*.sh` script. When a script is added or renamed, update this list and [`/docs/H/tests/TESTING.md`](/docs/H/tests/TESTING.md) in the same change. Per-test pages live in `/docs/H/tests/`.

- tests/test_00_all.sh - Orchestrator that runs all the tests
- tests/test_01_compilation.sh - Builds all targets
- tests/test_02_secrets.sh - Checks that PAYLOAD_LOCK and PAYLOAD_KEY are usable
- tests/test_03_shell.sh - Validates environment variables used in Hydrogen configuration
- tests/test_04_check_links.sh - Cross-checks all Markdown files for missing links and orphaned files
- tests/test_10_unity.sh - Runs the Unity unit-test battery
- tests/test_11_leaks_like_a_sieve.sh - AddressSanitizer leak run (the debug build). Test 10 is Unity
- tests/test_12_env_variables.sh - Checks that environment variables in JSON config files work
- tests/test_13_crash_handler.sh - Tests that crash dumps contain debugging information
- tests/test_14_library_dependencies.sh - Checks for all the libraries the project requires
- tests/test_15_json_error_handling.sh - Tests that the Hydrogen Server reports JSON errors
- tests/test_16_shutdown.sh - Quick check that Hydrogen shuts down cleanly without hanging
- tests/test_17_startup_shutdown.sh - Checks that Hydrogen starts and stops cleanly in min and max configurations
- tests/test_18_signals.sh - Confirms signal handling for SIGHUP, SIGUSR1, SIGUSR2, SIGTERM and SIGINT
- tests/test_19_socket_rebind.sh - Confirms TIME_WAIT is configured correctly
- tests/test_20_api_prefix.sh - Checks that changing the /api prefix works
- tests/test_21_system_endpoints.sh - Validates the functionality of system endpoints
- tests/test_22_swagger.sh - Checks that Swagger files are served from the payload
- tests/test_23_websockets.sh - Retrieves status from the WebSocket interface
- tests/test_24_uploads.sh - Upload a file to the server
- tests/test_26_terminal.sh - xterm.js terminal, payload serving, and WebSocket connections
- tests/test_27_reporting_image_scale.sh - Reporting image scale endpoint with ImageMagick
- tests/test_30_database.sh - All-engines parallel operational test (PostgreSQL, MySQL, SQLite, DB2, MariaDB, Firebird, YugabyteDB, MSSQL, plus a combined config)
- tests/test_31_migrations.sh - SQL-lints Helium migration files on disk with sqruff. Does not launch Hydrogen
- tests/test_32_postgres_migrations.sh - PostgreSQL migration performance test
- tests/test_33_mysql_migrations.sh - MySQL migration performance test
- tests/test_34_sqlite_migrations.sh - SQLite migration performance test
- tests/test_35_db2_migrations.sh - DB2 migration performance test
- tests/test_36_mariadb_migrations.sh - MariaDB migration performance test
- tests/test_37_firebird_migrations.sh - Firebird migration performance test
- tests/test_38_yugabytedb_migrations.sh - YugabyteDB migration performance test
- tests/test_39_mssql_migrations.sh - MSSQL migration performance test
- tests/test_40_auth.sh - Authentication endpoints (JWT tokens across multiple database engines)
- tests/test_41_exercise_asan.sh - Memory exercise under ASAN (500 concurrent auth requests across 7 enabled databases + LeakSanitizer; YugabyteDB disabled)
- tests/test_42_oidc_rp.sh - OIDC Relying Party authentication flow through role mapping
- tests/test_43_scripting.sh - Scripting subsystem orchestrator lifecycle across all 8 database engines in parallel (with and without DefaultDatabase, fail-fast)
- tests/test_44_exercise_native.sh - Native RSS exercise (5000 concurrent auth requests across 7 enabled databases; YugabyteDB disabled; port 5444; suite-parallel with test 41)
- tests/test_45_oidc_idp.sh - OIDC Identity Provider across eight engines (WebServer ports 5450–5458)
- tests/test_46_conduit_script.sh - Conduit script invoke JWT blackbox (Api.Echo, 8 engines parallel)
- tests/test_47_mcp.sh - MCP Streamable HTTP blackbox (JWT, Mcp.Echo, 8 engines; WebServer 1547x, MCP 1548x)
- tests/test_48_mdns.sh - mDNS announcements
- tests/test_50_conduit_query.sh - Single public query endpoint (WebServer port 5500)
- tests/test_51_conduit_queries.sh - Conduit multiple-queries endpoint (`hydrogen_test_51_conduit_queries.json`, WebServer port 5510)
- tests/test_52_conduit_auth_query.sh - Authenticated single query endpoint
- tests/test_53_conduit_auth_queries.sh - Authenticated batch query endpoint
- tests/test_54_conduit_alt_query.sh - Cross-database single query with override
- tests/test_55_conduit_alt_queries.sh - Cross-database batch query with override
- tests/test_56_cap_query.sh - Conduit `/api/conduit/cap_query` (WebServer port 5560). Page: [`test_56_cap_query.md`](/docs/H/tests/test_56_cap_query.md)
- tests/test_57_mailrelay_outbound.sh - Mail Relay outbound delivery with a local SMTP sink
- tests/test_58_mailrelay_api.sh - Mail Relay authenticated API, plaintext and STARTTLS, across engines
- tests/test_59_auth_chat.sh - Authenticated chat REST and WebSocket against a local mock LLM (WebServer 15900, mock LLM 15901, WebSocket 15902). Page: [`test_59_auth_chat.md`](/docs/H/tests/test_59_auth_chat.md)
- tests/test_60_performance.sh - Performance benchmarking across databases
- tests/test_61_mailrelay_inbound.sh - Mail Relay inbound SMTP listener (mailval sink 5580, inbound SMTP 5581). Page: [`test_61_mailrelay_inbound.md`](/docs/H/tests/test_61_mailrelay_inbound.md)
- tests/test_62_nats.sh - NATS client blackbox (local nats-server, ports 5620–5629; JWT `/api/nats/status` and `/api/nats/instances`)
- tests/test_63_conduit_webhook.sh - Conduit webhook ingress (HMAC signatures, alias, error paths, 8 engines, WebServer ports 1563x)
- tests/test_64_queryref_transactions.sh - QueryRef transaction blocks on `/api/conduit/alt_query` (QueryRefs 156–158, WebServer port 5640)
- tests/test_70_installer.sh - Standalone installer build
- tests/test_71_database_diagrams.sh - Database diagram generation
- tests/test_72_schemahelper.sh - SchemaHelper fixture and luacheck (no live DB)
- tests/test_73_argent_mcp.sh - Argent MCP tools and validation variants (8 engines; WebServer 1573x, MCP 1574x)
- tests/test_89_coverage.sh - Unity and blackbox coverage
- tests/test_90_markdownlint.sh - Markdown lint
- tests/test_91_cppcheck.sh - C lint (cppcheck)
- tests/test_92_shellcheck.sh - Bash lint (shellcheck)
- tests/test_93_jsonlint.sh - JSON lint
- tests/test_94_eslint.sh - JavaScript lint
- tests/test_95_stylelint.sh - CSS lint
- tests/test_96_htmlhint.sh - HTML lint
- tests/test_97_xmlstarlet.sh - XML and SVG lint (xmlstarlet)
- tests/test_98_luacheck.sh - Lua lint (luacheck)
- tests/test_99_code_size.sh - Source size limits and cloc. There is no test_99_cleanup.sh

## CRITICAL DOCUMENTATION

- /docs/H/README.md - contains TOC for all docs
- /RELEASES.md - release history and update instructions
- /docs/H/SITEMAP.md - links to all markdown docs
- /docs/H/STRUCTURE.md - links to all files
- /docs/H/LUA_GUIDE.md - intro to Lua and writing Hydrogen host-API scripts
- /docs/H/LUA_FEATURES.md - pure Lua language features and standard library recipes
- /docs/H/MAIL_GUIDE.md - Mail Relay, templates, rewrites, and Lua mail handling
- /docs/H/core/subsystems/scripting/lua_api.md - Lua `H.*` host API reference
- /docs/H/core/subsystems/mcp/mcp.md - MCP Streamable HTTP subsystem
- /docs/H/core/subsystems/nats/nats.md - NATS client, cache invalidation, relay, and presence
- /docs/H/tests/TESTING.md - blackbox and integration tests
- /docs/H/tests/TESTING_UNITY.md - Unity unit tests

## MARKDOWN CODING REQUIREMENTS

- Use consistent headings (# H1, ## H2)
- Use absolute links starting with /docs/H or with /elements/001-hydrogen/hydrogen/
- DO NOT USE RELATIVE LINKS LIKE ../../../elements/...
- DO NOT ADD LINE REFS TO LINKS LIKE /docs/H/README.md:123 - they don't work
- Use headings instead of **topic** for emphasis
- Fix common markdown issues such as these
  - MD009/no-trailing-spaces: Trailing spaces
  - MD012/no-multiple-blanks: Multiple consecutive blank lines
  - MD022/blanks-around-headings: Headings should be surrounded by blank lines
  - MD032/blanks-around-lists: Lists should be surrounded by blank lines
  - Unique headings
  - Label code snippets
- Resolve any problems reported by Visual Studio Code
- Run Test 04 after adding new documents

## C CODING REQUIREMENTS

- cppcheck configured with all directives enabled, including styles
- All cppcheck issues need to be corrected - prefer fixes over exceptions
- src/hydrogen.h is included first in all .c source files
- Function Prototypes absolutely must be declared for every function
- Grouped/commented include files at the top of every .c/.h file
- DO NOT USE GOTO UNDER ANY CIRCUMSTANCES. Why? Because I said so.
- src/config/config_defaults.c contains many useful subsystem defaults
- Use `log_this` when outputting anything to the log
- cmake/CMakeLists.txt is used for all project builds including Unity
- Run tests/test_91_cppcheck.sh whenever C code is changed
- Unit test filenames must be unique - search before creating new files
- When using #include use <src/folder/...> instead of "../../.."
- cmake files are in cmake/ folder, starting with cmake/CMakeLists.txt

> ⚠️ **CRITICAL — NO `static` FUNCTIONS (ENFORCED AT BUILD TIME)**
> Do **NOT** declare internal helper functions `static` in `src/`. Unity
> Framework unit tests link directly against the project's object files and
> **cannot call `static` functions**, so `static` helpers are impossible to
> test. This has bitten us repeatedly: the `mailrelay` and `scripting`
> subsystems (new additions) shipped with ~200 `static` helpers that later had
> to be de-`static`'d by hand just to enable unit tests.
> **Rule:** every function that could conceivably be a Unity test target must
> be non-`static` **and** have a visible declaration in a header (a `static`
> function whose only caller is in the same file is a red flag). The only
> acceptable `static` usage is for file-scope *state* (globals, string
> literals, test-seam callbacks) — never for callable functions. `static` in
> a Unity test file is fine. The gate covers `src/` only.
>
> **This rule is enforced automatically.** `mkq`/`mkt` (the trial build) runs a
> *static-function gate* in its Dependency Check: any **new** `static`
> function definition added to `src/` fails the build immediately, with a
> message telling you to remove `static` and add a header declaration. The set
> of pre-existing `static` names is grandfathered in
> `tests/.static-baseline.txt`. Do not regenerate that file from a sweep of
> `src/`. A sweep would grandfather the callable statics the gate exists to
> stop. When the new static is file-scope state, add that one name to the
> baseline by hand.

- Do not add callable `static` functions in `src/`. Unity cannot call them, and the trial build rejects a new one

IMPORTANT: The alias `mkp` can be used to run cppcheck against all project sources.

## BASH CODING REQUIREMENTS

- shellcheck configured with all directives enabled
- All shellcheck issues need to be corrected - prefer fixes over exceptions
- Update CHANGELOG and TEST_VERSION at top of every script after every change
- ALWAYS use `jq` for JSON parsing, filtering, and validation
- NEVER use `grep` or text manipulation tools for JSON data
- Try to reduce external calls wherever possible to increase performance
- Do NOT use a statement like ((x++)) instead use x=$(( x + 1 ))
- Use the full "${variable}" declaration  instead of $variable wherever possible
- SC2292: Prefer [[ ]] over [ ]
- SC2250: Braces around variable preferences even when not strictly required
- SC2312: Invoke commands separately to avoid masking return values (or use `|| true` to ignore)
- SC2248: Add double quotes even when variables don't contain special characters
- Run tests/test_92_shellcheck.sh whenever Bash code is changed

IMPORTANT: The alias `mks` can be used to run shellcheck against all project scripts.

## CONFIGURATION

App uses a JSON-based configuration with robust fallback handling. Details in src/config/config.c comments.
src/config/config_defaults.c has definition of what is used when no JSON configuration file is provided.

A. Server
B. Network
C. Database
D. Logging
E. WebServer
F. API
G. Swagger
H. WebSocket
I. Terminal
J. mDNS Server
K. mDNS Client
L. Mail Relay
M. Print
N. Resources
O. OIDC
P. Notify
Q. Scripting
R. Reporting
S. Webhooks
T. MCP
U. Chat (config-only today: no launch or landing)
V. NATS

W is reserved for Subscribers. That letter is not in `config.h` yet.
Subscribers is still an unapproved plan. The finished NATS plan is
[`/docs/H/plans/complete/NATS_PLAN_COMPLETE.md`](/docs/H/plans/complete/NATS_PLAN_COMPLETE.md).

## SUBSYSTEM ORDER

App uses subsystems and a launch/landing system to control them. Details in src/launch/launch.c comments.

1. Registry
2. Payload
3. Threads
4. Network
5. Database
6. Logging
7. WebServer
8. API
9. Swagger
10. WebSocket
11. Terminal
12. mDNS Server
13. mDNS Client
14. MailRelay
15. Print
16. Resources
17. OIDC
18. Notify
19. Scripting
20. Reporting
21. MCP
22. NATS

That list is what `launch_readiness.c` registers today. Subscribers is
planned as **23** and is not approved. `MAX_SUBSYSTEMS` is 24. Do not
add a 25th registration: the readiness writer does not bounds-check
the index.

Landing order is the table in `landing_readiness.c`, first entry first.
It is not this list reversed. Comments in `landing_readiness.c` and
`landing.c` that say "reverse launch order" are stale. NATS is inserted
before Print. `landing_plan.c` `expected_order[]` is only the Go/No-Go
log.

## LAUNCH / LANDING

- Launch Phases: Readiness > Plan > Launch <subsystem> > Review
- Landing Phases: Readiness > Plan > Landing <subsystem> > Review

## RESOURCES

- Payload Cache
- Token Hash Cache
- SQL Query Cache
- AI Prompt Cache
- Mail Template Cache
- Icon Cache

## Resuming a Subsystem Implementation

When returning to a multi-phase subsystem such as Mail Relay, use this quick-start to avoid rediscovering state:

1. Read the subsystem plan, e.g. [`/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md`](/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md).
2. Check the current phase status blocks and the Working Log at the bottom of the plan. Close a phase the way [`/docs/PLANNING.md`](/docs/PLANNING.md) says.
3. Verify the baseline build by running `zsh -ic 'mkt'` (full trial) or `zsh -ic 'mkq'` if `build/` is already configured.
4. Run the existing Unity tests for the subsystem:
   `zsh -ic 'mku mailrelay_message_test && mku mailrelay_render_test && mku mailrelay_smtp_test && mku mailrelay_send_raw_test && mku config_mail_relay_test_load_mailrelay_config'`
5. Inspect the current source directory: [`/elements/001-hydrogen/hydrogen/src/mailrelay/`](/elements/001-hydrogen/hydrogen/src/mailrelay/).
6. Inspect the current Unity tests: [`/elements/001-hydrogen/hydrogen/tests/unity/src/mailrelay/`](/elements/001-hydrogen/hydrogen/tests/unity/src/mailrelay/).
7. Review uncommitted changes with `git status`.
8. Build aliases are defined in `~/.zshrc`; in non-interactive shells invoke them as `zsh -ic '<alias>'`. The common aliases are:
    - `mkq` — quick trial build (no clean, no cmake reconfigure)
    - `mkt` — full trial build (clean + cmake). Required after adding or removing a `src/` or `tests/unity/src/` file
    - `mka` — build all targets
    - `mku <base>` — build and run one Unity test. The argument is the basename without `.c`, for example `mutex_test_stats`
    - `mkp` — C lint (cppcheck, `tests/test_91_cppcheck.sh`)
    - `mks` — shell script lint (shellcheck, `tests/test_92_shellcheck.sh`)
    - `mkl` — markdown link check (Test 04)

Use this baseline before asking qualifying questions and presenting the next implementation chunk.
