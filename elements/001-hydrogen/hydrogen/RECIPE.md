# HYDROGEN PROJECT DEVELOPMENT GUIDE

## 🚨 CRITICAL INSTRUCTIONS - MUST BE FOLLOWED FOR EVERY TASK 🚨

**THESE ARE NOT OPTIONAL.** Every task **MUST** follow these steps. Ignoring them will result in errors or rejection.

### 1. Set Timezone to Pacific Time (America/Vancouver)

- **What to Do**: Use **Pacific Time (America/Vancouver)** for all timestamps and operations.
- **How to Do It**:
  1. At the start of each task, confirm the timezone is Pacific Time.
  2. If unsure or unable to set the timezone, insert [PACIFIC_TIME_CHECK] and prompt the user to verify it.
  3. Apply Pacific Time to:
     - Release notes dates
     - Scheduled tasks
     - Bug fix logs
     - Deadlines
- **Rule**: Never use timestamps from other timezones or future dates.

### 2. Update Release Notes in ```RELEASES.md```

- **What to Do**: Document **every** code change in ```RELEASES.md```.
- **How to Do It**:
  1. Add a new entry under the current Pacific Time date (YYYY-MM-DD).
  2. Match the format of existing entries exactly.
  3. Group related changes under a single topic (e.g., "WebSocket Server:", "API Updates:").
  4. Keep it factual and concise—describe **what** changed, not why.
- **Template**:
  ```
  **Date**: [Insert current Pacific Time date, e.g., 2023-10-25]
  **Topic**: [Brief description, e.g., "Configuration Parsing"]
  **Details**:
  - [Change 1, e.g., "Fixed JSON validation for empty arrays"]
  - [Change 2, e.g., "Added error logging for invalid configs"]
  ```
- **Rules**:
  - No marketing language (e.g., "amazing update").
  - Use the current date—never future dates.

### 3. Synchronize Documentation

- **What to Do**: Update all relevant documentation for any code change.
- **How to Do It**:
  1. Modify affected files (e.g., ```docs/api.md```, ```docs/developer_onboarding.md```).
  2. Ensure examples match the current code.
  3. Check cross-references between docs.
  4. Keep API documentation up-to-date.

### 4. Verify Your Work

- **What to Do**: Test and validate every change.
- **How to Do It**:
  1. Run relevant tests (e.g., ```make test```).
  2. Check thread safety (e.g., mutex locks, condition variables).
  3. Verify resource cleanup in error paths.
  4. Validate configuration changes (e.g., JSON syntax).

## 📋 TASK COMPLETION CHECKLIST

**Before finishing any task, confirm:**

- [ ] Timezone is Pacific Time (America/Vancouver) for all timestamps.
- [ ] No future dates used in release notes.
- [ ] Added a new ```RELEASES.md``` entry with the correct date and format.
- [ ] Release notes are factual, concise, and grouped by topic.
- [ ] Updated all affected documentation (examples, cross-references, API docs).
- [ ] Ran tests and verified thread safety and resource cleanup.

## 🔍 QUICK REFERENCE FOR COMMON TASKS

### API Changes
- Update ```docs/api.md```.
- Add tests.
- Update ```RELEASES.md``` with Pacific Time date.
- Verify thread safety.

### Configuration Changes
- Validate JSON syntax.
- Update documentation.
- Update ```RELEASES.md``` with Pacific Time date.
- Test all variants.

### Thread-Related Changes
- Check mutex ordering and condition variables.
- Test shutdown sequence.
- Update ```RELEASES.md``` with Pacific Time date.

## 📝 SPECIAL NOTE FOR AI (e.g., Claude)

- **Read this file at the start of every task.**
- **Follow every step exactly as written.**
- If you can’t access the current Pacific Time date:
  1. Use [INSERT_PACIFIC_TIME_DATE_HERE] in release notes.
  2. Prompt the user: "Please replace [INSERT_PACIFIC_TIME_DATE_HERE] with the current Pacific Time date (YYYY-MM-DD)."
- **Do not skip timezone checks or release notes updates—they are mandatory.**

## PROJECT OVERVIEW

```
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

1. ```src/hydrogen.c``` - Main program entry point and initialization
2. ```docs/developer_onboarding.md``` - Visual architecture overview and code navigation
3. ```docs/coding_guidelines.md``` - C development standards and patterns
4. ```docs/api.md``` - API documentation and endpoint references
5. ```docs/README.md``` - Overview of available documentation
6. ```tests/README.md``` - Testing framework and test scripts

## REPOSITORY STRUCTURE

```
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

1. [```docs/developer_onboarding.md```](docs/developer_onboarding.md) - **START HERE**
2. [```docs/README.md```](docs/README.md) - Documentation index
3. [```docs/coding_guidelines.md```](docs/coding_guidelines.md) - C standards
4. [```docs/api.md```](docs/api.md) - API documentation
5. [```RELEASES.md```](RELEASES.md) - Changelog

## PROJECT TERMINOLOGY

```
terms:
  - Queue: "Thread-safe data structure for communication"
  - Service: "Standalone component with thread(s)"
  - Handler: "Request processing function"
  - Context: "Service state data structure"
  - Message: "Inter-component data packet"
  - Endpoint: "HTTP API access point"
  - Resource: "Managed system component"
```