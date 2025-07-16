# HYDROGEN

## CRITICAL INSTRUCTIONS

- The script `extras/make-trial.sh` is used to build the project and check for compilation errors
- This can be run using the alias `mkt` from any directory as it has been integrated as a shell alias for this purpose
- After ANY code/script/MD change, run `mkt` to verify compilation/linting. Do not attempt completion until it passes.
- End tasks by checking/updating RELEASES.md if changes warrant it.
- Do NOT run full tests (e.g., test_00_all.sh)—use `mkt` only; humans handle comprehensive testing.
- In order to run tests, typically `mkb` needs to be run, as this builds all the necessary test binaries.

## HYDROGEN PROJECT

```json
{"name":"Hydrogen","lang":"C","type":"REST API/WebSocket Server","arch":"Multithreaded",
"tech":["POSIX","HTTP/WS","mDNS","Queues","JSON","SQL","OIDC","OpenSSL","Postgres"],
"critical":["ErrorHandling","ThreadSafety"]}
```

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
docs/           Documentation
examples/       Example code
payloads/       Payload definitions
tests/          Test framework
```

## TESTS

- tests/test_00_all.sh
- tests/test_01_compilation.sh
- tests/test_03_code_size.sh
- tests/test_06_check_links.sh
- tets/test_10_leaks_like_a_sieve.sh
- tests/test_11_unity.sh
- tests/test_12_env_variables.sh
- tests/test_14_env_payload.sh
- tests/test_16_library_dependencies.sh
- tests/test_18_json_error_handling.sh
- tests/test_20_crash_handler.sh
- tests/test_22_startup_shutdown.sh
- tests/test_24_signals.sh
- tests/test_26_shutdown.sh
- tests/test_28_socket_rebind.sh
- tests/test_30_api_prefixes.sh
- tests/test_32_system_endpoints.sh
- tests/test_34_swagger.sh
- tests/test_36_websockets.sh
- tests/test_91_cppcheck.sh
- tests/test_92_shellcheck.sh
- tests/test_94_eslint.sh
- tests/test_95_stylelint.sh
- tests/test_96_htmlhint.sh
- tests/test_97_jsonlint.sh
- tests/test_98_markdownlint.sh
- tests/test_99_coverage.sh

## DOCUMENTATION

- README.md contains TOC for all docs
- SITEMAP.md contains links to all markdown docs
- STRUCTURE.md contains links to all relevant files
- tests/README.md contains test suite information
- docs/developer_onboarding.md: Setup
- docs/coding_guidelines.md: Standards
- docs/api.md: API reference
- docs/testing.md: Test guide

## BASH CODING REQUIREMENTS

- Update CHANGELOG and SCRIPT_VER at top of every script file after every change

## C CODING REQUIREMENTS

- Function Prototypes at top of every .c file
- Grouped/commented include files at the top of every .c/.h file
- cmake/CMakeLists.txt is used for main project builds
- tests/unity/CMakeLists.txt is used for building unity unit tests
- Use `log_this` when outputing anything to the log

## JSON PROCESSING REQUIREMENTS

- ALWAYS use `jq` for JSON parsing, filtering, and validation
- NEVER use `grep` or text manipulation tools for JSON data
- Use `jq -r` for raw string output when needed
- Use `jq -e` for existence checks (exits with error if false/null)
- Examples:
  - Check field exists: `echo "$json" | jq -e '.status.server_started'`
  - Extract value: `echo "$json" | jq -r '.version.build'`
  - Validate structure: `echo "$json" | jq -e '.status and .system and .version'`

## LINTING GUIDANCE FOR AI-GENERATED CONTENT

### cppcheck

- Follow coding_guidelines.md
- Use cppcheck-friendly patterns (e.g., check malloc returns, avoid unused vars, unused params)

### shellcheck

- Add shebangs
- Quote vars (e.g., echo "$VAR")
- Avoid shellcheck errors like SC2086

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
- All resources freed during landing
