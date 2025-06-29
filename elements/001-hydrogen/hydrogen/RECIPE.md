# HYDROGEN

🚨 **CRITICAL: Trial Build with CMake MUST PASS** 🚨
Run the following command from the project root to set up and perform the trial build using the predefined preset:

```bash
cd elements/001-hydrogen/hydrogen/cmake && cmake -S . -B ../build --preset default && cmake --build ../build --preset default && cmake --build ../build --preset trial
```

Requires:

1. No warnings/errors
2. Shutdown test passes
3. Only then done

**Important Note on Build Process:**
Always run the trial build first after making any code changes to ensure there are no errors or warnings. Only proceed with other build types (like debug, release, etc.) after the trial build passes successfully. This approach helps catch issues early and maintains the integrity of the codebase.

**Building and Running the Release Version:**
To build the release version of Hydrogen for production deployment, which includes optimizations and embedded payload, run the following command from the project root:

```bash
cd elements/001-hydrogen/hydrogen/cmake && cmake -B ../build && cmake --build ../build --target hydrogen_release
```

After building, to run the release executable and verify features like Swagger, use:

```bash
cd elements/001-hydrogen/hydrogen && ./hydrogen_release
```

This ensures you are using the optimized release build with all configured features enabled.

## Project

```json
{"name":"Hydrogen","lang":"C","type":"REST API/WebSocket Server","arch":"Multithreaded",
"tech":["POSIX","HTTP/WS","mDNS","Queues","JSON","SQL","OIDC","OpenSSL","Postgres"],
"critical":["ErrorHandling","ThreadSafety"]}
```

## Coding REQUIREMENTS

- Function Prototypes at top of every .c file
- Grouped/commented include files at the top of every .c/.h file
- Use log_this outputing anything
- Update RELEASES.md only when a task is complete

## Configuration

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

## Subsystem Order

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

## Structure

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

## Docs

- developer_onboarding.md: Setup
- coding_guidelines.md: Standards
- api.md: API reference
- testing.md: Test guide

## Logging

```c
// Levels: 0=TRACE 1=DEBUG 2=STATE 3=ALERT 4=ERROR 5=FATAL 6=QUIET
log_this("Component", "State: READY", LOG_LEVEL_STATE, true, true, true);
log_this("Component", "Failed: reason", LOG_LEVEL_ERROR, true, true, true);
```

## Launch/Landing

- Registry first in, last out
- Phases: Readiness > Plan > Execute > Review
- Landing reverses launch exactly
- SIGHUP triggers landing then re-launch
- All resources freed during landing

## Builds

Use CMake presets or build configurations for different build types. Refer to `CMakePresets.json` for details.

- **trial**: Required, clean build with tests
- **debug**: Includes debug symbols for troubleshooting
- **perf**: Optimized for performance analysis
- **release**: Optimized for production use
- **valgrind**: Configured for memory checking with Valgrind
- **default**: Standard build configuration
