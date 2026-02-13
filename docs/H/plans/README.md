# PLANS

These started life typically as AI conversations that morphed into a wishlist, then into an implementation plan, and finally into a completed to-do list for the project. The remains tend to be a smoking husk compared to what started as an egalitarian vision, but the journey may offer some insight into how we've gone from lightbulb to implementation.

## [VICTORIALOGGING](/docs/H/plans/VICTORIALOGGING.md)

Implmenentation plan for adding support for VictoriaLogs to Hydrogen.

## [TO-DO](/docs/H/plans/TODO.md)
  
The Development Roadmap serves as a central location to track incomplete items across the Hydrogen project. It organizes tasks by subsystem (API, Database, Launch, Logging, OIDC, Print) with difficulty-based categorization from Easy to Hard. The document provides detailed status tracking for each task, including specific file locations and descriptions of what needs to be implemented. It also includes a comprehensive section on test management, outlining procedures for fixing failing tests, removing invaluable tests, and maintaining test quality across the codebase.

## [CONDUIT](/docs/H/plans/CONDUIT.md)

This describes implementation of a RESTful query execution endpoint for Hydrogen that allows clients to execute pre-defined database queries by ID reference. It details the Query Table Cache (QTC) design with in-memory caching of SQL templates, typed parameter format supporting INTEGER/STRING/BOOLEAN/FLOAT types, and automatic conversion of named parameters to database-specific formats. The document covers intelligent queue selection algorithms, synchronous execution with blocking wait mechanisms, and comprehensive API specifications with detailed request/response formats and error handling strategies.

## [CONDUIT](/docs/H/plans/CONDUIT_FIX.md)

It has been a struggle to get through all the conduit implementation details, stalling at one point with auth_query. So a new plan was hatched to delve more into its details to try to get it moving forward again. Switching models (Kimi K2.5) also seemed to help break the log jam.  Seems grok-code-fast-1 is becoming stupider by the day for some reason.

## [MIRAGE PLAN](/docs/H/plans/MIRAGE_PLAN.md)

Mirage describes a new subystem that implements a distributed proxy network architecture where public Mirage servers act as multi-tenant proxy hubs for remote Hydrogen instances. It outlines the dual-server architecture with transparent proxying of both HTTP and WebSocket traffic, preserving remote server customizations and feature parity. The document covers security considerations including mutual authentication and end-to-end encryption, protocol extensions for tunnel establishment, and scalability features supporting thousands of remote devices with load balancing and automatic failover capabilities.

## [DATABASE PLAN](/docs/H/plans/DATABASE_PLAN.md) ✅ **COMPLETED**

The Database Architecture Plan outlines the complete implementation of Hydrogen's multi-engine database subsystem. It covers the Database Queue Manager (DQM) architecture with dynamic child queue spawning, thread-safe operations, and comprehensive support for PostgreSQL, SQLite, MySQL, and IBM DB2 engines. The document details the two-phase migration workflow (LOAD and APPLY phases), bootstrap query integration, trigger-based cache invalidation, and extensive testing strategies. The plan also includes detailed implementation status tracking, showing completed phases for DQM infrastructure, multi-engine interface layers, and all database engine implementations.

## [TERMINAL PLAN](/docs/H/plans/TERMINAL_PLAN.md) ✅ **COMPLETED**

The Terminal Interface Plan provides a comprehensive roadmap for implementing an xterm.js-based terminal subsystem for Hydrogen. It details the architecture using xterm.js frontend with C backend PTY management and WebSocket data flow, following Hydrogen's established subsystem patterns. The document covers all implementation phases including configuration infrastructure with WebRoot and CORS support, payload generation for xterm.js assets, file serving with WebRoot support, PTY process management, WebSocket communication, and comprehensive testing frameworks. It also includes extensive lessons learned from authentication challenges and detailed success criteria for terminal functionality.

## [MIGRATIONS](/docs/H/plans/MIGRATIONS.md) ✅ **COMPLETED**

The Migration Strategy document focuses on optimizing database migration performance across PostgreSQL, DB2, SQLite, and MySQL engines. It provides detailed performance analysis showing significant variance between engines and identifies specific bottlenecks including transaction command complexity, excessive timeout management, and cleanup inefficiencies. The plan outlines targeted optimizations for PostgreSQL (primary focus) and DB2 (secondary focus), with specific implementation changes to connection-level timeout management, transaction command simplification, and batch cleanup operations.
