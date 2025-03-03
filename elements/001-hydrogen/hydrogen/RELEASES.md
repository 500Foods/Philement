# Release Notes

<!--
EDITING GUIDELINES (not visible when rendered):
- Keep entries concise and factual
- Focus on WHAT changed, not WHY it's good
- Avoid adjectives like "comprehensive", "robust", "significant"
- Eliminate unnecessary justifications
- Use fewer words where possible
- Stick to listing actual changes, not opinions about them
- This is a technical record, not marketing material
- Use collapsible sections by month (except for the most recent month)
  - Wrap older months in <details><summary>YYYY-MMM</summary> and </details> tags
  - Keep the current/most recent month expanded
  - This creates a more compact view while maintaining all history
- Maintain consistent structure for all entries:
  - Start with a topic heading (e.g., "WebSocket Server:", "Testing:")
  - Follow with bullet points for specific changes related to that topic
  - Group related changes under the same topic
- When adding to an existing date section:
  - Consolidate entries by topic rather than repeating headers
  - Reflow content to keep all related items together
  - Example: Keep all "Documentation:" entries together, all "API:" entries together, etc.
  - Avoid patterns like "Documentation:"/items, "API:"/items, "Documentation:"/more items
-->

## 2025-Mar-04

Configuration:

- Fixed API prefix initialization when WebServer section is missing:
  - Added default initialization of api_prefix to "/api" in configuration.c
  - Ensured API endpoints work correctly with minimal configuration
  - Fixed test_api_prefixes.sh test failures due to missing prefix
  - Added logging for default API prefix when using minimal configuration

Testing:

- Enhanced test summary with subtest result aggregation:
  - Added export_subtest_results function to support_utils.sh
  - Modified test_all.sh to collect and display subtest statistics
  - Implemented subtest tracking in all test scripts:
    - test_api_prefixes.sh (10 subtests)
    - test_system_endpoints.sh (9 subtests)
    - test_startup_shutdown.sh (6 subtests, combining min/max configurations)
    - test_compilation.sh (7 subtests)
    - test_dynamic_loading.sh (7 subtests)
    - test_env_variables.sh (3 subtests)
    - test_json_error_handling.sh (2 subtests)
    - test_library_dependencies.sh (16 subtests)
    - test_socket_rebind.sh (3 subtests)
    - test_swagger_ui.sh (10 subtests)
  - Added comprehensive subtest pass/fail counts to test summary

## 2025-Mar-03

Web Server:

- Fixed Swagger UI access with URLs lacking trailing slash:
  - Implemented HTTP 301 redirect from /swagger to /swagger/
  - Added proper handling in handle_swagger_request function
  - Enhanced logging of URL processing for debugging
  - Ensured correct relative path resolution for all resources
  - Maintained backward compatibility with existing functionality
- Updated Swagger UI implementation to use configurable prefix:
  - Removed hardcoded "/swagger/" path in swagger-initializer.js
  - Used config->swagger.prefix for all internal paths
  - Ensured correct operation when prefix is changed in configuration
  - Fixed dynamic content generation to respect configured prefix
- Made API prefix configurable in configuration file:
  - Replaced hardcoded "/api" with configurable WebConfig->api_prefix
  - Fixed format string issue in web_server_swagger.c for dynamic initializer
  - Updated API endpoint detection in web_server_request.c for consistency
  - Added helper function to build API paths with configured prefix
  - Ensured compatibility with existing API endpoints

Testing:

- Fixed test results directory path in test_system_endpoints.sh:
  - Updated script to use tests/results/ directory for output files
  - Standardized results path to match other test scripts
  - Eliminated output file scattering between project root and tests directory
- Updated upload directory paths in test configuration files:
  - Modified hydrogen_test_api.json, hydrogen_test_min.json, and hydrogen_test_max.json
  - Changed upload directories from "./uploads" to "/tmp/hydrogen_test_uploads"
  - Standardized temporary directory usage across test and production configurations
  - Prevented uploads directory creation in project root
- Refactored test scripts to use dynamic port configuration:
  - Updated test_api_prefixes.sh to extract and use port from configuration files
  - Modified test_swagger_ui.sh to use dynamically extracted port for URL construction
  - Updated test_system_endpoints.sh to use consistent base URL with correct port
  - Added port-aware socket state checking and cleanup in test_api_prefixes.sh
  - Improved handling of custom port configurations in all network-related tests
  - Fixed connection failures when using non-default ports in configuration
