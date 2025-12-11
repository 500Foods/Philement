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
- After ANY C coding change, run alias `mkt` to perform a test build - can be run from any directory
- The trial build outputs minimal text - typically only error messages - greatly reducing AI token usage
- Once test build succeeds, run alias `mka` to perform build against all targets - can be run from any directory
- Working with unit tests, after alias `mkt` use alias `mku <base test name without .c>` to build and run the test fro any directory
- When making only script changes, there is no need to run alias `mkt` - that is for changes to C code only
- Assume all env vars (like HYDROGEN_ROOT) and all aliases are properly defined and declaredp

## ⚠️ ADDITINAL GUIDANCE

- When searching code, use tools like grep to find patterns efficiently
- Follow the C and Bash coding requirements strictly to avoid linting errors detected by cppcheck and shellcheck
- Use the repository structure diagram to quickly understand the codebase organization
- Only create new test scripts when specifically asked to create test scripts

## PROJECT STRUCTURE - from elements/001-hydrogen/hydrogen

```directory
build/          Temporary build artifacts
cmake/          cmake/CMakeLists.tst for all builds
configs/        JSON configuration files
examples/       Example code
extras/         Helpful scripts and code snippets
payloads/       Payload definitions
src/
 ├─api/          API endpoints
 ├─config/       Config management
 ├─database/       Database management
 ├─landing/      Shutdown handlers
 ├─launch/       Startup handlers
 ├─logging/      Logging system
 ├─mdns/         Service discovery
 ├─network/      Network interfaces
 ├─oidc/         OpenID Connect
 ├─payload/      Payload management
 ├─print/        Print subsystem
 ├─queue/        Thread-safe queues
 ├─registry/     Subsystem registry
 ├─state/        System state
 ├─state/        Terminal
 ├─threads/      Thread management
 ├─utils/        Utilities
 ├─webserver/    HTTP server
 ├─websocket/    WebSocket
 └─hydrogen.c    Main entry
tests/          Test framework
```

## TESTS

- tests/test_00_all.sh - Orchestrator that runs all the tests
- tests/test_01_compilation.sh - Builds all targets
- tests/test_02_secrets.sh - Checks that PAYLOAD_LOCK and PAYLOAD_KEY are usable
- tests/test_03_shell.sh - Validates environment variables used in Hydrogen configuration
- tests/test_04_check_links.sh - Cross-checks all Markdown files for missing links and orphaned files
- tests/test_10_unity.sh - Runs battery of Unity unit tests
- tests/test_11_leaks_like_a_sieve.sh - Uses ASAN to test for memory leaks
- tests/test_12_env_variables.sh - Checks that environment variables on JSON confg files work properly
- tests/test_13_crash_handler.sh - Tests that crash dumps contain debugging information
- tests/test_14_library_dependencies.sh - Checks for all the libraries the project requires
- tests/test_15_json_error_handling.sh - Tests that the Hydrogen Server will report JSON errors properly
- tests/test_16_shutdown.sh - Quick check that Hydrogen Server shuts down cleanly without hanging
- tests/test_17_startup_shutdown.sh - Checks that the Hyrogen Server starts and stops cleanly in min and max configurations
- tests/test_18_signals.sh - Confirms signal handling for SIGHUP, SIGUSR1, SIGUSR2, SIGTERM and SIGINT
- tests/test_19_socket_rebind.sh - Confirms TIME_WAIT is configured correctly
- tests/test_20_api_prefix.sh - Checks that changing the /api prefix works
- tests/test_21_system_endpoints.sh - Validates the functionality of system endpoints
- tests/test_22_swagger.sh - Checks that Swagger files are served up from payload
- tests/test_23_websockets.sh - Retrieves status from WebSockets interface
- tests/test_24_uploads.sh - Upload file to server
- tests/test_25_mdns.sh - mDNS announcements
- tests/test_26_terminal.sh - Testing xterm.js implementation
- tests/test_30_database.sh - All Engines Parallel Operational Test
- tests/test_32_postgres_migrations.sh - PostgreSQL migration performance test
- tests/test_33_mysql_migrations.sh - MySQL migration performance test
- tests/test_34_sqlite_migrations.sh - SQLite migration performance test
- tests/test_35_db2_migrations.sh - DB2 migration performance test
- tests/test_41_conduit.sh - Conduit Query endpoint test
- tests/test_70_installer.sh - Standalone installer building test
- tests/test_71_database_diagrams.sh - Database diagram generation
- tests/test_89_coverage.sh - Shows Unity and Blackbox test coverage
- tests/test_90_markdownlint.sh - Lint for Markdown
- tests/test_91_cppcheck.sh - Lint for C
- tests/test_92_shellcheck.sh - Lint for Bash
- tests/test_93_jsonlint.sh - Lint for JSON
- tests/test_94_eslint.sh - Lint for JavaScript
- tests/test_95_stylelint.sh - Lint for CSS
- tests/test_96_htmlhint.sh - Lint for HTML
- tests/test_97_xmlstarlet.sh - Lint for XML/SVG (xmlstarlet)
- tests/test_98_luacheck.sh - Lua code analysis with luacheck
- tests/test_99_code_size.sh - Checks limits on source code, runs cloc

## CRITICAL DOCUMENTATION

- /docs/H/README.md - contains TOC for all docs
- /docs/H/RELEASES.md - release history and update instructions
- /docs/H/SITEMAP.md - links to all markdown docs
- /docs/H/STRUCTURE.md - links to all files
- /docs/H/tests/TESTING.md - blackbox/integration tests tests
- /docs/H/tests/TESTING_UNITY.md - Unity unit tests

## C CODING REQUIREMENTS

- cppcheck configured with all directives enabled, including styles
- All cppcheck issues need to be corrected - prefer fixes over exceptions
- src/hydrogen.h is included first in all .c source files
- Function Prototypes absolutely must be declared for every function
- Grouped/commented include files at the top of every .c/.h file
- src/config/config_defaults.c contains many useful subsystem defaults
- Use `log_this` when outputing anything to the log
- cmake/CMakeLists.txt is used for all project builds including Unity
- Run tests/test_91_cppcheck.sh whenever C code is changed
- Unit test filenames must be unique - search beforee creating new files
- Don't use static functions if possible as we can't test those
- When using #include use <src/folder/...> instead of "../../.."
- cmake files are in cmake/ folder, starting with cmake/CMakeLists.txt

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
- Run tests/test_92_cppcheck.sh whenever Bash code is changed

IMPORTANT: The alias `mks` can be used to run shellcheck against all project scripts.

## MARKDOWN CODING REQUIREMENTS

- Use consistent headings (# H1, ## H2), valid links
- Use headings instead of **topic** for emphasis
- Fix common markdown issues such as these
- MD009/no-trailing-spaces: Trailing spaces
- MD012/no-multiple-blanks: Multiple consecutive blank lines
- MD022/blanks-around-headings: Headings should be surrounded by blank lines
- MD032/blanks-around-lists: Lists should be surrounded by blank lines
- Unique headings
- Label code snippets

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
Q. Mirage

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
19. Mirage

## LAUNCH / LANDING

- Launch Phases: Readiness > Plan > Launch <subsystem> > Review
- Landing PHases: Readiness > Plan > Landing <subsystem> > Review

## RESOURCES

- Payload Cache
- Token Hash Cache
- SQL Query Cache
- AI Prompt Cache
- Mail Template Cache
- Icon Cache
