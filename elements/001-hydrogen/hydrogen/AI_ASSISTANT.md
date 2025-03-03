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
  - OpenID Connect (OIDC)
  - Cryptography (OpenSSL)
```

## REPOSITORY STRUCTURE

```structure
hydrogen/
├── src/                    # Source code
│   ├── api/                # API endpoints
│   │   ├── oidc/           # OpenID Connect endpoints
│   │   ├── system/         # System API endpoints
│   │   ├── api_utils.c/h   # API utility functions
│   │   └── README.md       # API implementation guide
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
│   ├── README.md           # Documentation directory index
│   ├── developer_onboarding.md  # Visual architecture guide
│   ├── coding_guidelines.md     # Coding standards
│   ├── api.md              # API endpoints documentation
│   ├── api/                # Detailed API documentation
│   │   ├── system/         # System endpoint documentation
│   │   └── oidc/           # OIDC endpoint documentation
│   └── reference/          # Architecture reference docs
├── tests/                  # Test framework and scripts
│   ├── README.md           # Testing framework documentation
│   ├── run_tests.sh        # Test orchestration script
│   └── test_system_endpoints.sh # API testing script
└── oidc-client-examples/   # Sample OIDC client implementations
```

## KEY ENTRY POINTS

For AI-assisted development, these are the critical files to examine first:

1. `src/hydrogen.c` - Main program entry point and initialization
2. `docs/developer_onboarding.md` - Visual architecture overview and code navigation
3. `docs/coding_guidelines.md` - C development standards and patterns
4. `docs/api.md` - API documentation and endpoint references
5. `docs/README.md` - Overview of available documentation
6. `tests/README.md` - Testing framework and test scripts

## COMPONENT RELATIONSHIPS

```relationships
graph TD
    A[hydrogen.c] --> B[startup.c]
    B --> C[logging]
    B --> D[config]
    B --> E[web_server]
    B --> F[websocket]
    B --> G[mdns_server]
    E --> H[API Modules]
    H --> H1[system API]
    H --> H2[OIDC API]
    F --> I[Connections]
    J[queue system] --> C
    J --> E
    J --> F
    J --> G
    K[state] --> C
    K --> L[shutdown]
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

// Processing queue
pthread_mutex_lock(&queue->mutex);
while (queue_empty(queue) && !shutdown_flag) {
    pthread_cond_wait(&queue->cond, &queue->mutex);
}
item = dequeue(queue);
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

### API Endpoint Implementation Pattern