- Added dedicated port configurations for test isolation:
  - Created hydrogen_test_swagger_port.json with port 5060 for Swagger UI testing
  - Created hydrogen_test_system_endpoints.json with port 5070 for API endpoint testing
  - Updated test_swagger_ui.sh to use the new port-specific configuration
  - Updated test_system_endpoints.sh to use the new port-specific configuration
  - Prevented port conflicts between different test runs
  - Improved test reliability by avoiding socket rebinding issues
- Renamed Swagger test configuration files for consistency:
  - Changed to hydrogen_test_swagger_test_1.json and hydrogen_test_swagger_test_2.json
  - Updated tests/README.md with the new naming convention
  - Updated test_swagger_ui.sh to use the new configuration files
- Refactored test framework for improved modularity:
  - Removed unused support_test_runner.sh script
  - Enhanced support_utils.sh with common test functions
  - Added server management functions for test scripts
  - Created standardized test environment setup functions
  - Added HTTP request validation utilities
  - Created comprehensive test script template
  - Updated test documentation with standardized approach
  - Added step-by-step instructions for creating new tests

Service Discovery:

- Added mDNSClient subsystem:
  - Service discovery and monitoring capabilities
  - Health checking for discovered services
  - Load balancing support
  - Integration with existing mDNSServer
  - Support for multiple service types
  - Configurable monitoring intervals

Web Server:

- Added runtime Swagger UI integration:
  - Automatic payload detection from executable using magic marker
  - In-memory file serving with Brotli compression support
  - Configurable URL prefix via JSON configuration
  - Thread-safe resource management
  - TAR extraction with dynamic memory allocation
  - Dynamic URL updates for Swagger UI based on incoming requests:
    - Automatic server URL detection from Host header
    - Fallback to localhost:port when Host header missing
    - HTTPS detection via X-Forwarded-Proto header
    - Runtime modification of swagger-initializer.js
    - Client-side swagger.json URL updates for correct server paths
    - Automatic API server URL updates based on request origin

Architecture:

- Added new subsystems to core architecture:
  - Terminal: Web-based shell access with xterm.js integration
  - Databases: Multi-database support with connection pooling
  - SMTPRelay: Email processing with message transformation
  - Enhanced Swagger: Interactive API documentation

Configuration:

- Added new subsystems to configuration:
  - Terminal: Web-based terminal access with xterm.js integration
  - Databases: Multi-database support with connection pooling
  - SMTPRelay: Email processing with queue management
  - Enhanced Swagger: Added configurable URL prefix option
- Updated test configurations with new subsystems
- Added environment variable support for sensitive credentials

Documentation:

- Renamed AI_ASSISTANT.md to ADVICE.md for improved clarity
- Added mandatory requirements section at top of document
- Added task completion checklist and quick reference guide
- Streamlined documentation for improved task efficiency
- Enhanced emphasis on timezone compliance and release notes requirements
- Moved release_notes.md to project root for better visibility and to emphasize its critical role
- Updated all documentation references to new release notes location
- Removed old AI_ASSISTANT.md file
- Renamed ADVICE.md to RECIPE.md for clearer purpose indication
- Added "Additional Documentation" section to README.md listing all .md files outside docs/
- Updated all documentation references to reflect current filenames

Library Management:

- Added dynamic library loading system:
  - Created extensible API for runtime library management
  - Implemented conditional loading of optional libraries only when needed
  - Added graceful fallback mechanisms for missing libraries
  - Created function pointer abstraction for safe library usage
  - Added support for delayed loading of libraries used in specific threads
  - Maintained backward compatibility with existing dependency checking
  - Added memory management for clean library unloading
  - Implemented proper error reporting for missing dependencies
  - Added helper macros for simplified function pointer usage
  - Created example implementation for WebSocket server

## 2025-Mar-02

API Documentation:

