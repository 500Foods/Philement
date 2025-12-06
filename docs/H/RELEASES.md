# Release Notes
<!--

CRITICAL INSTRUCTIONS

The dev team for this project is based in Vancouver, so PACIFIC TIME applies

IMPORTANT: File paths in release notes should be relative to the RELEASES.md file location (project root).
When creating release notes in docs/releases/YYYY-MM/YYYY-MM-DD.md, use ../../../ prefix to navigate
from the release note file back to project root before descending to target files.

Step 1: Gather Changes
- Use git log to list ALL changed files for the day with the numbers representing the number of lines added or removed:
  ```
  git log --since="2025-04-04 00:00" --until="2025-04-04 23:59" --numstat --pretty=format: | awk '{add[$3]+=$1; del[$3]+=$2} END {for (f in add) printf "%d\t%d\t%s\n", add[f], del[f], f}' | sort -rn
  ```
- Ignore automatic build artifacts like COVERAGE.svg, COMPLETE.svg, CLOC_CODE.svg, CLOC_STAT.svg as they are updated by the build system
- Group files by subsystem/component
- For each significant component, examine detailed changes. Please request one file at a time as these can be quite large.
- Any file with more than 25 lines changed should guarantee an entry in the release notes, though lesser changes could qualify
  ```
  git log --since="YYYY-MM-DD 00:00" --until="YYYY-MM-DD 23:59" -p -- path/to/component
  ```
- Also use grep to search the repo for CHANGELOG entries, eg `grep -r '2025-08-07'` might return a list of CHANGELOG entries from various files

Step 2: Document Changes
- Keep entries concise and factual
- Focus on WHAT changed, not WHY
- Avoid marketing language ("comprehensive", "robust", etc.)
- Include links to key source files (2-3 per major change)
- Group related changes under clear topic headings

Step 3: Structure Format
- Start with topic heading (e.g., "WebSocket Server:", "Testing:")
- List specific changes as bullet points
- Include links to significant files in bullet points
- Example:
  ```
  Memory:
  - Added cleanup handlers [src/landing/landing.c]
  - Enhanced resource tracking [src/utils/utils.c]
  ```

Step 4: Quality Checks
- Verify all major changes are documented
- Ensure links point to actual changed files
- Confirm grouping is logical
- Remove any speculation or marketing language
- Keep focus on technical changes

Step 5: Update RELEASES.md
- Add link to new release notes file in this RELEASES.md
- Add entries with newest dates at the top of their respective month/year section
- Include brief summary after the link
- Use same format as existing links

Remember:
- This is a technical record, not marketing
- Every statement should be backed by commit evidence
- Include links to 2-3 key files per major change
- Group by topic to maintain clarity
-->

- December 2025
  - [2025-Dec-05 (Fri)](docs/releases/2025-12/2025-12-05.md): Test renumbering - Test 80 to Test 70
  - [2025-Dec-03 (Wed)](docs/releases/2025-12/2025-12-03.md): Installer system refactoring and standalone test implementation
  - [2025-Dec-02 (Tue)](docs/releases/2025-12/2025-12-02.md): Core handling enhancements, payload tools reorganization, and testing improvements
  - [2025-Dec-01 (Mon)](docs/releases/2025-12/2025-12-01.md): Installer updates, testing enhancements, and utility scripts

