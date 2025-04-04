# HYDROGEN

🚨 **CRITICAL: `make trial` MUST PASS** 🚨
Run from /src/. Requires:

1. No warnings/errors
2. Shutdown test passes
3. Only then done

## Project

```json
{"name":"Hydrogen","lang":"C","type":"REST API/WebSocket Server","arch":"Multithreaded",
"tech":["POSIX","HTTP/WS","mDNS","Queues","JSON","SQL","OIDC","OpenSSL","Postgres"],
"critical":["ErrorHandling","ThreadSafety"]}
```

## Subsystem Order

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

trial: Required, clean + tests
debug: Debug symbols
perf: Optimized
release: Production
valgrind: Memory check
default: Standard