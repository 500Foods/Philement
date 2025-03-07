# HYDROGEN PROJECT DEVELOPMENT GUIDE

## PROJECT OVERVIEW

```overview
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
  - Code Organization: Follow standard file structure
```

## CODING STANDARDS

All code must follow these organization patterns:

1. Source File Structure:
   - File-level documentation with purpose and design decisions
   - Feature test macros (if needed)
   - Includes (system, standard, third-party, project)
   - External declarations (extern functions/variables)
   - Public interface (exposed functions)
   - Private declarations (static functions)
   - Implementation

2. Critical Practices:
   - Thread-safe design for all operations
   - Comprehensive error handling and cleanup
   - Clear documentation of "why" decisions
   - Proper mutex and resource management

Detailed guidelines are in [docs/coding_guidelines.md](docs/coding_guidelines.md).

## KEY ENTRY POINTS

For AI-assisted development, these are the critical files to examine first:

1. ```src/hydrogen.c``` - Main program entry point and initialization
2. ```docs/developer_onboarding.md``` - Visual architecture overview and code navigation
3. ```docs/coding_guidelines.md``` - C development standards and patterns
4. ```docs/api.md``` - API documentation and endpoint references
5. ```docs/README.md``` - Overview of available documentation
6. ```tests/README.md``` - Testing framework and test scripts

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

1. **Default Build** (```make``` or ```make all```) - Standard optimization, ```build/```
2. **Debug Build** (```make debug```) - Debug symbols, ```build_debug/```
3. **Performance Build** (```make perf```) - Maximum optimization, LTO, ```build_perf/```
4. **Release Build** (```make release```) - Production ready, ```build_release/```
5. **Valgrind Build** (```make valgrind```) - Memory analysis, ```build_valgrind/```

## COMMON ISSUES AND SOLUTIONS

1. **Thread Safety**
   - Use mutex locks consistently
   - Check condition variables
   - Verify shutdown sequence
   - Run ```tests/analyze_stuck_threads.sh```

2. **Memory Management**
   - Use valgrind build for leaks
   - Check resource cleanup
   - Verify error path cleanup
   - Monitor with ```tests/monitor_resources.sh```

3. **Configuration**
   - Validate JSON syntax
   - Check file permissions
   - Verify paths
   - Test all parameters

## RELEASE NOTES GUIDELINES

The project uses a two-level release notes system:

1. **Main Release Log (`RELEASES.md`)**
   - Chronological index of all changes
   - Organized by month and date
   - Links to detailed daily release notes

   ```markdown
   - March 2025
     - [2025-Mar-05 (Wed)](docs/releases/2025-03-05.md): API, WebSocket Server, Testing
     - [2025-Mar-04 (Tue)](docs/releases/2025-03-04.md): Web Server, Configuration
   ```

2. **Daily Release Notes (`docs/releases/YYYY-MM-DD.md`)**
   - One file per day with changes
   - Organized by technical topics
   - Detailed but concise descriptions
   - Example structure:

   ```markdown
   # March 5, 2025
   
   ## API
   - Fixed segmentation fault in System Config endpoint:
     - Removed redundant json_decref call
     - Ensured proper JSON object lifecycle
   
   ## WebSocket Server
   - Added configurable ExitWaitSeconds parameter:
     - Made thread exit wait timeout configurable
     - Added exit_wait_seconds to config struct
   ```

3. **Writing Guidelines**
   - Use Pacific Time (America/Vancouver) for all dates
   - Focus on technical details, not marketing
   - Group related changes under appropriate topics
   - Use nested bullet points for related sub-changes
   - Keep descriptions factual and specific
   - Avoid adjectives like "improved", "enhanced", "better"
   - Document exactly what changed, not why it changed
   - Include all affected components (API, docs, tests)

4. **Topic Organization**
   - Use consistent topic headings:
     - "API:" - API endpoint changes
     - "WebSocket Server:" - WebSocket functionality
     - "Web Server:" - HTTP server changes
     - "Testing:" - Test suite updates
     - "Documentation:" - Doc changes
     - "Configuration:" - Config changes
     - "General Improvements:" - Cross-cutting changes
   - Group all changes for the same topic together
   - Order topics by significance/impact

5. **Release Notes Workflow**
   1. Create new daily file if needed: `docs/releases/YYYY-MM-DD.md`
   2. Add changes under appropriate topic headings
   3. Update `RELEASES.md` with link and topic summary
   4. Verify all dates are in Pacific Time

## PRIMARY DOCUMENTATION

1. [```docs/developer_onboarding.md```](docs/developer_onboarding.md) - **START HERE**
2. [```docs/README.md```](docs/README.md) - Documentation index
3. [```docs/coding_guidelines.md```](docs/coding_guidelines.md) - C standards
4. [```docs/api.md```](docs/api.md) - API documentation
5. [```RELEASES.md```](RELEASES.md) - Changelog

## PROJECT TERMINOLOGY

```terms
terms:
  - Queue: "Thread-safe data structure for communication"
  - Service: "Standalone component with thread(s)"
  - Handler: "Request processing function"
  - Context: "Service state data structure"
  - Message: "Inter-component data packet"
  - Endpoint: "HTTP API access point"
  - Resource: "Managed system component"
```