- November 2025
  - [2025-Nov-30 (Sun)](docs/releases/2025-11/2025-11-30.md): Terminal I/O enhancements and comprehensive testing
  - [2025-Nov-29 (Sat)](docs/releases/2025-11/2025-11-29.md): Test fixes and installer update
  - [2025-Nov-28 (Fri)](docs/releases/2025-11/2025-11-28.md): Brotli UDF enhancements and migration fixes
  - [2025-Nov-27 (Thu)](docs/releases/2025-11/2025-11-27.md): Brotli decompression UDFs and installer updates
  - [2025-Nov-26 (Wed)](docs/releases/2025-11/2025-11-26.md): Theme migrations, DB2 parameter handling, and automated updates
  - [2025-Nov-25 (Tue)](docs/releases/2025-11/2025-11-25.md): Database migration and diagram enhancements
  - [2025-Nov-24 (Mon)](docs/releases/2025-11/2025-11-24.md): Database query and migration updates
  - [2025-Nov-23 (Sun)](docs/releases/2025-11/2025-11-23.md): Database parameter handling and migration improvements
  - [2025-Nov-22 (Sat)](docs/releases/2025-11/2025-11-22.md): Database connection and query enhancements
  - [2025-Nov-21 (Fri)](docs/releases/2025-11/2025-11-21.md): Database migration and testing updates
  - [2025-Nov-20 (Thu)](docs/releases/2025-11/2025-11-20.md): Database query and migration enhancements
  - [2025-Nov-18 (Tue)](docs/releases/2025-11/2025-11-18.md): Database JSON handling and query enhancements
  - [2025-Nov-16 (Sun)](docs/releases/2025-11/2025-11-16.md): Database diagram testing and migration load testing
  - [2025-Nov-15 (Sat)](docs/releases/2025-11/2025-11-15.md): SQLite connection and type enhancements
  - [2025-Nov-14 (Fri)](docs/releases/2025-11/2025-11-14.md): Diagram generation and DB2 UDF enhancements
  - [2025-Nov-13 (Thu)](docs/releases/2025-11/2025-11-13.md): DB2 base64 UDF and API conduit updates
  - [2025-Nov-11 (Tue)](docs/releases/2025-11/2025-11-11.md): Swagger and database parameter enhancements
  - [2025-Nov-10 (Mon)](docs/releases/2025-11/2025-11-10.md): Launch subsystem unit testing expansion
  - [2025-Nov-09 (Sun)](docs/releases/2025-11/2025-11-09.md): Comprehensive database unit testing expansion
  - [2025-Nov-08 (Sat)](docs/releases/2025-11/2025-11-08.md): Logging, mutex, and testing framework enhancements
  - [2025-Nov-07 (Fri)](docs/releases/2025-11/2025-11-07.md): Database engine and queue management updates
  - [2025-Nov-06 (Thu)](docs/releases/2025-11/2025-11-06.md): Database migration execution and engine updates
  - [2025-Nov-05 (Wed)](docs/releases/2025-11/2025-11-05.md): Database queue migration operations
  - [2025-Nov-04 (Tue)](docs/releases/2025-11/2025-11-04.md): Database pending operations and parameters
  - [2025-Nov-03 (Mon)](docs/releases/2025-11/2025-11-03.md): Database connection and caching enhancements