- Added OpenAPI 3.1.0 integration for REST API documentation:
  - Created swagger directory at project root with generation script
  - Implemented annotation-based documentation using //@ swagger: prefix
  - Added automatic swagger.json generation from source code annotations
  - Created path, method, and response schema documentation capability
  - Added service-level organization for API endpoints
  - Implemented proper status code handling for responses
  - Added support for tagging and grouping related endpoints
  - Added example annotations for system/health endpoint
  - Generated OpenAPI-compliant output for use with standard tools
  - Completed Swagger annotations for all system endpoints (health, info, test)
  - Added comprehensive Swagger annotations for all OIDC endpoints
  - Created detailed README for swagger directory with documentation guidelines
  - Added SwaggerUI integration with download script
  - Implemented SwaggerUI packaging as optimized distributable tar file:
    - Selectively compressed static assets (JS, CSS, JSON) with Brotli for minimal size
    - Preserved dynamic files (index.html, swagger-initializer.js) for runtime modification
    - Added oauth2-redirect.html support for OAuth 2.0 authorization flows
    - Created flat file structure for efficient serving
    - Compressed final package with Brotli to ~345KB
  - Added Swagger subsystem to configuration with UI options:
    - Configurable endpoint prefix defaulting to /swagger
    - TryItEnabled, AlwaysExpanded, and other SwaggerUI configuration options
  - Added build dependencies documentation
  - Enhanced Swagger README with comprehensive implementation details

Build System:

- Added tarball identifier to release executable:
  - Appended "<<< HERE BE ME TREASURE >>>" magic string before tarball size value
  - Enables reliable detection of appended tarball in executable
  - Supports runtime determination of whether the executable includes SwaggerUI
- Enhanced build versioning system:
  - Added compile-time RELEASE timestamp using ISO8601 format:
    - Implemented in Makefile using `date -u +'%Y-%m-%dT%H:%M:%SZ'`
    - Passed to compiler via `-DRELEASE` macro
    - Replaced runtime timestamp detection with compile-time value
    - Added fallback definitions for compatibility
  - Added BUILD_TYPE identifier for each build variant:
    - Assigned specific values for each variant (Regular, Debug, Valgrind-Compatible, Performance, Release)
    - Passed to compiler via `-DBUILD_TYPE` macro
    - Added logging of build type at startup
    - Added build_type field to system status JSON response
  - Maintained consistent compiler flags structure to prevent redefinition warnings
  - Integrated with existing version information in logs and API responses

Configuration:

- Added environment variable substitution in configuration values:
  - Implemented ${env.VARIABLE} format for referencing environment variables
  - Added automatic type conversion based on environment variable content
  - Support for null, boolean, number, and string value types
  - Fallback to defaults when environment variables don't exist
  - Consistent handling across all configuration settings
  - Added INFO-level logging under "Environment" subsystem showing variable name, type, and value
  - Implemented masking for sensitive data (keys, passwords, tokens) showing only first 5 characters
  - Enhanced testing with improved validation of environment variable processing
- Removed routine JSON processing log messages from configuration subsystem

Initialization:

- Added library dependency checking system:
  - Created utils_dependency utility module for version checking
  - Implemented runtime detection of required libraries and versions
  - Added configuration-aware dependency status reporting
  - Different log levels based on dependency status (INFO, WARN, CRITICAL)
  - Intelligent determination of required vs. optional libraries
  - Automatic detection of versions during initialization
  - Added critical dependency counting and startup reporting
  - Fixed dependency tracking logic to accurately count missing dependencies

Testing:

- Enhanced test_compilation.sh with tarball verification:
  - Added test for Swagger UI tarball presence in release builds
  - Implemented detection of "<<< HERE BE ME TREASURE >>>" delimiter
  - Added size verification to ensure executable contains the tarball
  - Integrated test into existing compilation test sequence
- Improved environment variable test script:
  - Added tracking of passed and failed checks with detailed counts
  - Added specific tests for environment variable type detection
  - Enhanced output with clear test results for each test phase
  - Fixed issues with warning/error reporting and test outcome determination
  - Added section headers to clarify different test phases
  - Fixed inconsistency where warnings didn't affect pass/fail status
  - Ensured all check failures increment failure counter properly
  - Added clearer output when environment variable logs are missing
  - Refocused tests to properly detect environment variables with more reliable pattern matching
  - Fixed handling of problematic environment variables by skipping checks where appropriate
  - Simplified regex patterns for improved reliability across different log formats
- Modified test scripts to display relative paths:
  - Added path conversion function to test_utils.sh
  - Updated command display to show relative paths instead of absolute paths
  - Modified working directory paths to use relative format
  - Updated file paths in cleanup functions to use relative paths
  - Modified log output to show only the part starting from "hydrogen/"
  - Improved log readability by removing unnecessary path prefixes
  - Fixed final result output for consistent path presentation
- Aligned library dependency test output with system test format
- Fixed process termination handling in dependency tests

