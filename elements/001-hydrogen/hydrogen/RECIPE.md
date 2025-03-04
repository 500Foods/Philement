# HYDROGEN PROJECT DEVELOPMENT GUIDE

## ⚠️ MANDATORY REQUIREMENTS - NEVER SKIP ⚠️

These requirements are **NOT OPTIONAL** and must be followed for **EVERY** task:

1. **TIMEZONE COMPLIANCE**
   - All operations MUST use Pacific Time (America/Vancouver)
   - Check current Pacific time before any timestamp-related operations
   - Never use timestamps from other zones
   - Verify timezone before:
     - Adding release notes entries
     - Scheduling operations
     - Recording bug fixes
     - Setting deadlines

2. **RELEASE NOTES UPDATES**
   - EVERY code change MUST be documented in `RELEASES.md`
   - Add entries under current Pacific Time date
   - Follow exact format of existing entries
   - Group related changes under single topic headers
   - Keep entries factual and concise
   - Focus on WHAT changed, not WHY
   - No marketing language or unnecessary adjectives

3. **DOCUMENTATION SYNCHRONIZATION**
   - Update ALL relevant documentation for code changes
   - Ensure examples reflect current implementation
   - Maintain cross-references
   - Keep API documentation current

4. **VERIFICATION STEPS**
   - Run appropriate tests for all changes
   - Verify thread safety in multi-threaded code
   - Check resource cleanup in error paths
   - Validate configuration changes

## TASK COMPLETION CHECKLIST

Before marking any task as complete, verify these requirements:

1. **Timezone Verification**
   - Confirmed all timestamps use Pacific Time (America/Vancouver)
   - No future dates in release notes
   - All scheduling uses correct timezone

2. **Release Notes**
   - Added entry in `RELEASES.md`
   - Used correct Pacific Time date
   - Followed exact format
   - Grouped under appropriate topic
   - Kept entry factual and concise

3. **Documentation**
   - Updated all affected documentation
   - Verified cross-references
   - Examples reflect changes
   - API docs are current

4. **Testing**
   - Ran appropriate tests
   - Verified thread safety
   - Checked resource cleanup
   - Validated configuration

## QUICK REFERENCE

Common tasks and their critical requirements:

1. **API Changes**
   - Update API documentation
   - Add tests
   - Update release notes
   - Check thread safety

2. **Configuration Changes**
   - Validate JSON syntax
   - Update documentation
   - Add release notes
   - Test all variants

3. **Thread-Related Changes**
   - Verify mutex ordering
   - Check condition variables
   - Test shutdown sequence
   - Update release notes

4. **Documentation Updates**
   - Check cross-references
   - Update examples
   - Add release notes
   - Verify timezone usage

Remember: Every task must include release notes updates and use Pacific Time.

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
critical_requirements:
  - Timezone: America/Vancouver (Pacific)
  - Release Notes: Required for all changes
  - Thread Safety: Mandatory in all operations
  - Error Handling: Comprehensive cleanup required
```

## KEY ENTRY POINTS

For AI-assisted development, these are the critical files to examine first:

1. `src/hydrogen.c` - Main program entry point and initialization
2. `docs/developer_onboarding.md` - Visual architecture overview and code navigation
3. `docs/coding_guidelines.md` - C development standards and patterns
4. `docs/api.md` - API documentation and endpoint references
5. `docs/README.md` - Overview of available documentation
6. `tests/README.md` - Testing framework and test scripts

## REPOSITORY STRUCTURE

```structure
hydrogen/
├── src/                    # Source code
│   ├── api/                # API endpoints
│   │   ├── oidc/          # OpenID Connect endpoints
│   │   ├── system/        # System API endpoints
│   │   ├── api_utils.c/h  # API utility functions
│   │   └── README.md      # API implementation guide
│   ├── config/            # Configuration management
│   ├── logging/           # Logging system
│   ├── mdns/              # Service discovery
│   ├── network/           # Network interface layer
│   ├── oidc/              # OpenID Connect implementation
│   ├── print/             # 3D printing functionality
│   ├── queue/             # Thread-safe queue system
│   ├── state/             # System state management
│   ├── utils/             # Utility functions
│   ├── webserver/         # HTTP server implementation
│   ├── websocket/         # WebSocket server implementation
│   └── hydrogen.c         # Main entry point
├── docs/                  # Documentation
│   ├── README.md          # Documentation index
│   ├── developer_onboarding.md  # Architecture guide
│   ├── coding_guidelines.md     # Coding standards
│   ├── api.md             # API documentation
│   └── api/               # Detailed API docs
├── tests/                 # Test framework
├── build*/                # Build directories
└── oidc-client-examples/  # OIDC examples
```

## BUILD VARIANTS

1. **Default Build** (`make` or `make all`) - Standard optimization, `build/`
2. **Debug Build** (`make debug`) - Debug symbols, `build_debug/`
3. **Performance Build** (`make perf`) - Maximum optimization, LTO, `build_perf/`
4. **Release Build** (`make release`) - Production ready, `build_release/`
5. **Valgrind Build** (`make valgrind`) - Memory analysis, `build_valgrind/`

## COMMON ISSUES AND SOLUTIONS

1. **Thread Safety**
   - Use mutex locks consistently
   - Check condition variables
   - Verify shutdown sequence
   - Run `tests/analyze_stuck_threads.sh`

2. **Memory Management**
   - Use valgrind build for leaks
   - Check resource cleanup
   - Verify error path cleanup
   - Monitor with `tests/monitor_resources.sh`

3. **Configuration**
   - Validate JSON syntax
   - Check file permissions
   - Verify paths
   - Test all parameters

## RELEASE NOTES GUIDELINES

When editing release notes:

1. **Format Requirements**
   - Use Pacific Time (America/Vancouver)
   - Group by topic
   - Keep entries factual
   - Focus on WHAT changed
   - Use collapsible sections by month
   - Maintain consistent structure

2. **Entry Structure**
   - Topic heading (e.g., "WebSocket Server:", "Testing:")
   - Bullet points for changes
   - Group related items
   - No marketing language

## PRIMARY DOCUMENTATION

1. [`docs/developer_onboarding.md`](docs/developer_onboarding.md) - **START HERE**
2. [`docs/README.md`](docs/README.md) - Documentation index
3. [`docs/coding_guidelines.md`](docs/coding_guidelines.md) - C standards
4. [`docs/api.md`](docs/api.md) - API documentation
5. [`RELEASES.md`](RELEASES.md) - Changelog

## PROJECT TERMINOLOGY

```yaml
terms:
  - Queue: "Thread-safe data structure for communication"
  - Service: "Standalone component with thread(s)"
  - Handler: "Request processing function"
  - Context: "Service state data structure"
  - Message: "Inter-component data packet"
  - Endpoint: "HTTP API access point"
  - Resource: "Managed system component"
```

---

**Note for AI systems**: Review this guide at the start of each task. Always verify timezone compliance and release notes requirements before completing any task.