- October 2025
  - [2025-Oct-30 (Thu)](docs/releases/2025-10/2025-10-30.md): Database query implementation updates
  - [2025-Oct-29 (Wed)](docs/releases/2025-10/2025-10-29.md): Database migration system enhancements
  - [2025-Oct-25 (Fri)](docs/releases/2025-10/2025-10-25.md): Database bootstrap coverage completion
  - [2025-Oct-23 (Thu)](docs/releases/2025-10/2025-10-23.md): CONDUIT query coverage completion
  - [2025-Oct-22 (Wed)](docs/releases/2025-10/2025-10-22.md): CONDUIT query coverage improvements
  - [2025-Oct-21 (Tue)](docs/releases/2025-10/2025-10-21.md): Database parameters, CONDUIT query handling
  - [2025-Oct-20 (Mon)](docs/releases/2025-10/2025-10-20.md): CONDUIT service implementation, database cache management
  - [2025-Oct-19 (Sat)](docs/releases/2025-10/2025-10-19.md): Lead DQM algorithm, migration testing
  - [2025-Oct-18 (Fri)](docs/releases/2025-10/2025-10-18.md): Database configuration tests, migration updates
  - [2025-Oct-17 (Fri)](docs/releases/2025-10/2025-10-17.md): Terminal websocket protocol, TODO tracking
  - [2025-Oct-16 (Thu)](docs/releases/2025-10/2025-10-16.md): Database queue lead implementation
  - [2025-Oct-15 (Wed)](docs/releases/2025-10/2025-10-15.md): Migration updates, Lua check test, diagram generation
  - [2025-Oct-13 (Mon)](docs/releases/2025-10/2025-10-13.md): SQLite support, terminal websocket bridge
  - [2025-Oct-12 (Sun)](docs/releases/2025-10/2025-10-12.md): Database query improvements, prepared statements
  - [2025-Oct-11 (Sat)](docs/releases/2025-10/2025-10-11.md): Migration path handling, backup cleanup
  - [2025-Oct-10 (Fri)](docs/releases/2025-10/2025-10-10.md): Database reorganization, CMake updates
  - [2025-Oct-09 (Thu)](docs/releases/2025-10/2025-10-09.md): Database queue creation, migration file handling
  - [2025-Oct-08 (Wed)](docs/releases/2025-10/2025-10-08.md): Database migration execution, Lua integration
  - [2025-Oct-07 (Tue)](docs/releases/2025-10/2025-10-07.md): Database migrations, Lead DQM implementation
  - [2025-Oct-06 (Mon)](docs/releases/2025-10/2025-10-06.md): Database launch logging, migration transaction handling
  - [2025-Oct-05 (Sun)](docs/releases/2025-10/2025-10-05.md): WebSocket coverage, launch network improvements
  - [2025-Oct-04 (Sat)](docs/releases/2025-10/2025-10-04.md): WebSocket unit tests, coverage improvements
  - [2025-Oct-02 (Thu)](docs/releases/2025-10/2025-10-02.md): Launch system improvements, logging cleanup
  - [2025-Oct-01 (Wed)](docs/releases/2025-10/2025-10-01.md): Database designs, unit tests, documentation updates, build enhancements

- September 2025
  - [2025-Sep-30 (Tue)](docs/releases/2025-09/2025-09-30.md): Database migration files, test framework updates, code organization
  - [2025-Sep-29 (Mon)](docs/releases/2025-09/2025-09-29.md): Database diagram improvements, SVG generation, test enhancements
  - [2025-Sep-28 (Sun)](docs/releases/2025-09/2025-09-28.md): Database diagram generation, preprocessing functions, test script
  - [2025-Sep-26 (Fri)](docs/releases/2025-09/2025-09-26.md): Installer System, Database Migration Testing
  - [2025-Sep-25 (Thu)](docs/releases/2025-09/2025-09-25.md): Helium Database System, Test Framework Improvements
  - [2025-Sep-24 (Wed)](docs/releases/2025-09/2025-09-24.md): Payload Subsystem Enhancement, Build Optimization
  - [2025-Sep-23 (Tue)](docs/releases/2025-09/2025-09-23.md): Build System Enhancement, File Organization
  - [2025-Sep-22 (Mon)](docs/releases/2025-09/2025-09-22.md): Build System Enhancement, Unity Testing Expansion
  - [2025-Sep-21 (Sun)](docs/releases/2025-09/2025-09-21.md): Database Backend Refinement, Testing Framework
  - [2025-Sep-20 (Sat)](docs/releases/2025-09/2025-09-20.md): Database System Optimization, Test Coverage
  - [2025-Sep-19 (Fri)](docs/releases/2025-09/2025-09-19.md): WebSocket Server Enhancement, Database Integration
  - [2025-Sep-18 (Thu)](docs/releases/2025-09/2025-09-18.md): Terminal Subsystem Refinement, API Improvements
  - [2025-Sep-17 (Wed)](docs/releases/2025-09/2025-09-17.md): Configuration System Updates, Mutex Integration
  - [2025-Sep-16 (Tue)](docs/releases/2025-09/2025-09-16.md): Launch System Enhancement, Payload Processing
  - [2025-Sep-15 (Mon)](docs/releases/2025-09/2025-09-15.md): mDNS Server Improvements, Web Server Updates
  - [2025-Sep-14 (Sun)](docs/releases/2025-09/2025-09-14.md): Logging System Refinement, System Integration
  - [2025-Sep-13 (Sat)](docs/releases/2025-09/2025-09-13.md): Database Backend Enhancements, Test Framework
  - [2025-Sep-12 (Fri)](docs/releases/2025-09/2025-09-12.md): Database Backend Enhancements, WebSocket Testing
  - [2025-Sep-11 (Thu)](docs/releases/2025-09/2025-09-11.md): Database Backends Implementation, Mutex Library, System Integration
  - [2025-Sep-10 (Wed)](docs/releases/2025-09/2025-09-10.md): Database Modularization
  - [2025-Sep-09 (Tue)](docs/releases/2025-09/2025-09-09.md): Database Queue Refinement, WebSocket Processing, Terminal I/O
  - [2025-Sep-08 (Mon)](docs/releases/2025-09/2025-09-08.md): Database Architecture, Terminal Enhancements, Test Framework
  - [2025-Sep-07 (Sat)](docs/releases/2025-09/2025-09-07.md): Installer System, Test Framework Improvements
  - [2025-Sep-06 (Fri)](docs/releases/2025-09/2025-09-06.md): Installer System, Test Framework Improvements
  - [2025-Sep-05 (Thu)](docs/releases/2025-09/2025-09-05.md): Database Subsystem Implementation, Test Framework Improvements
  - [2025-Sep-04 (Wed)](docs/releases/2025-09/2025-09-04.md): Installer System, Test Framework Improvements
  - [2025-Sep-03 (Tue)](docs/releases/2025-09/2025-09-03.md): Database System Planning, Test Framework Improvements
  - [2025-Sep-01 (Sun)](docs/releases/2025-09/2025-09-01.md): Terminal Subsystem Implementation, WebSocket Server Enhancement

