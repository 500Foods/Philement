# 🚀 TERMINAL SUBSYSTEM IMPLEMENTATION PLAN

---

# 📖 REQUIRED PREPARATION FOR IMPLEMENTATION

## Essential Reading (Must Review Before Starting Implementation)

**REQUIRED: Complete understanding required**

- [**RECIPE.md**](./RECIPE.md) - Critical development guide with C programming requirements, build configurations, and Hydrogen development patterns
- [**README.md**](./README.md) - Project overview, architecture, and testing status information
- [**tests/README.md**](./tests/README.md) - Complete testing framework documentation and test suite structure

**RECOMMENDED: Understanding context and inspiration**

- [**STRUCTURE.md**](./STRUCTURE.md) - Complete file organization and directory structure documentation
- [**docs/web_socket.md**](./docs/web_socket.md) - WebSocket implementation patterns (test_23_websockets.sh reference)
- [**docs/api.md**](./docs/api.md) - API structure and documentation approaches (swagger endpoint patterns)
- [**docs/data_structures.md**](./docs/data_structures.md) - Core data structure patterns and conventions
- [**cmake/README.md**](./cmake/README.md) - Build system configuration and requirements

---

## 🔍 CURRENT PROJECT CONTEXT (Distilled from Analysis)

### Hydrogen System Architecture Essentials

- **C Programming Style**: Strict following of RECIPE.md requirements (GOTO-free, function prototypes, commenting conventions)
- **Subsystem Pattern**: Launch/landing initialization cycle (Registry → Payload → Threads → Network → Database → Logging → WebServer → API → Swagger → WebSocket → Terminal)
- **Configuration Management**: JSON files with `${ENV_VAR}` substitution, environment-aware defaults
- **Memory Management**: Comprehensive cleanup required, no leaks (test_11_leaks_like_a_sieve validates)
- **Logging System**: Global logging with subsystem tags (SR_TERMINAL for terminal subsystem)
- **Error Handling**: Consistent error codes and cleanup patterns throughout codebase

### Existing Similar Subsystems (Reference Implementation Patterns)

- **Swagger**: Closest pattern - payload-based static file serving with dynamic content generation (swagger.json)
- **WebSocket**: Concurrent session management and message framing patterns (test_23 focuses here)
- **API**: Configuration parsing and JSON handling (multiple endpoints with shared config)
- **WebServer**: Request routing and file serving with MHD (compression, caching, CORS patterns)

### Current Terminal Configuration State

```json
{
  "Terminal": {
    "Enabled": true,
    "WebPath": "/terminal",
    "ShellCommand": "/bin/bash",
    "MaxSessions": 4,
    "IdleTimeoutSeconds": 600,
    "BufferSize": 4096
  }
}
```

*Existing config structure - requires WebRoot and CORS field additions*

### Build and Test Status (From Latest Test Results)

- **28/28 tests currently passing** (100% success rate)
- **No compilation failures** or major integration issues
- **Payload generation system** working (Swagger UI successfully encrypted/packageable)
- **WebServer core** stable (handles multiple endpoints safely)
- **WebSocket infrastructure** robust (handles concurrent connections)

---

## 🧠 KEY TECHNICAL KNOWLEDGE REQUIREMENTS

### Essential Understanding (Must Have Before Implementation)

- **xterm.js Ecosystem**: Core library + addons (attach, fit, webgl, etc.), WebSocket connection handling, terminal dimensions
- **PTY System Calls**: forkpty/fork/exec, signal propagation, TTY setup, environment variables
- **WebSocket Protocol**: Message framing (text/binary), ping/pong, connection lifecycle, error states
- **MicroHTTPd Integration**: MHD response handlers, request processing, thread safety, compression
- **C Memory Management**: Comprehensive allocation/deallocation, avoiding leaks, error path cleanup

### Important Background Reference Patterns

- **MHD Request Handler Pattern**: `enum MHD_Result handle_request(...)` with connection inspection
- **Async Signal Safety**: `volatile sig_atomic_t` for shutdown flags, safe cleanup routines
- **Thread Safety Patterns**: Mutex usage (follow existing session handling approaches)
- **Payload System**: Brotli compression, RSA+AES hybrid encryption, tar extraction logic

---

## 🏗️ ARCHITECTURAL DECISIONS ALREADY MADE

### Core Implementation Approach

- **Pattern Followed**: Reuse Swagger UI subsystem patterns for payload and endpoint handling
- **Technology Stack**: xterm.js frontend + C backend PTY management + WebSocket data flow
- **Initialization Strategy**: Launch/landing pattern with cleanup tracking and PID monitoring
- **Configuration Philosophy**: Sensible defaults with environment variable substitution
- **Testing Methodology**: Blackbox/integration testing (build on Test 22/23 patterns)

### Major Technical Decisions Resolved

- **Shell Choice**: zsh with OhMyZsh, server environment inheritance (customizable via config)
- **Session Limits**: 10 concurrent connections (configurable default, large buffer sizes)
- **Process Management**: Server-owned PTY processes, comprehensive cleanup with kill -9
- **Frontend Architecture**: HTML/JavaScript with xterm.js, real-time WebSocket updates
- **Backend Integration**: MidnightCommander-style PTY handling with signal propagation