## 2025-Mar-01

API Architecture:

- Refactored OIDC endpoints into separate directories:
  - Separated combined OIDC endpoint implementation into individual endpoint handlers
  - Created dedicated directories for each OIDC endpoint (discovery, authorization, token, etc.)
  - Created consistent interface with clear separation of concerns
  - Followed same organizational pattern as system endpoints
  - Maintained utility functions in central oidc_service for shared functionality

Web Server:

- Added Brotli compression:
  - Created compression utility module with client detection
  - Implemented pre-compressed .br file serving for static content
  - Added on-the-fly compression for API JSON responses
  - Added dynamic compression level selection based on content size:
    - Level 11 (highest) for content ≤5KB
    - Level 6 (medium) for content >5KB and ≤500KB
    - Level 4 (lower) for content >500KB
  - Integrated proper Content-Encoding headers
  - Added libbrotli dependency for encoding/decoding
  - Added detailed compression logging with timing information and compression level

- Fixed socket binding on server restart:
  - Implemented SO_REUSEADDR socket option to allow immediate port rebinding
  - Eliminated TIME_WAIT state blocking when restarting server (~60s delay)
  - Used MHD_OPTION_LISTENING_ADDRESS_REUSE for cleaner implementation
  - Created dedicated test for verifying socket rebinding functionality
  - Improved error handling with proper timeout-based testing

Testing:

- Enhanced test system with socket rebinding verification:
  - Integrated socket rebinding test into main test suite
  - Added proper signal handling for test script interruption
  - Implemented robust cleanup functions to prevent process leaks
  - Added timeout handling for reliable test execution
  - Created version-aware test integration for different microhttpd versions
  - Added special handling for SIGTERM to avoid false test failures
  - Improved test sequencing with socket rebinding check before endpoint tests
  - Enhanced resource cleanup between sequential tests

Build System:

- Added all-variants build target:
  - Created single command to build all configurations
  - Added clean step before building for fresh environment
  - Added output message listing all successfully built variants
  - Streamlined full rebuild process for testing

- Added performance build variant:
  - Implemented -O3 optimization level for maximum speed
  - Added -march=native for CPU-specific optimizations
  - Added -ffast-math for optimized floating-point operations
  - Implemented -finline-functions for aggressive inlining
  - Added -funroll-loops for automatic loop optimization
  - Created dedicated build directory for performance artifacts
  - Updated test framework to verify performance build

Documentation:

- Enhanced project documentation for improved efficiency:
  - Created README.md for src/api/ directory with implementation details
  - Updated AI_ASSISTANT.md with expanded project information
  - Added troubleshooting guide to AI_ASSISTANT.md
  - Added contribution guidelines and performance optimization sections
  - Added detailed security considerations beyond coding guidelines
  - Improved timezone references and release notes formatting guidance
  - Added future roadmap and external dependencies information
  - Added CI/CD integration details

<details>
<summary><h2>2025-Feb</h2></summary>

## 2025-Feb-28

Build System:

- Added optimized release build variant:
  - Added -O2 -s flags for performance optimization and symbol removal
  - Added -DNDEBUG flag to disable assertions in production
  - Added -march=x86-64 -flto for platform optimization and link-time optimization
  - Added -fstack-protector for additional security
  - Added version definition with -DVERSION=\"1.0.0\"
  - Integrated upx compression for reduced binary size
  - Created dedicated build directory for release artifacts
  - Added comprehensive clean target for all build variants

- Enhanced versioning system:
  - Implemented git-based versioning scheme with auto-incrementing build numbers
  - Added MAJOR.MINOR.PATCH.BUILD format for consistent version identification
  - Derived BUILD number from git commit history (commit count + base offset)
  - Ensured version information is passed to all compilation steps
  - Modified startup logging to display version and release timestamp
  - Standardized version and release presentation in all logging

- Improved version management:
  - Fixed VERSION macro to support compile-time definition
  - Added #ifndef guard in configuration.h to prevent redefinition
  - Enabled version override via compiler flags
  - Maintained backward compatibility with default version

Testing:

- Enhanced test framework with release build support:
  - Modified test scripts to prioritize release build for all tests
  - Added release build as the first tested variant in compilation tests
  - Updated startup/shutdown tests to use release build when available
  - Updated system endpoint tests to prefer release build
  - Updated JSON error handling tests to use release build
  - Added automatic fallback to standard build when release not available
  - Preserved build variants between test stages for consistent testing
  - Improved test run reliability and binary availability