- August 2025
  - [2025-Aug-31 (Sun)](docs/releases/2025-08/2025-08-31.md): Terminal Subsystem Implementation, Test Framework Improvements
  - [2025-Aug-30 (Sat)](docs/releases/2025-08/2025-08-30.md): Launch/Landing System Enhancement, Documentation Updates
  - [2025-Aug-29 (Fri)](docs/releases/2025-08/2025-08-29.md): mDNS Subsystem Enhancement, Test Framework Improvements
  - [2025-Aug-28 (Thu)](docs/releases/2025-08/2025-08-28.md): Test Framework Improvements, Unity Testing Expansion
  - [2025-Aug-27 (Wed)](docs/releases/2025-08/2025-08-27.md): Configuration System Enhancement, Unity Testing Expansion
  - [2025-Aug-26 (Tue)](docs/releases/2025-08/2025-08-26.md): Configuration System Enhancement, Unity Testing Expansion
  - [2025-Aug-25 (Mon)](docs/releases/2025-08/2025-08-25.md): Launch/Landing System Enhancement, Unity Testing Expansion
  - [2025-Aug-24 (Sun)](docs/releases/2025-08/2025-08-24.md): Print Subsystem Enhancement, Upload System Implementation
  - [2025-Aug-23 (Sat)](docs/releases/2025-08/2025-08-23.md): Unity Testing Expansion
  - [2025-Aug-22 (Fri)](docs/releases/2025-08/2025-08-22.md): Documentation Updates
  - [2025-Aug-21 (Thu)](docs/releases/2025-08/2025-08-21.md): Test Framework Improvements
  - [2025-Aug-20 (Wed)](docs/releases/2025-08/2025-08-20.md): Thread Management Enhancement
  - [2025-Aug-19 (Tue)](docs/releases/2025-08/2025-08-19.md): Core System Architecture Updates
  - [2025-Aug-18 (Mon)](docs/releases/2025-08/2025-08-18.md): Test Framework Maintenance
  - [2025-Aug-17 (Sun)](docs/releases/2025-08/2025-08-17.md): Test Framework Maintenance
  - [2025-Aug-16 (Sat)](docs/releases/2025-08/2025-08-16.md): Test Framework Maintenance
  - [2025-Aug-15 (Fri)](docs/releases/2025-08/2025-08-15.md): Test Framework Maintenance
  - [2025-Aug-14 (Thu)](docs/releases/2025-08/2025-08-14.md): Test Framework Maintenance
  - [2025-Aug-13 (Wed)](docs/releases/2025-08/2025-08-13.md): Test Framework Maintenance
  - [2025-Aug-12 (Tue)](docs/releases/2025-08/2025-08-12.md): Test Framework Maintenance
  - [2025-Aug-11 (Mon)](docs/releases/2025-08/2025-08-11.md): Unity Testing Expansion
  - [2025-Aug-10 (Sun)](docs/releases/2025-08/2025-08-10.md): Coverage System Optimization
  - [2025-Aug-09 (Sat)](docs/releases/2025-08/2025-08-09.md): Swagger Testing Enhancement
  - [2025-Aug-08 (Fri)](docs/releases/2025-08/2025-08-08.md): Signal Handling Testing
  - [2025-Aug-07 (Wed)](docs/releases/2025-08/2025-08-07.md): Shutdown Timing Enhancement, Timing System
  - [2025-Aug-05 (Tue)](docs/releases/2025-08/2025-08-05.md): Coverage System Optimization, Testing Framework Performance
  - [2025-Aug-04 (Mon)](docs/releases/2025-08/2025-08-04.md): GCOV Integration, Coverage System Enhancement, Performance Optimization
  - [2025-Aug-03 (Sun)](docs/releases/2025-08/2025-08-03.md): Testing Framework Metrics, Script Optimization
  - [2025-Aug-02 (Sat)](docs/releases/2025-08/2025-08-02.md): API Documentation, Swagger CLI Validation
  - [2025-Aug-01 (Fri)](docs/releases/2025-08/2025-08-01.md): Testing Framework Improvements

