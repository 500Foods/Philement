# Release Notes

<!--
CRITICAL INSTRUCTIONS

The dev team for this project is based in Vancouver, so PACIFIC TIME applies

Step 1: Gather Changes
- Use git log to list ALL changed files for the day with the numbers representing the number of lines added or removed:
  ```
  git log --since="2025-04-04 00:00" --until="2025-04-04 23:59" --numstat --pretty=format: | awk '{add[$3]+=$1; del[$3]+=$2} END {for (f in add) printf "%d\t%d\t%s\n", add[f], del[f], f}' | sort -rn
  ```
- Group files by subsystem/component
- For each significant component, examine detailed changes. Please request one file at a time as these can be quite large.
- Any file with more than 25 lines changed should guarantee an entry in the release notes, though lesser changes could qualify
  ```
  git log --since="YYYY-MM-DD 00:00" --until="YYYY-MM-DD 23:59" -p -- path/to/component
  ```
- Also use grep to search the repo, eg `grep -r '2025-08-07'` might return a list of CHANGELOG entries from various files

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

- August 2025
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