- Updated compilation test to fail when warnings are detected:
  - Modified test_compilation.sh to set EXIT_CODE=1 on warning detection
  - Ensures all builds, including debug build, must be warning-free
  - Maintains strict code quality standards across all build types

Code Quality:

- Improved string handling in WebSocket and utility modules:
  - Replaced unsafe strncpy usages with safer alternatives
  - Implemented explicit string truncation with proper null termination
  - Fixed format-truncation warnings in fixed-buffer string operations
  - Ensured all builds compile cleanly with Address Sanitizer enabled
  - Eliminated string handling warnings in all build types

- Improved test artifacts management:
  - Added automatic cleanup of log files before and after test runs
  - Removed stray log files from tests directory
  - Created consistent log handling across all test scripts
  - Enhanced test environment cleanliness

Configuration:

- Improved JSON error handling:
  - Added detailed JSON parsing error messages with line and column information
  - Added user-friendly error output to stderr for configuration syntax issues
  - Added graceful exit on JSON syntax errors to prevent segfaults
  - Enhanced error diagnostics to simplify troubleshooting

API Architecture:

- Restructured API modules with endpoint-specific directories:
  - Reorganized system API endpoints into dedicated subdirectories
  - Created consistent structure for all future endpoint implementations
  - Added isolated endpoint handlers with clear interfaces
  - Improved separation of concerns for API functionality

- Added new system/test diagnostic endpoint:
  - Added support for both GET and POST methods
  - Implemented JSON response with client IP address
  - Added JWT token parsing with claim extraction
  - Added proxy detection via X-Forwarded-For header
  - Added query parameter decoding into JSON array
  - Added POST data handling with URL decoding support
  - Created example endpoint for future development

Testing:

- Fixed system endpoint tests:
  - Improved form data handling in system/test endpoint
  - Fixed field extraction from form-encoded POST data
  - Enhanced POST request processing in test endpoint
  - Fixed shell script syntax error in test_system_endpoints.sh
  - Added variable initialization to ensure robust test execution
  - Updated test documentation to reflect improved capabilities

- Enhanced test artifact management:
  - Expanded .gitignore with comprehensive log file patterns
  - Added explicit exclusion for test-generated logs
  - Added exclusion patterns for response JSON files and HTML reports
  - Untracked previously tracked test files from version control
  - Improved repository hygiene by preventing test output commits

- Improved testing infrastructure:
  - Created dedicated API endpoint test configuration
  - Updated system endpoint tests to use proper configuration
  - Ensured all test logs remain in tests directory
  - Added configuration-specific log paths
  - Isolated test artifacts from production code

## 2025-Feb-27

Code Quality and Compilation:

- Fixed compilation issues across OIDC components:
  - Fixed function signature mismatches in oidc_tokens.c:
    - Updated token generation functions to use OIDCTokenClaims
    - Corrected return types for token validation functions
    - Added missing parameters to ensure header/implementation consistency
  - Removed duplicate OIDCConfig definition (now in configuration.h)
  - Resolved compiler warnings by adding proper (void) casts for unused parameters
  - Fixed unused variable in websocket_server.c
  - Ensured clean builds with strict compiler flags (-Wall -Wextra -pedantic)

- Fixed OIDC client examples:
  - Corrected malformed comment structure in client_credentials.c
  - Resolved compilation errors in Client Credentials flow example
  - Ensured clean build with strict compiler flags

## OIDC Service Architecture

- Implemented new client registry system:
  - Added oidc_clients.h and oidc_clients.c for client management
  - Created client validation and authentication methods
  - Added support for client credentials

- Added full OpenID Connect Identity Provider capabilities:
  - Core OIDC service with standard endpoints (authorization, token, userinfo)
  - JWT token handling with RSA key rotation and management
  - User identity and profile management
  - Support for multiple authentication flows (authorization code, client credentials)

- Configuration Framework:
  - Extended configuration system with OIDC-specific settings
  - Added sensible defaults for all OIDC options
  - Structured validation for security parameters

- API Endpoints:
  - Added standard OIDC endpoints to API structure
  - Implemented JWKS (JSON Web Key Set) endpoint
  - Added token introspection and revocation endpoints
  - Added client registration endpoint

- Client Examples:
  - Created comprehensive client integration examples for multiple languages
  - Implemented C client examples for three OAuth flows
  - Developed a JavaScript browser-based client example
  - Created a Makefile for easy compilation of C examples