- July 2025
  - [2025-Jul-31 (Thu)](docs/releases/2025-07/2025-07-31.md): Major Testing Framework Overhaul Complete, Code Quality
  - [2025-Jul-30 (Wed)](docs/releases/2025-07/2025-07-30.md): Testing Framework Cleanup, Signal Handling
  - [2025-Jul-29 (Tue)](docs/releases/2025-07/2025-07-29.md): Performance Optimization, Caching Implementation
  - [2025-Jul-28 (Mon)](docs/releases/2025-07/2025-07-28.md): Testing Framework Cleanup, Performance Improvements
  - [2025-Jul-27 (Sun)](docs/releases/2025-07/2025-07-27.md): Code Quality, Shellcheck Updates
  - [2025-Jul-25 (Fri)](docs/releases/2025-07/2025-07-25.md): Script Infrastructure Improvements
  - [2025-Jul-24 (Thu)](docs/releases/2025-07/2025-07-24.md): Script Infrastructure, Testing Framework
  - [2025-Jul-23 (Wed)](docs/releases/2025-07/2025-07-23.md): Major Shellcheck Overhaul, Testing Framework
  - [2025-Jul-22 (Tue)](docs/releases/2025-07/2025-07-22.md): Comprehensive Shellcheck Validation
  - [2025-Jul-21 (Mon)](docs/releases/2025-07/2025-07-21.md): Code Quality, Testing Framework
  - [2025-Jul-20 (Sun)](docs/releases/2025-07/2025-07-20.md): Testing Framework, Script Infrastructure
  - [2025-Jul-19 (Sat)](docs/releases/2025-07/2025-07-19.md): Documentation System, Testing Framework, Build System
  - [2025-Jul-18 (Fri)](docs/releases/2025-07/2025-07-18.md): WebSocket Shutdown Optimization, Testing Framework SVG Visualization
  - [2025-Jul-17 (Thu)](docs/releases/2025-07/2025-07-17.md): WebSocket Naming Consistency
  - [2025-Jul-16 (Wed)](docs/releases/2025-07/2025-07-16.md): Unity Test Batching Algorithm Improvement
  - [2025-Jul-15 (Tue)](docs/releases/2025-07/2025-07-15.md): Unity Testing Expansion with WebSocket and Swagger Focus
  - [2025-Jul-14 (Mon)](docs/releases/2025-07/2025-07-14.md): Test Framework Architecture Overhaul
  - [2025-Jul-13 (Sun)](docs/releases/2025-07/2025-07-13.md): Coverage Improvements & Test Reliability
  - [2025-Jul-12 (Sat)](docs/releases/2025-07/2025-07-12.md): Coverage Tools and Build Updates
  - [2025-Jul-11 (Fri)](docs/releases/2025-07/2025-07-11.md): Coverage and Testing Enhancements
  - [2025-Jul-10 (Thu)](docs/releases/2025-07/2025-07-10.md): Documentation Updates
  - [2025-Jul-09 (Wed)](docs/releases/2025-07/2025-07-09.md): Testing Infrastructure
  - [2025-Jul-07 (Mon)](docs/releases/2025-07/2025-07-07.md): Testing Updates
  - [2025-Jul-06 (Sun)](docs/releases/2025-07/2025-07-06.md): Testing Updates
  - [2025-Jul-04 (Fri)](docs/releases/2025-07/2025-07-04.md): Testing Updates
  - [2025-Jul-03 (Thu)](docs/releases/2025-07/2025-07-03.md): Documentation, Testing Updates
  - [2025-Jul-02 (Wed)](docs/releases/2025-07/2025-07-02.md): Testing Framework, Documentation
  - [2025-Jul-01 (Tue)](docs/releases/2025-07/2025-07-01.md): CMake Build System, OIDC Examples