### Security and Compatibility Scope

- **V1 Priority**: Functional implementation first, security hardening in V2
- **Browser Support**: Firefox/Chrome/Safari desktop/mobile baseline (xterm.js defines minimum versions)
- **Network Compatibility**: Standard WebSocket protocols, nginx proxy-compatible
- **Configuration Flexibility**: PAYLOAD:/ paths, filesystem paths, environment variable substitution

---

## 🎯 IMPLEMENTATION PREPARATION CHECKLIST

### Required Development Environment

- [ ] C compilation with all Hydrogen dependencies (see SETUP.md)
- [ ] MicroHTTPd development understanding (see cmake CMakeLists.txt)
- [ ] Testing framework familiarity (see tests/test_00_all.sh patterns)
- [ ] Build system operation (see cmake/README.md and RECIPE.md `mkt` process)
- [ ] Bash scripting for payload generation (see payloads/*.sh patterns)

### Suggested Code Review Priority Order

1. **src/swagger/** - Reference implementation for file serving patterns
2. **src/webserver/** - Endpoint routing and request handling approaches
3. **src/config/** - JSON parsing and environment variable substitution
4. **tests/test_22_swagger.sh** - Parallel testing patterns and retry logic
5. **tests/test_23_websockets.sh** - WebSocket connection and messaging patterns
6. **tests/test_18_signals.sh** - Process lifecycle and signal handling techniques

### Development Workflow Reminders

- **Critical Build Rule**: `mkt` (extras/make-trial.sh) is ONLY supported build method
- **Compilation Errors**: Fix with `mkt` before any commits or completion
- **Code Standards**: Follow RECIPE.md C requirements (prototypes, commenting, GOTO-free)
- **Documentation**: Update docs on functionality completion (following Structure.md patterns)
- **Testing**: Run `mkt` successfully before full test suite validation

---

# 🚀 IMPLEMENTATION GUIDELINES FOR FUTURE SESSIONS

## Phase-by-Phase Implementation Strategy

- **Phase 1**: Configuration - Build on existing config patterns, add new fields carefully
- **Phase 2**: Payload - Study swagger-generate.sh pattern, adapt for xterm.js files
- **Phase 3**: Endpoint - Reuse swagger request handler structure with path modifications
- **Phase 4**: Testing - Mirror Test 22 structure with terminal-specific validations
- **Phase 5**: Terminal Core - Follow existing subsystem launch/landing patterns
- **Phase 6**: Integration - Final build system cleanup and documentation updates

## Common Pitfalls to Avoid

- **Memory Leaks**: All allocated memory must have corresponding free() paths
- **Signal Safety**: Non-async-signal-safe functions in signal handlers
- **Process Cleanup**: Zombie processes from untracked PTY child processes
- **Thread Contentiousness**: Concurrent access to session structures
- **WebSocket Framing**: Incomplete message handling or connection state assumptions
- **Testing Intermittency**: Race conditions in test execution between server startup/requests

## Relationship to Existing Code

- **Modularity**: Each phase modifies/adds files without breaking existing functionality
- **Consistency**: Follow existing patterns (naming, structure, error handling)
- **Scope**: V1 focuses on functional implementation, V2 for security/performance optimization
- **Independence**: Terminal subsystem should work without impacting other Hydrogne features

---

## 📝 QUICK START DEVELOPMENT WORKFLOW (Per Session)

### Session Preparation (2-3 minutes)

1. Review TERMINAL_PLAN.md for current progress status
2. Identify specific phase/task to work on from implementation roadmap
3. Run `mkt` to confirm build system functional
4. Identify specific files to modify/create

### During Development (Per Change)

1. Implement minimal changes with clear commit objectives
2. Run `mkt` after each C file modification
3. Fix linting/compilation errors immediately
4. Test core functionality manually when possible

### Session Wrap-up (5-10 minutes)

1. Document changes made in TERMINAL_PLAN.md progress tracking
2. Run `mkt` final verification
3. Create comprehensive test case for new functionality
4. Update relevant documentation (add to docs/README.md if needed)

---

## Overview

Implementation of xterm.js-based terminal subsystem for Hydrogen, providing web-based terminal access via WebSocket-connected PTY processes.

## 🎯 Core Architecture

### Subsystem Design

- **Web Application**: xterm.js terminal emulator served from payload
- **Backend Communication**: WebSocket bidirectional data flow
- **Process Management**: PTY spawn/management with session lifecycle
- **Configuration**: Per-subsystem WebRoot (PAYLOAD:/ or filesystem path)

### Key Components

1. **Payload Generation**: Download and package xterm.js assets (including resize addon)
2. **Web Server Integration**: Endpoint registration with configurable paths
3. **PTY Management**: Process spawning, I/O handling, cleanup (with PID tracking)
4. **WebSocket Protocol**: Real-time bidirectional communication
5. **Testing Framework**: Comprehensive validation with test artifacts
6. **Launch/Landing**: Subsystem initialization and shutdown patterns (following webserver)

---

## 📁 WORK DIRECTORY STRUCTURE

```
elements/001-hydrogen/hydrogen/
├── RECIPE.md                                    # Existing project guide
├── TERMINAL_PLAN.md                             # This plan document
├── payloads/
│   ├── payload-generate.sh                      # Updated to include terminal
│   └── terminal-generate.sh                     # NEW: xterm.js download/packaging
├── src/
│   ├── config/
│   │   ├── config_defaults.c                    # Updated: PAYLOAD:/ default WebRoots
│   │   ├── config_swagger.h                     # Updated: WebRoot field
│   │   ├── config_terminal.h                    # Updated: WebRoot field
│   │   └── ...
│   ├── swagger/                                 # Reference implementation
│   │   ├── swagger.c                            # Updated for WebRoot support
│   │   └── swagger.h
│   ├── terminal/                                # NEW SUBSYSTEM
│   │   ├── terminal.c                           # Main terminal logic
│   │   ├── terminal.h                           # Public API & types
│   │   ├── terminal_pt-shell.c                  # PTY process management
│   │   ├── terminal_websocket.c                 # WebSocket communication
│   │   └── README.md                            # Terminal subsystem docs
│   ├── launch/
│   │   └── launch_terminal.c                    # NEW: Terminal subsystem initialization
│   ├── landing/
│   │   └── landing_terminal.c                   # NEW: Terminal subsystem shutdown
│   └── webserver/
│       └── web_server_core.c                   # Updated endpoint registration
├── tests/artifacts/
│   ├── swagger/                                 # Test files for WebRoot validation
│   │   ├── index.html                           # Marker: "Test Swagger Page"
│   │   └── swagger.json                         # Minimal API spec
│   ├── terminal/                                # Test files for terminal validation
│   │   ├── index.html                           # Marker: "Test Terminal Page"
│   │   └── xterm-test.html                      # xterm.js test interface
│   └── ...
├── tests/
│   ├── configs/
│   │   ├── hydrogen_test_22_swagger_webroot.json    # NEW: WebRoot test config
│   │   └── hydrogen_test_26_terminal_webroot.json   # NEW: Terminal webroot config
│   ├── test_22_swagger.sh                           # Updated for WebRoot testing
│   └── test_26_terminal.sh                          # NEW: Terminal test framework
├── build/cmake/
│   ├── CMakeLists.txt                             # Updated: Terminal subsystem
│   └── CMakePresets.json                          # Updated: Build configuration
└── ...
```

---

## 🎯 DETAILED IMPLEMENTATION PHASES

## Phase 1: Configuration Infrastructure - ✅ COMPLETED SUCCESSFULLY

### 1.1 ✅ COMPLETED: Update Configuration Structures

**EXTENDED IMPLEMENTATION**: Added comprehensive CORS support across all subsystems

```c
// src/config/config_swagger.h - Add WebRoot and CORS fields
typedef struct {
    bool enabled;                    // Already exists
    char* prefix;                   // Already exists
    bool payload_available;         // Already exists
    SwaggerMetadata metadata;       // Already exists
    SwaggerUIOptions ui_options;    // Already exists

    char* webroot;                  // ✅ IMPLEMENTED: PAYLOAD:/swagger or /filesystem/path
    char* cors_origin;              // ✅ IMPLEMENTED: Per-subsystem CORS override
} SwaggerConfig;

// src/config/config_terminal.h - Add WebRoot and CORS fields
typedef struct TerminalConfig {
    bool enabled;                   // Already exists
    char* web_path;                // Already exists
    char* shell_command;           // Already exists
    int max_sessions;              // Already exists
    int idle_timeout_seconds;      // Already exists

    char* webroot;                 // ✅ IMPLEMENTED: PAYLOAD:/terminal or /filesystem/path
    char* cors_origin;             // ✅ IMPLEMENTED: Per-subsystem CORS override
} TerminalConfig;

// src/config/config_webserver.h - Add global CORS field
typedef struct WebServerConfig {
    // ... existing WebServer config fields ...
    char* cors_origin;             // ✅ IMPLEMENTED: Global CORS default "*" with subsystem overrides possible
} WebServerConfig;

// BONUS: src/config/config_api.h - Add API CORS support
typedef struct APIConfig {
    bool enabled;         // Already exists
    char* prefix;         // Already exists
    char* jwt_secret;     // Already exists
    char* cors_origin;    // ✅ BONUS: CORS support for API endpoints
} APIConfig;
```

### 1.2 ✅ COMPLETED: Update Default Configuration & CORS Architecture

**IMPLEMENTED CORS HIERARCHY**: WebServer global → Subsystem overrides

```c
// src/config/config_defaults.c - Updated defaults with CORS "*" defaults
void initialize_config_defaults_swagger(AppConfig* config) {
    config->swagger.enabled = true;           // Already exists
    config->swagger.prefix = strdup("/apidocs"); // Already exists
    // ... other existing fields ...
    config->swagger.webroot = strdup("PAYLOAD:/swagger");     // ✅ IMPLEMENTED
    config->swagger.cors_origin = strdup("*");               // ✅ IMPLEMENTED: Allow all origins default
}

void initialize_config_defaults_terminal(AppConfig* config) {
    config->terminal.enabled = true;                             // Already exists
    // ... other existing fields ...
    config->terminal.webroot = strdup("PAYLOAD:/terminal");     // ✅ IMPLEMENTED
    config->terminal.cors_origin = strdup("*");                 // ✅ IMPLEMENTED: Allow all origins default
}

void initialize_config_defaults_webserver(AppConfig* config) {
    // ... existing WebServer fields ...
    config->webserver.cors_origin = strdup("*");  // ✅ IMPLEMENTED: Global CORS default
}

void initialize_config_defaults_api(AppConfig* config) {
    // ... existing API fields ...
    config->api.cors_origin = strdup("*");  // ✅ BONUS: API CORS support
}
```

### 1.3 ✅ COMPLETED: Update JSON Configuration Parsing & Processing

**FULL CORS PARSING IMPLEMENTATION**:

- **config_swagger.c**: Parse `Swagger.WebRoot` & `Swagger.CORSOrigin` fields ✅
- **config_terminal.c**: Parse `Terminal.WebRoot` & `Terminal.CORSOrigin` fields ✅
- **config_webserver.c**: Parse `WebServer.CORSOrigin` field ✅
- **config_api.c**: Parse `API.CORSOrigin` field ✅
- **Environment variable substitution** on all CORS/WebRoot fields ✅
- **Hierarchical CORS resolution** (global override by subsystem) ✅
- **Enhanced dump functions** displaying resolved CORS origins ✅
- **Memory cleanup** for all new string fields ✅

### 1.4 ✅ BONUS: Comprehensive CORS Architecture

**CORS OVERRIDE CHAIN**: `*` (Global Default) ← `https://example.com` (Subsystem Override)

```
WebServer → Global "*/null" → Swagger (Can override global) → API (Can override global) → Terminal (Can override global)
│                     │                          │                        │                        │
└─────────────────────┴──────────────────────────┴────────────────────────┴────────────────────────┘
                                      ↓
                                     User
```

### 1.5 ✅ VERIFICATION: Build & Configuration Testing

**BUILD SUCCESS**: ✅ `mkt` compilation passes cleanly
**CONFIGURATION LOADING**: ✅ Server starts without hanging, processes all defaults successfully
**CORS DEFAULTS**: ✅ All subsystems properly initialized with `"*"` CORS origins
**MEMORY MANAGEMENT**: ✅ All new string fields properly allocated and freed
**CONFIG PARSING**: ✅ JSON fields correctly parsed with PROCESS_ macros

## Phase 2: Payload Generation for xterm.js

### 2.1 ✅ COMPLETED: Create terminal-generate.sh

Following `swagger-generate.sh` pattern:

- ✅ Download latest xterm.js from JSDelivr CDN (v5.3.0)
- ✅ Extract essential files: core library (276KB), CSS (5.4KB), addons (attach: 1.7KB, fit: 1.5KB)
- ✅ Create HTML templates with terminal initialization and WebSocket integration
- ✅ Compress with brotli, encrypt in payload system (ready for integration)
- ✅ Files created: xterm.js, xterm.css, xterm-addon-attach.js, xterm-addon-fit.js, terminal.html, terminal.css, xtermjs_version.txt

### 2.2 Update payload-generate.sh

- Add terminal tarball creation alongside swagger
- Call terminal-generate.sh in workflow
- Structure: `terminal/xterm.js`, `terminal/xterm.css`, `terminal/index.html`

### 2.3 HTML Template Structure

```html
<!-- payloads/terminal/terminal.html -->
<!DOCTYPE html>
<html>
<head>
    <title>Hydrogen Terminal</title>
    <link rel="stylesheet" href="xterm.css">
</head>
<body>
    <div id="terminal"></div>

    <script src="xterm.js"></script>
    <script src="xterm-addon-attach.js"></script>
    <script src="xterm-addon-fit.js"></script>

    <script>
        const term = new Terminal({
            cursorBlink: true,
            theme: 'dark'
        });

        const fitAddon = new FitAddon.FitAddon();
        term.loadAddon(fitAddon);

        term.open(document.getElementById('terminal'));
        fitAddon.fit();
        term.write('\r\n\x1b[32mHydrogen Terminal Ready\x1b[0m\r\n$ ');

        // WebSocket connection to PTY backend
        const ws = new WebSocket(`ws://${location.host}/terminal/ws`);
        const attachAddon = new AttachAddon.AttachAddon(ws);
        term.loadAddon(attachAddon);
    </script>
</body>
</html>
```

## Phase 3: File Serving with WebRoot Support - ✅ **COMPLETED SUCCESSFULLY**



## Phase 4: Terminal Subsystem Implementation

### 4.1 Core Terminal Structure

```c
// src/terminal/terminal.h
typedef struct TerminalSession {
    int pty_master_fd;              // PTY master file descriptor
    pid_t child_pid;                // Child process PID (tracked for shutdown cleanup)
    uint8_t* io_buffer;             // Data buffer
    size_t buffer_size;             // Buffer capacity
    time_t last_activity;           // For timeout management
    bool active;                    // Session state
    uint32_t session_id;            // Unique identifier
} TerminalSession;

typedef struct TerminalConfig {
    // ... config fields from Phase 1 ...

    // Runtime session management
    TerminalSession* sessions;      // Array of active PTY sessions
    size_t session_count;          // Current number of active sessions
    size_t max_sessions;           // Configured maximum sessions
    pid_t* child_pids;             // Array of all child process PIDs for shutdown
    size_t pid_count;              // Number of tracked child processes
    pthread_mutex_t session_lock;   // Thread safety for session operations

    // Shutdown cleanup tracking
    bool shutting_down;            // Flag to prevent new sessions during shutdown
    volatile sig_atomic_t cleanup_complete; // Atomic flag for shutdown coordination
} TerminalConfig;

// PID tracking functions
void track_child_pid(TerminalConfig* config, pid_t pid);
void cleanup_child_processes(TerminalConfig* config);
void remove_child_pid(TerminalConfig* config, pid_t pid);
```

### 4.2 PTY Management

```c
// src/terminal/terminal_pt-shell.c

// Process lifecycle
pid_t spawn_pty_shell(const char* shell_cmd,
                      int* master_fd,
                      struct termios* term_settings);

// I/O handling
enum MHD_Result handle_pty_io(TerminalSession* session,
                             struct MHD_Connection* connection);

// Cleanup
void terminate_session(TerminalSession* session,
                       bool force_kill);
```

### 4.3 WebSocket Implementation

```c
// src/terminal/terminal_websocket.c

// WebSocket upgrade handling
enum MHD_Result upgrade_websocket_connection(struct MHD_Connection* connection,
                                           TerminalConfig* config);

// Message framing
bool send_websocket_data(int client_fd,
                        const uint8_t* data,
                        size_t data_len,
                        bool is_text);

// Real-time data flow
void handle_websocket_message(const uint8_t* payload,
                             size_t payload_len,
                             int master_fd);
```

## Phase 5: Testing Infrastructure

### 5.1 Test Artifacts Creation

```
tests/artifacts/swagger/index.html:
    <!DOCTYPE html>
    <html><body><h1>HYDROGEN_SWAGGER_TEST_MARKER</h1></body></html>

tests/artifacts/terminal/terminal.html:
    <!DOCTYPE html>
    <html><body><h1>HYDROGEN_TERMINAL_TEST_MARKER</h1></body></html>
```

### 5.2 Test Configuration Files

```
tests/configs/hydrogen_test_22_swagger_webroot.json:
    {
        "Swagger": {
            "WebRoot": "tests/artifacts/swagger",  // Relative to server working directory
            "Enabled": true
        }
    }

tests/configs/hydrogen_test_26_terminal_webroot.json:
    {
        "Terminal": {
            "WebRoot": "tests/artifacts/terminal",  // Relative to server working directory
            "Enabled": true
        }
    }
```

### 5.3 Test Script Updates

- **test_22_swagger.sh**: Add WebRoot configuration tests + Range request testing
- **test_26_terminal.sh**: Complete terminal functionality testing + Range request testing (following parallel test patterns)
- Both scripts validate specific markers from artifacts directory
 PRESERVE Range request subtest in Test 26 structure

## Phase 6: Integration and Build System

### 6.1 Launch/Landing Files Creation

Following webserver pattern conventions:

**src/launch/launch_terminal.c**:

```c
// Terminal subsystem initialization following launch_webserver.c pattern
bool init_terminal_support(AppConfig* config) {
    log_this(SR_TERMINAL, "Initializing terminal subsystem", LOG_LEVEL_STATE);

    // Allocate session tracking arrays
    config->terminal.sessions = calloc(config->terminal.max_sessions, sizeof(TerminalSession));
    config->terminal.child_pids = calloc(config->terminal.max_sessions, sizeof(pid_t));

    if (!config->terminal.sessions || !config->terminal.child_pids) {
        log_this(SR_TERMINAL, "Failed to allocate session tracking", LOG_LEVEL_ERROR);
        return false;
    }

    // Initialize mutex for thread safety
    if (pthread_mutex_init(&config->terminal.session_lock, NULL) != 0) {
        log_this(SR_TERMINAL, "Failed to initialize session mutex", LOG_LEVEL_ERROR);
        return false;
    }

    log_this(SR_TERMINAL, "Terminal subsystem initialized successfully", LOG_LEVEL_STATE);
    return true;
}
```

**src/landing/landing_terminal.c**:

```c
// Terminal subsystem shutdown following landing_webserver.c pattern
void cleanup_terminal_support(AppConfig* config) {
    log_this(SR_TERMINAL, "Shutting down terminal subsystem", LOG_LEVEL_STATE);

    // Set shutdown flag to prevent new sessions
    config->terminal.shutting_down = true;

    // Kill all tracked child processes
    cleanup_child_processes(&config->terminal);

    // Cleanup session arrays
    if (config->terminal.sessions) {
        free(config->terminal.sessions);
        config->terminal.sessions = NULL;
    }

    if (config->terminal.child_pids) {
        free(config->terminal.child_pids);
        config->terminal.child_pids = NULL;
    }

    // Destroy mutex
    pthread_mutex_destroy(&config->terminal.session_lock);

    log_this(SR_TERMINAL, "Terminal subsystem shutdown complete", LOG_LEVEL_STATE);
}
```

### 6.2 Subsystem Registration

Update `src/launch/launch.c` to include terminal in subsystem sequence (following existing pattern):

```c
Subsystem subsystems[] = {
    {"Registry", init_registry, cleanup_registry, SR_REGISTRY},
    {"Payload", init_payload, cleanup_payload, SR_PAYLOAD},
    {"Threads", init_threads, cleanup_threads, SR_THREADS},
    {"Network", init_network, cleanup_network, SR_NETWORK},
    {"Database", init_database, cleanup_database, SR_DATABASE},
    {"Logging", init_logging, cleanup_logging, SR_LOGGING},
    {"WebServer", init_webserver, cleanup_webserver, SR_WEBSERVER},
    {"API", init_api, cleanup_api, SR_API},
    {"Swagger", init_swagger, cleanup_swagger, SR_SWAGGER},
    {"WebSocket", init_websocket, cleanup_websocket, SR_WEBSOCKET},
    {"Terminal", init_terminal_support, cleanup_terminal_support, SR_TERMINAL},  // NEW
    // ... remaining subsystems ...
};
```

---

## 📊 SUCCESS CRITERIA & VALIDATION

### Configuration Working

- [ ] PAYLOAD:/swagger serves from payload files
- [ ] PAYLOAD:/terminal serves xterm.js from payload
- [ ] /filesystem/path serves from local filesystem
- [ ] Config validation handles invalid paths

### Terminal Functionality

- [ ] `/terminal` endpoint serves HTML interface
- [ ] xterm.js loads and initializes correctly
- [ ] WebSocket connects to backend (/terminal/ws)
- [ ] PTY processes spawn with specified shell
- [ ] Bidirectional I/O flow (terminal ↔ shell)
- [ ] Session management and cleanup
- [ ] Multiple concurrent sessions work

### Testing Framework

- [ ] Test artifacts contain distinct markers
- [ ] WebRoot configuration switches work
- [ ] Test 22 validates swagger WebRoot switching
- [ ] Test 26 validates terminal functionality
- [ ] All tests pass in CI/CD pipeline

---

## ⚠️ RISK ANALYSIS & MITIGATION

### High-Risk Areas

**1. PTY Management Complexity**

- **Risk**: Process lifecycle, signal handling, zombie processes
- **Mitigation**:
  - Comprehensive error handling and logging
  - Study existing system() and fork() usages in codebase
  - Implement timeout and force-kill mechanisms
  - Reference test_18_signals.sh patterns

**2. WebSocket Protocol Implementation**

- **Risk**: Message fragmentation, binary/text confusion, connection drops
- **Mitigation**:
  - Follow test_23_websockets.sh patterns
  - Implement proper framing and heartbeat
  - Add connection state validation

**3. Session Management & Concurrency**

- **Risk**: Race conditions, resource leaks, cleanup failures
- **Mitigation**:
  - Use existing thread safety patterns from logging system
  - Implement session tracking with atomic operations
  - Add timeout and cleanup on graceful shutdown

**4. Security Considerations (Lower Priority - V2 Focus)**

- **Note**: Security features will be added in a later iteration after basic functionality is working
- **Initial Approach**: Basic resource limits and input validation with good defaults
- **Future V2 Enhancements**: Command injection protection, advanced access control, audit trails

### Performance Considerations

- PTY process overhead (use thread pooling)
- WebSocket message frequency limits
- Buffer size optimization
- Memory usage monitoring

---

## 🚀 IMPLEMENTATION ROADMAP

### Phase 1 (Configuration) - 2 days

- Update config structures and defaults
- JSON parsing for WebRoot fields
- Validation and error handling

### Phase 2 (Payload Generation) - 3 days

- Create terminal-generate.sh
- Integrate with main payload workflow
- Test packaging and encryption

### Phase 3 (File Serving) - 2 days - ✅ COMPLETED

- WebRoot path resolution ✅ IMPLEMENTED
- Update swagger system for new pattern ✅ IMPLEMENTED
- Endpoint routing updates ✅ IMPLEMENTED

### Phase 4 (Testing) - 3 days

- Create test artifacts and configurations
- Update test_22_swagger.sh
- Create test_26_terminal.sh
- Validation and debugging

### Phase 5 (Terminal Core) - 5 days

- PTY process management
- WebSocket communication
- Session lifecycle handling
- Error recovery mechanisms

### Phase 6 (Integration) - 2 days

- Build system updates (CMake)
- Documentation updates
- Final validation and testing

**Total Timeline**: ~12 working days remaining (Phase 3 already completed)

---

## ❓ POTENTIAL IMPLEMENTATION QUESTIONS & RISKS

### 🛠️ Phase 1: Configuration Deep Dive - UPDATED ANSWERS

**Critical Questions:**

- **✅ RESOLVED Path Resolution**: WebRoot paths are relative to WebServer's WebRoot (need to add WebServer.WebRoot field, default "" = current working directory). Tests set WebServer.WebRoot = "tests/artifacts"
- **✅ RESOLVED Environment Variables**: All config values support `${VAR}` substitution - no new work needed. Use `/terminal` and `/swagger` for test configs, `PAYLOAD:terminal`/`PAYLOAD:swagger` as defaults
- **✅ RESOLVED Validation Rules**: Skip validation for now
- **✅ RESOLVED Memory Management**: WebRoot needs to be freed during config reloads (just like PIDs are killed)
- **✅ RESOLVED Order Dependencies**: They can point to the same path - no problem

### 📦 Phase 2: Payload Generation Edge Cases - UPDATED ANSWERS

**Critical Questions:**

- **✅ RESOLVED Version Compatibility**: Pull latest xterm.js on script runs, handle breaking changes manually through deployment process
- **✅ RESOLVED Network Failures**: No API rate limiting concerns - use same approach as Swagger (GitHub clone via authorized account)
- **✅ RESOLVED File Integrity**: No checksum validation needed - assume GitHub clone legitimacy
- **✅ RESOLVED Addon Dependencies**: Pull current addon versions alongside xterm.js core - handle incompatibilities during deployment if they occur
- **✅ RESOLVED License Compliance**: No code changes in this effort - review xterm.js licenses separately outside payload generation

### 🌐 Phase 3: File Serving Implementation - UPDATED ANSWERS

**Critical Questions:**

- **✅ RESOLVED MIME Types**: Mirror Swagger approach (works well for similar content types)
- **✅ RESOLVED Path Precedence**: Unique filenames (terminal.html vs index.html) to avoid overlap
- **✅ RESOLVED Cache Headers**: Same caching and compression as Swagger
- **✅ RESOLVED CORS Policies**: Add global CORS config to WebServer (default "*") with optional per-subsystem settings
- **✅ RESOLVED Range Requests**: Implement custom range header support - microhttpd exposes headers but doesn't handle automatically. Add as separate subtest in Test 26 following Test 22 pattern.

### 💻 Phase 4: Terminal Core Deep Dives - UPDATED ANSWERS

**Critical Questions:**

- **✅ RESOLVED Shell Environment**: Use zsh with OhMyZsh, inherit server environment variables
- **✅ RESOLVED Process Ownership**: Server owns PTY processes with full permissions
- **✅ RESOLVED Signal Propagation**: Pass all signals through to PTY process (xterm.js handles)
- **✅ RESOLVED Resource Limits**: Add to config defaults, implement restriction later (similar to other limits)
- **✅ RESOLVED Session Identity**: Store PID + user string (default "User", OIDC integration for real usernames later)
- **✅ RESOLVED File Descriptors**: Handle like other resource limits (add to config defaults)
- **✅ RESOLVED Zombie Prevention**: Kill -9 all PTYs during shutdown, external script handles cleanup
- **✅ RESOLVED Termination Cleanup**: Kill -9 equivalent in landing code, flag issues
- **✅ RESOLVED Terminal Dimensions**: Handled by xterm.js plugins (FitAddon)
- **✅ RESOLVED Binary Data Handling**: Assume xterm.js handles if needed, focus on normal terminal I/O

### 🧪 Phase 5: Testing Implementation Details - UPDATED ANSWERS

**Critical Questions:**

- **✅ RESOLVED Process Cleanup**: Register PTYs in terminal struct to detect completion between test runs
- **✅ RESOLVED WebSocket Testing**: No unit testing/mocking - Test 26 is blackbox/integration test focusing on basic functionality
- **✅ RESOLVED CI/CD Compatibility**: No PTY testing in CI - focus on file serving validation (like Test 22 pattern)
- **✅ RESOLVED Race Conditions**: Not concerned with session management in initial testing
- **✅ RESOLVED Artifact Conflicts**: No overlap between build and test artifacts - separate folders

### 🏗️ Phase 6: System Integration Challenges - UPDATED ANSWERS

**Critical Questions:**

- **✅ RESOLVED CMake Dependencies**: Link `util` library for PTY support (standard requirement)
- **✅ RESOLVED Memory Leaks**: Existing memory leak test (test_11_leaks_like_a_sieve.sh) covers new code - defaults are fine
- **✅ RESOLVED Thread Safety**: Libwebsockets is single-threaded by design, assume handlers are safe - defaults are fine
- **✅ RESOLVED Performance Monitoring**: No initial tracking of PTY process resource usage - defaults are fine for V1
- **✅ RESOLVED Log Analysis**: Basic session start/end logging with optional command logging - defaults are fine

### 🚨 Security Implementation Questions (V2) - UPDATED ANSWERS

**Even for V1, we should consider:**

- **✅ RESOLVED Input Sanitization**: No input sanitization needed for V1 - defaults are fine
- **✅ RESOLVED Rate Limiting**: No rate limiting implemented in V1 - defaults are fine
- **✅ RESOLVED Command Audit**: Log commands if feasible, but default basic session logging is fine for V1
- **✅ RESOLVED Resource Quotas**: No resource limiting forced in V1 - handles via config defaults if needed

### 📋 Missing Specification Details - UPDATED ANSWERS

**Items needing clarification:**

1. **✅ RESOLVED Maximum concurrent sessions**: Set to 10 default in configs and test files (reasonable default)
2. **✅ RESOLVED Terminal buffer sizes**: Configure 5,000 lines or 50MB default (large reasonable values)
3. **✅ RESOLVED WebSocket ping/pong**: Yes, need explicit ping/pong keepalives (required for reliability)
4. **✅ RESOLVED Session idle timeout**: 10m idle timeout, 60m max total session time (configurable defaults)
5. **✅ RESOLVED Shell command validation**: Check shell existence during server startup (handles missing shell cases)
6. **✅ RESOLVED Browser compatibility**: Firefox, Chrome, Safari on desktop/mobile as minimum (sensible default)
7. **✅ RESOLVED xterm.js themes**: Use default theming, fine-tune later as needed (defaults are fine)

### 🔄 Integration Testing Questions - UPDATED ANSWERS

**End-to-end scenarios to consider:**

1. **✅ RESOLVED Browser → WebSocket → PTY → Shell**: Focus on getting HTML/JS file serving working first
2. **✅ RESOLVED Multi-tab terminal usage**: Get basic single-session working, then expand - defaults fine for now
3. **✅ RESOLVED Server restart with active sessions**: Honor termination/cleanup patterns already defined
4. **✅ RESOLVED Network connection loss**: Standard WebSocket reconnection patterns - defaults fine
5. **✅ RESOLVED Large data streams**: Standard terminal/file operation handling - defaults fine
6. **✅ RESOLVED Background processes**: Standard PTY/shell handling capabilities - defaults fine

### 🏗️ Additional Implementation Questions - UPDATED ANSWERS

#### Constants and Buffers - ✅ ALL RESOLVED (Defaults are fine)

- **✅ Buffer Sizes**: Use configurable defaults (5k lines, 50MB) - fine-tune during implementation
- **✅ Session IDs**: Use sequential counters with timestamp - sufficient for basic functionality
- **✅ Default Ports**: Use existing WebServer/WebSocket port configuration patterns - defaults fine
- **✅ File Permissions**: Follow existing payload file serving permissions patterns - defaults fine
- **✅ Maximum WebSocket Frame Size**: Use reasonable limits, enforce at message level - defaults fine

#### Error Handling and Recovery - ✅ ALL RESOLVED (Defaults are fine)

- **✅ WebSocket Disconnection**: Standard reconnect patterns, log events - defaults fine
- **✅ PTY Failure**: Log error, terminate session gracefully - defaults fine via existing error patterns
- **✅ Shell Not Found**: Config validation catches missing shells at startup - already resolved
- **✅ Memory Allocation Failures**: Standard allocation error handling like rest of codebase - defaults fine
- **✅ Thread Pool Exhaustion**: Let HTTP server handle thread limits as configured - defaults fine

#### Performance and Scalability - ✅ ALL RESOLVED (Defaults are fine)

- **✅ Concurrent Sessions**: Start with 10 limit, optimize performance incrementally - defaults fine
- **✅ WebSocket Frame Rate**: Basic message throttling if needed, observe first - defaults fine
- **✅ File Descriptor Limits**: Use system defaults, log when approaching limits - defaults fine
- **✅ CPU Overhead**: Standard polling frequency, non-blocking where possible - defaults fine

#### Interoperability - ✅ ALL RESOLVED (Defaults are fine)

- **✅ Alternative Shells**: zsh primary, support others via config - bash/fish defaults fine
- **✅ Terminal Capabilities**: TERM=xterm-256color standard - xterm.js handles most scenarios
- **✅ Browser Compatibility**: Firefox/Chrome/Safari baseline - already resolved defaults
- **✅ Network Proxies**: Standard WebSocket proxy support - nginx patterns work fine

#### Monitoring and Observability - ✅ ALL RESOLVED (Defaults are fine)

- **✅ Session Metrics**: Basic session start/end counts sufficient for V1 - defaults fine
- **✅ Performance Metrics**: Standard system monitoring patterns - existing logging works
- **✅ Health Checks**: Basic subprocess status checks - defaults fine
- **✅ Log Correlation**: Use existing logging subsystem with SR_TERMINAL tag - defaults fine

#### Error Codes and Reporting - ✅ ALL RESOLVED (Defaults are fine)

- **✅ Standardized Error Codes**: Follow existing Hydrogen error patterns - consistent approach
- **✅ Client Error Display**: Standard HTTP error responses with logging - defaults fine
- **✅ Server Log Detail**: Appropriate log levels following existing patterns - defaults fine
- **✅ Recovery Procedures**: Manual intervention guides match system documentation - defaults fine

#### Version Compatibility - ✅ ALL RESOLVED (Defaults are fine)

- **✅ Hydrogen Version Changes**: Standard config validation handles version differences
- **✅ Browser Updates**: xterm.js version pulled handles browser compatibility - deployment manages
- **✅ OS Updates**: Standard PTY system calls, deployment handles OS differences
- **✅ External Dependencies**: Lock to compatible versions, update during deployment cycles

#### Deployment and Configuration - ✅ ALL RESOLVED (Defaults are fine)

- **✅ Multi-Instance Deployment**: Load balancers route based on WebSocket sessions - standard patterns
- **✅ User Session Mapping**: Basic IP-based identification sufficient for most use cases
- **✅ Persistent Sessions**: Not needed for V1, sessions ephemeral by design - revisit for V2
- **✅ Configuration Override**: Follow existing subsystem config override patterns - defaults fine

---

## 📚 REFERENCE ARCHITECTURE

The implementation follows these existing Hydrogen patterns:

1. **Configuration**: Like Swagger, terminal config uses JSON with env var substitution
2. **File Serving**: Uses existing MHD request routing and payload decryption
3. **Subsystem Organization**: Follows launch/landing pattern with init/cleanup
4. **Testing**: Leverages existing test framework with retry logic
5. **Build Integration**: CMake library targets with dependency linking

This ensures consistency with Hydrogen's established architecture while providing the terminal functionality required.
