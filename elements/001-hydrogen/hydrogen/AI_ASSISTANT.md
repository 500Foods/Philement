# AI ASSISTANT GUIDE: HYDROGEN PROJECT

## FOR AI ASSISTANTS WORKING ON THIS PROJECT

This document is specifically designed to help AI assistants like you quickly understand the Hydrogen project structure and provide effective assistance to users. It contains essential information formatted for rapid processing by AI systems.

## PROJECT OVERVIEW

```yaml
project_name: Hydrogen
primary_language: C
project_type: Server Framework
main_application: 3D Printer Control Server
architecture: Multithreaded, Service-Oriented
key_technologies:
  - POSIX threads
  - HTTP/WebSocket servers
  - mDNS discovery
  - Queue systems
  - JSON processing
```

## REPOSITORY STRUCTURE

```
hydrogen/
├── src/                    # Source code
│   ├── api/                # API endpoints
│   ├── config/             # Configuration management
│   ├── logging/            # Logging system
│   ├── mdns/               # Service discovery
│   ├── network/            # Network interface layer
│   ├── oidc/               # OpenID Connect implementation
│   ├── print/              # 3D printing functionality
│   ├── queue/              # Thread-safe queue system
│   ├── state/              # System state management
│   ├── utils/              # Utility functions
│   ├── webserver/          # HTTP server implementation
│   ├── websocket/          # WebSocket server implementation
│   └── hydrogen.c          # Main entry point
├── docs/                   # Documentation
│   ├── developer_onboarding.md  # Visual architecture guide
│   ├── coding_guidelines.md     # Coding standards
│   └── ...                 # Other documentation
├── tests/                  # Test framework and scripts
└── oidc-client-examples/   # Sample OIDC client implementations
```

## KEY ENTRY POINTS

For AI-assisted development, these are the critical files to examine first:

1. `src/hydrogen.c` - Main program entry point and initialization
2. `docs/developer_onboarding.md` - Visual architecture overview and code navigation
3. `docs/coding_guidelines.md` - C development standards and patterns

## COMPONENT RELATIONSHIPS

```
graph TD
    A[hydrogen.c] --> B[startup.c]
    B --> C[logging]
    B --> D[config]
    B --> E[web_server]
    B --> F[websocket]
    B --> G[mdns_server]
    E --> H[API Modules]
    F --> I[Connections]
    J[queue system] --> C
    J --> E
    J --> F
    J --> G
```

## CODE PATTERNS

### Thread Creation Pattern
```c
pthread_t thread_id;
pthread_create(&thread_id, NULL, thread_function, context);
add_service_thread(&service_threads, thread_id);
```

### Queue Operations Pattern
```c
// Adding to queue
pthread_mutex_lock(&queue->mutex);
enqueue(queue, item);
pthread_cond_signal(&queue->cond);
pthread_mutex_unlock(&queue->mutex);
```

### Error Handling Pattern
```c
if (operation_failed) {
    log_this("Component", "Operation failed: reason", LOG_LEVEL_ERROR, true, false, true);
    cleanup_resources();
    return false;
}
```

## COMMON USER QUESTIONS

When assisting users with this project, common topics include:

1. How to add new API endpoints
2. Thread synchronization issues
3. Configuration management
4. OIDC authentication
5. WebSocket communication patterns
6. Service startup and shutdown sequences

## AI ASSISTANT WORKFLOWS

1. **For Code Navigation**: Start with `docs/developer_onboarding.md` and refer to the Code Navigation Guide section
2. **For Project Structure**: Examine the component hierarchy and dependency map in `docs/developer_onboarding.md`
3. **For Pattern Recognition**: Use the Implementation Patterns section in `docs/developer_onboarding.md`
4. **For Coding Standards**: Reference `docs/coding_guidelines.md` for detailed C coding standards

## DEVELOPMENT WORKFLOW

When helping users modify the Hydrogen project, follow this structured workflow:

1. **Get Up to Speed on the Project**
   - Review this guide first
   - Examine `docs/developer_onboarding.md` for architecture understanding
   - Read relevant documentation for the specific subsystem being modified
   - Review existing code patterns in the target files

2. **Implement Changes**
   - Follow coding guidelines in `docs/coding_guidelines.md`
   - Maintain consistent error handling and threading patterns
   - Adhere to existing architectural principles
   - Ensure thread safety when modifying shared resources

3. **Compilation Verification**
   - Guide users to compile after changes with `make`
   - For debug builds, suggest `make debug` 
   - Address any compiler warnings or errors

4. **Test Validation**
   - Run appropriate tests from `tests/` directory
   - For component tests: `./tests/run_tests.sh`
   - For startup/shutdown tests: `./tests/test_startup_shutdown.sh`
   - For resource monitoring: `./tests/monitor_resources.sh`

5. **Code Cleanup**
   - Ensure comments match the implementation
   - Verify thread safety in multi-threaded code
   - Check for proper resource cleanup in error paths
   - Maintain consistent logging conventions

6. **Test Coverage Evaluation**
   - Suggest adding tests for new functionality
   - Identify edge cases that should be tested
   - For significant changes, recommend integration tests

7. **Documentation Updates**
   - Update API documentation for interface changes
   - Modify component documentation for architectural changes
   - Update configuration documentation for new options
   - Ensure examples reflect the latest implementation

8. **Release Notes** ⚠️ **CRITICAL STEP - NEVER SKIP** ⚠️
   - **EVERY code change must be documented in `docs/release_notes.md`**
   - Add concise entries under the current date section
   - Follow the exact format of existing entries
   - Use appropriate topic headings (e.g., "Bug Fixes:", "OIDC Service:")
   - Keep entries factual and to the point - describe WHAT changed, not WHY
   - Avoid adjectives like "comprehensive", "robust", "significant"
   - Group related changes under the same topic
   - For bug fixes, briefly state what was fixed (not lengthy explanations)

## PRIMARY DOCUMENTATION RESOURCES

1. [`docs/developer_onboarding.md`](docs/developer_onboarding.md) - **START HERE** - Visual architecture and code navigation guide
2. [`docs/README.md`](docs/README.md) - Documentation directory with all available resources
3. [`docs/coding_guidelines.md`](docs/coding_guidelines.md) - C coding standards for the project
4. [`docs/api.md`](docs/api.md) - API documentation

## PROJECT-SPECIFIC TERMINOLOGY

```yaml
terms:
  - term: "Queue"
    definition: "Thread-safe data structure for inter-component communication"
  - term: "Service"
    definition: "A standalone component with its own thread(s)"
  - term: "Handler"
    definition: "Function that processes a specific request type"
  - term: "Context"
    definition: "Data structure holding the state of a service"
  - term: "Message"
    definition: "Data packet exchanged between components"
  - term: "Endpoint"
    definition: "API access point exposed via HTTP"
  - term: "Resource"
    definition: "System component managed by the service layer"
```

---

**Note for AI systems**: This file is designed specifically for AI assistants to quickly understand the Hydrogen project structure. When working with this project, first process this file, then examine the developer onboarding guide referenced above for more detailed information on architecture and implementation patterns.