- June 2025
  - [2025-Jun-30 (Mon)](docs/releases/2025-06/2025-06-30.md): Unity Testing Framework, Testing Documentation
  - [2025-Jun-29 (Sun)](docs/releases/2025-06/2025-06-29.md): Launch System, Documentation
  - [2025-Jun-28 (Sat)](docs/releases/2025-06/2025-06-28.md): Launch System, Documentation
  - [2025-Jun-25 (Wed)](docs/releases/2025-06/2025-06-25.md): CMake Build System, Unity Testing, Documentation
  - [2025-Jun-23 (Mon)](docs/releases/2025-06/2025-06-23.md): Documentation
  - [2025-Jun-20 (Fri)](docs/releases/2025-06/2025-06-20.md): Testing, Documentation
  - [2025-Jun-19 (Thu)](docs/releases/2025-06/2025-06-19.md): Documentation
  - [2025-Jun-18 (Wed)](docs/releases/2025-06/2025-06-18.md): Testing, Documentation
  - [2025-Jun-17 (Tue)](docs/releases/2025-06/2025-06-17.md): Testing, Payloads, Documentation
  - [2025-Jun-16 (Mon)](docs/releases/2025-06/2025-06-16.md): Testing, Documentation
  - [2025-Jun-09 (Mon)](docs/releases/2025-06/2025-06-09.md): Documentation
  - [2025-Jun-03 (Tue)](docs/releases/2025-06/2025-06-03.md): Examples
  - [2025-Jun-01 (Sun)](docs/releases/2025-06/2025-06-01.md): Status System

- May 2025
  - [2025-May-06 (Tue)](docs/releases/2025-05/2025-05-06.md): Documentation

