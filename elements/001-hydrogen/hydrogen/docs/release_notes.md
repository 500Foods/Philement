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
-->

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