## Testing

- Added compilation verification test:
  - Created test_compilation.sh script to verify clean builds
  - Tests all build variants (standard, debug, valgrind) of main project
  - Tests OIDC client examples compilation
  - Treats warnings as errors for strict quality enforcement
  - Integrated as first step in test workflow
  - Added failure detection to skip subsequent tests if compilation fails

## Documentation

- Added developer onboarding resources:
  - Created developer_onboarding.md with visual architecture diagrams
  - Added code navigation guide with component relationships
  - Created project glossary and implementation patterns
  - Added development workflow guidance

- Created AI assistant resources:
  - Added AI_ASSISTANT.md with machine-optimized project information
  - Structured project overview in YAML format for AI processing
  - Added HTML comment in README for AI discoverability

- Enhanced OIDC-specific documentation:
  - Created comprehensive OIDC architecture documentation
  - Added code examples for client integration
  - Added explanations of token structures and security considerations
  - Updated test documentation to reflect new capabilities

- Enhanced security documentation:
  - Added security coding standards for OIDC and authentication systems
  - Provided specific guidance for JWT and token handling
  - Added best practices for cryptographic operations
  - Created standards for secure input validation

## 2025-Feb-26

Testing System:

- Test cleanup automation:
  - Added automatic cleanup of old test results and diagnostics
  - Modified `run_tests.sh` to remove previous test data before each run
  - Created `.gitignore` file to exclude test artifacts from GitHub synchronization
  - Improved test execution efficiency

Shutdown Architecture:

- WebSocket Server:
  - Enhanced connection handling during shutdown
  - Added thread management with timeout-based cancellation
  - Implemented multi-phase context destruction
  - Extended error recovery with fallback mechanisms
  - Added resource leak prevention

- Core Shutdown:
  - Improved shutdown sequencing with proper dependency ordering
  - Enhanced thread monitoring and cleanup
  - Added detailed diagnostic information during shutdown
  - Implemented timeout-based waiting with fallback mechanisms
  - Improved service-specific cleanup procedures

- Documentation:
  - Fixed Markdown linting errors
  - Enhanced top-level README:
    - Replaced h3 tags with b tags to reduce spacing between collapsed sections
    - Renamed "Root Directory" to "Project Folder" for clarity
    - Added Tests section with links to testing scripts
    - Converted all file references to clickable links for improved navigation
    - Removed redundant location text for cleaner presentation
  - Added AI documentation:
    - Created AI Integration (docs/ai_integration.md) with features and implementation details
    - Created AI in Development (docs/development/ai_development.md) with development process information
    - Added section on AI-oriented commenting practices with examples
    - Updated main README and docs README with links to AI documentation
  - Added architecture documentation:
    - Created System Architecture (docs/reference/system_architecture.md) with high-level system overview
    - Created Print Queue Architecture (docs/reference/print_queue_architecture.md) with component details
    - Created WebSocket Server Architecture (docs/reference/websocket_architecture.md) with implementation specifics
    - Created Network Interface Architecture (docs/reference/network_architecture.md) with abstraction patterns
    - Updated main documentation README with Component Architecture section
    - Reorganized documentation for improved discoverability

## 2025-Feb-25

Testing System:

- Testing framework:
  - Configuration-driven test system with minimal and maximal configurations
  - Test scripts for startup/shutdown validation
  - Diagnostic tools for troubleshooting
  - Modular framework for test expansion

- Diagnostic tools for shutdown analysis:
  - Thread analyzer for stuck threads and deadlocks
  - Resource monitoring for memory, CPU, and thread usage
  - Logging and report generation
  - Automatic timeout handling

- Documentation:
  - Added testing documentation in docs/testing.md
  - Created tests/README.md with usage examples
  - Updated docs with shutdown diagnosis procedures

## 2025-Feb-24

WebSocket Server:

- Shutdown implementation:
  - Fixed race conditions between thread termination and context destruction
  - Two-phase pointer handling to prevent use-after-free
  - Extended timeouts with fallback mechanisms
  - Improved thread joining with synchronization
  - Error handling with logging

mDNS Server:

- Shutdown sequence:
  - Socket closure for all network interfaces
  - Thread coordination and exit verification
  - RFC-compliant service withdrawal ("goodbye" packets)
  - Resource cleanup with race condition prevention
  - Fixed memory and socket leaks

Documentation:

