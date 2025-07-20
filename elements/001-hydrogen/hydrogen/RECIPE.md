# RECIPE FOR SUCCESS

Hydrogen is a C project that implements a comprehensive suite of capabilities

- REST API with Swagger provided by embedded payload
- Web, WebSocket, SMTP, mDNS, and other services are included
- Blackbox and Unity unit tests with gcov for coverage reporting

## CRITICAL INSTRUCTIONS

- After ANY C coding change, run `mkt` to verify compilation/linting - do not attempt completion until it passes
- The trial build outputs minimal text - typically only error messages - greatly reducing AI token usage
- End tasks by checking/updating RELEASES.md if changes warrant it, following the instructions there closely
- Do NOT run full tests; humans handle comprehensive testing beyond `mkt`

## USEFUL ALIASES

These all assume that the repository has been cloned into a Projects folder off of the user's home directory. Adjust accordingly.  These also assume zsh - adding them to ~/.zshrc is the recommended approach.

### cdp - Change to the root of the Hydrogen project

`alias cdp='cd ~/Projects/Philement/elements/001-hydrogen/hydrogen/'`

### mka - Build all variants

`alias mka='cdp && extras/make-all.sh && cd - > /dev/null 2>&1'`

### mkb - Build all variants and run all tests

`alias mkb='cdp && cd tests && ./test_00_all.sh && cd - > /dev/null 2>&1'`

### mkc - Clean the project

`alias mkc='cdp && extras/make-clean.sh && cd - > /dev/null 2>&1'`

### mkl - Run the Markdown Links Check, aka github-sitemap.sh

`alias mkl='cdp && tests/lib/github-sitemap.sh README.md --quiet --noreport && cd - > /dev/null 2>&1'`

### mkt - Run the Trial Build

`alias mkt='cdp && extras/make-trial.sh && cd - > /dev/null 2>&1'`

## REPOSITORY STRUCTURE

```directory
src/
 ├─api/          API endpoints
 ├─config/       Config management
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
 ├─threads/      Thread management
 ├─utils/        Utilities
 ├─webserver/    HTTP server
 ├─websocket/    WebSocket
 └─hydrogen.c    Main entry
cmake/          cmake/CMakeLists.tst for all builds
configs/        JSON configuration files
docs/           Documentation including release notes
examples/       Example code
extras/         Helpful scripts and code snippets
payloads/       Payload definitions
tests/          Test framework
```

## TESTS

- tests/test_00_all.sh - Orchestrator that runs all the tests
- tests/test_01_compilation.sh - Builds all targets
- tests/test_03_code_size.sh - Checks limits on source code, runs cloc
- tests/test_06_check_links.sh - Cross-checks all Markdown files for missing links and orphaned files
- tests/test_19_leaks_like_a_sieve.sh - Uses ASAN to test for memory leaks
- tests/test_11_unity.sh - Runs battery of Unity unit tests
- tests/test_12_env_variables.sh - Checks that environment variables on JSON confg files work properly
- tests/test_14_env_payload.sh - Checks that PAYLOAD_LOCK and PAYLOAD_KEY are usable
- tests/test_16_library_dependencies.sh - Checks for all the libraries the project requires
- tests/test_18_json_error_handling.sh - Tests that the Hydrogen Server will report JSON errors properly
- tests/test_13_crash_handler.sh - Tests that crash dumps contain debugging information
- tests/test_22_startup_shutdown.sh - Checks that the Hyrogen Server starts and stops cleanly in min and max configurations
- tests/test_24_signals.sh - Confirms signal handling for SIGHUP, SIGUSR1, SIGUSR2, SIGTERM and SIGINT
- tests/test_26_shutdown.sh - Quick check that Hydrogen Server shuts down cleanly without hanging
- tests/test_28_socket_rebind.sh - Confirms TIME_WAIT is configured correctly
- tests/test_29_api_prefixes.sh - Checks that changing the /api prefix works
- tests/test_32_system_endpoints.sh - Validates the functionality of system endpoints
- tests/test_34_swagger.sh - Checks that Swagger files are served up from payload
- tests/test_36_websockets.sh - Retrieves status from WebSockets interface
- tests/test_91_cppcheck.sh - Lint for C
- tests/test_92_shellcheck.sh - Lint for Bash
- tests/test_94_eslint.sh - Lint for JavaScript
- tests/test_95_stylelint.sh - Lint for CSS
- tests/test_96_htmlhint.sh - Lint for HTML
- tests/test_97_jsonlint.sh - Lint for JSON
- tests/test_98_markdownlint.sh - Lint for Markdown
- tests/test_99_coverage.sh - Shows Unity and Blackbox test coverage

## CRITICAL DOCUMENTATION

- README.md - contains TOC for all docs
- RELEASES.md - release history and update instructions
- SITEMAP.md - links to all markdown docs
- STRUCTURE.md - links to all files
- cmake/README.md - cmake build information
- tests/README.md - blackbox and Unity unit tests

## C CODING REQUIREMENTS

- Function Prototypes at top of every .c file
- Grouped/commented include files at the top of every .c/.h file
- cmake/CMakeLists.txt is used for main project builds
- tests/unity/CMakeLists.txt is used for building unity unit tests
- Use `log_this` when outputing anything to the log

## BASH CODING REQUIREMENTS

- Update CHANGELOG and SCRIPT_VER at top of every script file after every change
- ALWAYS use `jq` for JSON parsing, filtering, and validation
- NEVER use `grep` or text manipulation tools for JSON data

## LINTING GUIDANCE

### cppcheck

- Use cppcheck-friendly patterns (e.g., check malloc returns, avoid unused vars, unused params)

### shellcheck

- Quote vars (e.g., echo "$VAR")
- Avoid shellcheck errors like SC2086
- Directives/exceptions require justification

### Markdown

- Use consistent headings (# H1, ## H2), valid links;
- Fix common markdown issues such as these
- MD009/no-trailing-spaces: Trailing spaces
- MD012/no-multiple-blanks: Multiple consecutive blank lines
- MD022/blanks-around-headings: Headings should be surrounded by blank lines
- MD032/blanks-around-lists: Lists should be surrounded by blank lines

## LOGGING

```c
// Levels: 0=TRACE 1=DEBUG 2=STATE 3=ALERT 4=ERROR 5=FATAL 6=QUIET
log_this("Component", "State: READY", LOG_LEVEL_STATE, true, true, true);
log_this("Component", "Failed: reason", LOG_LEVEL_ERROR, true, true, true);
```

## CONFIGURATION

App uses a JSON-based configuration with robust fallback handling. Details in src/config/config.c comments.

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

## LAUNCH / LANDING

- Registry first in, last out
- Phases: Readiness > Plan > Execute > Review
- Landing reverses launch exactly
- SIGHUP triggers landing then re-launch
- Everything allocated during launch is freed during landing

## RESOURCES

- Token Hash Cache
- SQL Query Cache
- AI Prompt Cache
- Mail Template Cache
- Icon Cache