- April 2025
  - [2025-Apr-19 (Sat)](docs/releases/2025-04/2025-04-19.md): Documentation
  - [2025-Apr-05 (Sat)](docs/releases/2025-04/2025-04-05.md): Logging System, Shutdown System, Release Notes
  - [2025-Apr-04 (Thu)](docs/releases/2025-04/2025-04-04.md): OIDC Integration, Web Server, Launch System, Testing
  - [2025-Apr-03 (Wed)](docs/releases/2025-04/2025-04-03.md): Status System, Launch System, Metrics and Monitoring, Configuration
  - [2025-Apr-02 (Tue)](docs/releases/2025-04/2025-04-02.md): Launch System, Network Infrastructure, Configuration
  - [2025-Apr-01 (Mon)](docs/releases/2025-04/2025-04-01.md): Build System, Testing Framework, Configuration System, Launch/Landing System
  
- March 2025
  - [2025-Mar-31 (Mon)](docs/releases/2025-03/2025-03-31.md): Landing System Architecture, Core System, State Management
  - [2025-Mar-30 (Sun)](docs/releases/2025-03/2025-03-30.md): Core System, Registry System, Thread Management, Shutdown System
  - [2025-Mar-29 (Sat)](docs/releases/2025-03/2025-03-29.md): Launch System, Landing System, Subsystem Architecture
  - [2025-Mar-28 (Fri)](docs/releases/2025-03/2025-03-28.md): Thread Management, Launch/Landing Integration, Subsystem Architecture
  - [2025-Mar-27 (Thu)](docs/releases/2025-03/2025-03-27.md): System Integration, Performance Monitoring, Testing
  - [2025-Mar-26 (Wed)](docs/releases/2025-03/2025-03-26.md): Subsystem Integration, State Management, Registry System
  - [2025-Mar-25 (Tue)](docs/releases/2025-03/2025-03-25.md): Thread Management, Thread Safety, Resource Management
  - [2025-Mar-24 (Mon)](docs/releases/2025-03/2025-03-24.md): Logging System, Monitoring, Resource Management
  - [2025-Mar-23 (Sun)](docs/releases/2025-03/2025-03-23.md): WebServer Subsystem, Error Handling, Integration
  - [2025-Mar-22 (Sat)](docs/releases/2025-03/2025-03-22.md): Launch System, System Architecture, Documentation
  - [2025-Mar-21 (Fri)](docs/releases/2025-03/2025-03-21.md): Configuration System, File Handling, Environment Integration
  - [2025-Mar-20 (Thu)](docs/releases/2025-03/2025-03-20.md): System Control, Signal Management, Startup Handling, Logging Improvements
  - [2025-Mar-19 (Wed)](docs/releases/2025-03/2025-03-19.md): Subsystem Management, Shutdown Functions, Documentation, Error Handling
  - [2025-Mar-18 (Tue)](docs/releases/2025-03/2025-03-18.md): Repository Integration, GitHub Workflows, JSON Configuration, Testing
  - [2025-Mar-17 (Mon)](docs/releases/2025-03/2025-03-17.md): System Enhancements, Launch Readiness, Shutdown Process, Testing
  - [2025-Mar-16 (Sun)](docs/releases/2025-03/2025-03-16.md): Implementation Preparation, Shutdown System, Restart Functionality, Integration Planning
  - [2025-Mar-15 (Sat)](docs/releases/2025-03/2025-03-15.md): Development Planning, Architecture Design, Code Organization
  - [2025-Mar-14 (Wed)](docs/releases/2025-03/2025-03-14.md): Repository Infrastructure, GitHub Workflows, Release Documentation
  - [2025-Mar-13 (Wed)](docs/releases/2025-03/2025-03-13.md): Configuration System, API Configuration, Logging Configuration
  - [2025-Mar-12 (Wed)](docs/releases/2025-03/2025-03-12.md): Code Quality, Testing
  - [2025-Mar-11 (Mon)](docs/releases/2025-03/2025-03-11.md): Configuration System, WebSocket Server, System Architecture, Web Server
  - [2025-Mar-10 (Sun)](docs/releases/2025-03/2025-03-10.md): Configuration System, Network Infrastructure, OIDC Service, Testing Framework
  - [2025-Mar-09 (Sat)](docs/releases/2025-03/2025-03-09.md): Network Infrastructure, Documentation System, SMTP Relay
  - [2025-Mar-08 (Fri)](docs/releases/2025-03/2025-03-08.md): Thread Management, Logging System, Service Infrastructure
  - [2025-Mar-07 (Thu)](docs/releases/2025-03/2025-03-07.md): Code Cleanup, Configuration Structure, Startup Sequence
  - [2025-Mar-06 (Thu)](docs/releases/2025-03/2025-03-06.md): Configuration System, Documentation
  - [2025-Mar-05 (Wed)](docs/releases/2025-03/2025-03-05.md): API, WebSocket Server, Web Server, Testing, Documentation, State Management
  - [2025-Mar-04 (Tue)](docs/releases/2025-03/2025-03-04.md): Web Server, Configuration, Testing
  - [2025-Mar-03 (Mon)](docs/releases/2025-03/2025-03-03.md): Web Server, Testing, Service Discovery
  - [2025-Mar-02 (Sun)](docs/releases/2025-03/2025-03-02.md): API Documentation, Build System, Configuration, Testing
  - [2025-Mar-01 (Sat)](docs/releases/2025-03/2025-03-01.md): API Architecture, Web Server, Testing, Build System