- Added Shutdown Architecture documentation
- Created mDNS Server implementation documentation
- Added WebSocket server shutdown documentation
- Updated main documentation index

## 2025-Feb-23

Core System:

- Refactored hydrogen module:
  - Reorganized header includes
  - Removed unused files
  - Updated web server print queue management

## 2025-Feb-22

Server Management:

- Server naming consistency:
  - Renamed mDNS references to mDNS Server
  - Updated documentation and configuration

WebSocket Server:

- Refactored initialization
- Updated configuration handling
- Removed deprecated example status code

Logging System:

- Added log level constants
- Standardized logging calls

## 2025-Feb-21

Queue System:

- Queue system readiness tracking
- Unified logging function
- Timing functions for server lifecycle
- Shutdown logging for thread status
- Configuration support for queue initialization
- Standardized percentage formatting

System Information:

- Added metrics to /api/system/info and WebSocket status:
  - Network interface details with IP addresses and traffic
  - Filesystem information with space usage
  - CPU usage per core and load averages
  - Memory and swap usage
  - Logged-in users

## 2025-Feb-20

WebSocket Server:

- Split implementation into modules:
  - Connection lifecycle
  - Authentication
  - Message processing
  - Event dispatching
  - Status reporting
- State management with WebSocketServerContext
- Updated initialization sequence and error handling
- Added port fallback mechanism
- Fixed session validation during vhost creation
- Thread safety with mutex protection
- Improved error reporting and logging

## 2025-Feb-19

System Status:

- Added server timing to /api/system/info endpoint:
  - `server_started` field with ISO-formatted UTC start time
  - `server_runtime` field with uptime in seconds
  - `server_runtime_formatted` field with human-readable uptime
- Updated API documentation with examples

Documentation:

- Added link to Print Queue Management documentation
- Moved release notes to dedicated file
- Updated System Dependencies section
- Improved documentation organization

## 2025-Feb-18

Network Infrastructure:

- IPv6 support for web and websocket servers:
  - Added EnableIPv6 configuration flags
  - Implemented dual-stack support in web server
  - Added IPv6 interface binding in websocket server
  - Consistent IPv6 configuration across network services

## 2025-Feb-17

Documentation and System Metrics:

- Added configuration and service documentation
- Updated thread and process memory reporting
- Added Bash script example
- Added File Descriptor information to info endpoint

## 2025-Feb-16

Code Quality and Configuration:

- Improved code comments
- Added IPv6Enable flag to JSON configuration for mDNS
- Standardized logging output format

## 2025-Feb-15

Documentation and Configuration:

- Added project documentation and status API endpoint example
- Synchronized API endpoint with WebSockets output
- Added server independence configuration:
  - JSON updated with Enabled flags for each server
  - Added PrintServer to JSON
  - Updated Startup/Shutdown functions to check for flags
  - Configured mDNS to advertise only enabled services

## 2025-Feb-14

API Development:

- Basic REST API implementation:
  - Added SystemService/Info Endpoint
  - Added logging API calls

## 2025-Feb-13

Code Maintenance:

- Updated code and comments in hydrogen.c
- Updated code and comments in Makefile

## 2025-Feb-12

Code Maintenance:

- Reduced logging detail, particularly for web sockets (lws)
- Improved shutdown process for web sockets
- Fixed file size and timestamp identification errors
- Moved startup and shutdown code to separate files

## 2025-Feb-08

Development Environment:

- Imported project into Visual Studio Code

</details>

<details>
<summary><h2>2024-Jul</h2></summary>

## 2024-Jul-18

WebSocket Server:

- Uses Authorization: Key abc in header
- Uses Protocol: hydrogen-protocol
- Implemented with libwebsockets

## 2024-Jul-15

System Improvements:

- mDNS code cleanup and updates:
  - Fixed log_this reliability issues
  - Fixed app_config memory issues
  - Improved shutdown code
- Added WebServer support for lithium content

## 2024-Jul-11

Network Infrastructure:

- mDNS and WebSockets groundwork:
  - Integrated code from nitrogen/nitro prototype

## 2024-Jul-08

Print Service:

- Implemented HTTP service for OrcaSlicer print requests:
  - Print job handling with /tmp storage using GUID filenames
  - JSON generation with filename mapping and beryllium-extracted data
  - PrintQueue integration for job management
  - /print/queue endpoint for queue inspection
  - Support for preview images embedded in G-code

</details>