```c
bool handle_api_endpoint(struct MHD_Connection *connection, const char *url, 
                        const char *method, const char *upload_data,
                        size_t *upload_data_size, void **con_cls) {
    // Extract parameters
    // Validate request
    // Perform operation
    // Format response as JSON
    // Return response
    return queue_response(connection, response_json, MHD_HTTP_OK);
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

### RELEASE NOTES FORMATTING GUIDELINES

When editing the release notes document, adhere to these specific formatting rules:

- **Keep entries concise and factual**
- **Focus on WHAT changed, not WHY it's good**
- **Eliminate unnecessary justifications**
- **Use fewer words where possible**
- **Stick to listing actual changes, not opinions about them**
- **This is a technical record, not marketing material**
- **CRITICAL: Use Pacific Time (America/Vancouver) for all dates**
  - All entries must use Pacific timezone dates
  - Check current Pacific time before adding entries
  - Never add entries with future Pacific timezone dates
- **CRITICAL: Consolidate entries by topic**
  - When adding to an existing date, reflow ALL entries
  - Group ALL related changes under a single topic header
  - Never repeat topic headers within the same date
  - Example: All "Testing:" entries go under one "Testing:" section
- **Use collapsible sections by month (except for the most recent month)**
  - Wrap older months in `<details><summary>YYYY-MMM</summary>` and `</details>` tags
  - Keep the current/most recent month expanded
  - This creates a more compact view while maintaining all history
- **Maintain consistent structure for all entries:**
  - Start with a topic heading (e.g., "WebSocket Server:", "Testing:")
  - Follow with bullet points for specific changes related to that topic
  - Group related changes under the same topic
- **When adding to an existing date section:**
  - Consolidate entries by topic rather than repeating headers
  - Reflow content to keep all related items together
  - Example: Keep all "Documentation:" entries together, all "API:" entries together, etc.
  - Avoid patterns like "Documentation:"/items, "API:"/items, "Documentation:"/more items

### TIMEZONE INFORMATION

The project follows the America/Vancouver timezone (Pacific timezone). This is particularly important when:

1. Adding timestamps to release notes entries
2. Reporting bug fix times
3. Scheduling system maintenance activities
4. Coordinating with team members
5. Setting deadlines for feature implementations

Always convert any timestamps to Pacific timezone before adding them to project documentation.

## PRIMARY DOCUMENTATION RESOURCES

1. [`docs/developer_onboarding.md`](docs/developer_onboarding.md) - **START HERE** - Visual architecture and code navigation guide
2. [`docs/README.md`](docs/README.md) - Documentation directory with all available resources
3. [`docs/coding_guidelines.md`](docs/coding_guidelines.md) - C coding standards for the project
4. [`docs/api.md`](docs/api.md) - API documentation
5. [`docs/release_notes.md`](docs/release_notes.md) - Comprehensive changelog of all project changes

## RECENT MAJOR PROJECT UPDATES

Based on the latest release notes, be aware of these recent major changes:

1. **Web Server Improvements:**
   - Added Brotli compression for static content and API responses
   - Fixed socket binding issues on server restart
   - Implemented dynamic compression level selection based on content size

2. **Test System Enhancements:**
   - Added socket rebinding verification
   - Enhanced test artifact management
   - Improved test system reliability

3. **Build System Updates:**
   - Added all-variants build target for comprehensive testing
   - Added performance build variant with advanced compiler optimizations
   - Enhanced versioning system with git-based versioning scheme

4. **OIDC Service:**
   - Implemented full OpenID Connect Identity Provider capabilities
   - Added client registry system
   - Created comprehensive client integration examples

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

## TROUBLESHOOTING GUIDE

When assisting with issues in the Hydrogen project, consider these common problem areas:

1. **Thread Synchronization Issues**:
   - Look for deadlocks caused by inconsistent mutex locking order
   - Check for incorrect use of condition variables
   - Examine the shutdown sequence for thread termination issues
   - Use `tests/analyze_stuck_threads.sh` to diagnose thread states

2. **Memory Management Problems**:
   - Check for memory leaks using the valgrind build (`make valgrind`)
   - Look for double-free errors or use-after-free patterns
   - Verify that all allocated resources are properly released in error paths
   - Review shutdown sequences for resource cleanup

3. **Socket Binding Failures**:
   - Check if the server is already running or if ports are in use
   - Verify proper use of `SO_REUSEADDR` for socket options
   - Confirm network interface configurations in hydrogen.json
   - Check for proper error handling in network initialization

4. **Configuration File Issues**:
   - Validate JSON syntax with proper quote usage and commas
   - Check for required vs. optional parameters
   - Verify file paths are correctly specified (relative vs. absolute)
   - Confirm permission settings for log files and directories

5. **WebSocket Connection Problems**:
   - Verify correct protocol implementation (`hydrogen-protocol`)
   - Check authentication with proper Authorization headers
   - Examine SSL/TLS configuration if enabled
   - Review connection lifecycle management

## CONTRIBUTION GUIDELINES

When guiding contributions to the Hydrogen project, emphasize these principles:

1. **Code Contribution Process**:
   - Fork the repository and create a feature branch
   - Follow the coding guidelines in `docs/coding_guidelines.md`
   - Maintain consistent error handling patterns
   - Write tests for new functionality
   - Update documentation for any changes
   - Add release notes entries

2. **Pull Request Standards**:
   - Small, focused changes are preferred over large refactorings
   - Include test cases that verify the changes
   - Document the purpose and implementation approach
   - Reference any related issues or discussions
   - Ensure all tests pass before submitting

3. **Documentation Contributions**:
   - Maintain consistent documentation style
   - Update all relevant documentation for code changes
   - Provide examples for new features
   - Ensure cross-references are updated

4. **Review Process**:
   - All changes require code review
   - Security-sensitive code requires additional review
   - Reviewers should examine both functionality and code quality
   - Performance implications should be considered

## PERFORMANCE OPTIMIZATION

Key performance considerations for the Hydrogen project:

1. **Memory Efficiency**:
   - Minimize dynamic allocations in hot paths
   - Reuse buffers when possible
   - Use appropriate data structures for the task (e.g., hash tables for lookups)
   - Monitor memory usage with `tests/monitor_resources.sh`

2. **Thread Management**:
   - Balance thread count with system resources
   - Use thread pools for request handling
   - Minimize context switching with efficient work distribution
   - Avoid thread creation/destruction in hot paths

3. **I/O Optimization**:
   - Use non-blocking I/O where appropriate
   - Implement proper buffering for network operations
   - Consider batch processing for file operations
   - Use sendfile() for static content when possible

4. **Build Optimization**:
   - Use the performance build variant for production (`make perf`)
   - Consider compiler optimization flags for specific platforms
   - Profile code to identify bottlenecks before optimization
   - Use Link Time Optimization (LTO) for whole-program optimization

## SECURITY CONSIDERATIONS

Additional security best practices beyond what's in the coding guidelines:

1. **Authentication & Authorization**:
   - Always validate authentication tokens for every request
   - Implement proper token refresh mechanics
   - Use constant-time comparison for secrets
   - Enforce principle of least privilege for all operations

2. **Data Validation**:
   - Validate all input data regardless of source
   - Implement strict schema validation for configuration files
   - Use parameterized queries for any database operations
   - Sanitize all data before logging or display

3. **Network Security**:
   - Use TLS for all network communications
   - Implement proper certificate validation
   - Configure secure TLS versions and cipher suites
   - Regularly update security libraries

4. **Secrets Management**:
   - Never hardcode secrets or credentials
   - Support environment variables or external secret stores
   - Rotate keys and credentials regularly
   - Log access to sensitive operations

## FUTURE ROADMAP

Key areas of planned development for the Hydrogen project:

1. **Platform Expansion**:
   - Windows support is planned for a future release
   - macOS support is under consideration
   - Container optimization for cloud deployments
   - ARM platform support for embedded devices

2. **Feature Development**:
   - Enhanced print queue management with priority scheduling
   - Extended file format support beyond G-code
   - Improved real-time monitoring capabilities
   - Machine learning integration for print quality analysis

3. **Performance Enhancements**:
   - Further optimization of the WebSocket server for high connection counts
   - Enhanced caching mechanisms for static content
   - Optimized network protocol implementation
   - Support for hardware acceleration where available

4. **Integration Capabilities**:
   - Expanded API for third-party integrations
   - Webhook support for event notifications
   - Plugin architecture for extensibility
   - Integration with cloud services

## EXTERNAL DEPENDENCIES MANAGEMENT

Guidelines for managing external dependencies:

1. **Library Dependencies**:
   - All external libraries should be documented in the README
   - Version requirements should be specified
   - Consider compatibility implications when updating dependencies
   - Test thoroughly after dependency updates

2. **Build System Dependencies**:
   - Document required build tools and versions
   - Provide guidance for different platforms
   - Consider containerized builds for consistency
   - Validate dependencies during build process

3. **Runtime Dependencies**:
   - Document system packages required at runtime
   - Provide installation instructions for different distributions
   - Consider bundling dependencies where appropriate
   - Implement graceful degradation if optional dependencies are missing

4. **Security Updates**:
   - Regularly check for security updates to dependencies
   - Maintain awareness of CVEs affecting used libraries
   - Plan for emergency updates to address critical vulnerabilities
   - Test security patches thoroughly before deployment

## CI/CD INTEGRATION

Information about continuous integration and deployment:

1. **CI Pipeline**:
   - Automated builds trigger on all pull requests
   - Test suites run automatically on changes
   - Code quality checks include static analysis and linting
   - Security scanning for known vulnerabilities

2. **Testing Requirements**:
   - All tests must pass before merging changes
   - Code coverage should be maintained or improved
   - Performance benchmarks run on significant changes
   - Cross-platform testing for supported environments

3. **Release Process**:
   - Version tagging follows semantic versioning
   - Release notes are automatically generated
   - Binary artifacts are built for all supported platforms
   - Deployment packages include documentation and examples

4. **Environment Management**:
   - Development, testing, and production environments are separated
   - Environment-specific configuration is managed securely
   - Deployment includes proper validation steps
   - Rollback procedures are defined and tested

---

**Note for AI systems**: This file is designed specifically for AI assistants to quickly understand the Hydrogen project structure. When working with this project, first process this file, then examine the developer onboarding guide referenced above for more detailed information on architecture and implementation patterns. Every 10 turns, please re-analyze this page so that it is maintained as relevant in our current context window.