- February 2025
  - [2025-Feb-28 (Fri)](docs/releases/2025-02/2025-02-28.md): Build System, Testing, Code Quality, Configuration, API
  - [2025-Feb-27 (Thu)](docs/releases/2025-02/2025-02-27.md): Code Quality, OIDC Service, Testing, Documentation
  - [2025-Feb-26 (Wed)](docs/releases/2025-02/2025-02-26.md): Testing, Shutdown Architecture, Documentation
  - [2025-Feb-25 (Tue)](docs/releases/2025-02/2025-02-25.md): Testing System, Documentation
  - [2025-Feb-24 (Mon)](docs/releases/2025-02/2025-02-24.md): WebSocket Server, mDNS Server, Documentation
  - [2025-Feb-23 (Sun)](docs/releases/2025-02/2025-02-23.md): Core System
  - [2025-Feb-22 (Sat)](docs/releases/2025-02/2025-02-22.md): Server Management, WebSocket Server, Logging
  - [2025-Feb-21 (Fri)](docs/releases/2025-02/2025-02-21.md): Queue System, System Information
  - [2025-Feb-20 (Thu)](docs/releases/2025-02/2025-02-20.md): WebSocket Server
  - [2025-Feb-19 (Wed)](docs/releases/2025-02/2025-02-19.md): System Status, Documentation
  - [2025-Feb-18 (Tue)](docs/releases/2025-02/2025-02-18.md): Network Infrastructure
  - [2025-Feb-17 (Mon)](docs/releases/2025-02/2025-02-17.md): Documentation, System Metrics
  - [2025-Feb-16 (Sun)](docs/releases/2025-02/2025-02-16.md): Code Quality, Configuration
  - [2025-Feb-15 (Sat)](docs/releases/2025-02/2025-02-15.md): Documentation, Configuration
  - [2025-Feb-14 (Fri)](docs/releases/2025-02/2025-02-14.md): API Development
  - [2025-Feb-13 (Thu)](docs/releases/2025-02/2025-02-13.md): Code Maintenance
  - [2025-Feb-12 (Wed)](docs/releases/2025-02/2025-02-12.md): Code Maintenance
  - [2025-Feb-08 (Fri)](docs/releases/2025-02/2025-02-08.md): Development Environment

- July 2024
  - [2024-Jul-18 (Thu)](docs/releases/2024-07/2024-07-18.md): WebSocket Server
  - [2024-Jul-15 (Mon)](docs/releases/2024-07/2024-07-15.md): System Improvements
  - [2024-Jul-11 (Thu)](docs/releases/2024-07/2024-07-11.md): Network Infrastructure
  - [2024-Jul-08 (Mon)](docs/releases/2024-07/2024-07-08.md): Print Service
