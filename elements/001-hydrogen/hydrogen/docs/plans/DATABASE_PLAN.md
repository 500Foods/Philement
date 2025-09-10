# DATABASE SUBSYSTEM IMPLEMENTATION PLAN

## ESSENTIAL PREREQUISITES

- [RECIPE.md](../../RECIPE.md) - C development standards and build processes
- [tests/README.md](../../tests/README.md) - Testing framework overview
- [docs/reference/database_architecture.md](../reference/database_architecture.md) - Architecture specs
- [src/queue/queue.c](../../src/queue/queue.c) - Queue infrastructure to extend
- [src/config/config_databases.c](../../src/config/config_databases.c) - Existing config system
- [CMAKE/README.md](../../cmake/README.md) - Build system for database engines

## ARCHITECTURE OVERVIEW

Hydrogen serves as a database gateway supporting PostgreSQL, SQLite, MySQL, DB2 with AI-ready query architecture. Key patterns:

- **Queue-Based Processing**: slow/medium/fast queues per database with priority routing
- **Query ID System**: REST API passes query numbers + parameters for schema independence
- **Cross-Database Hosting**: One database can serve queries for multiple downstream databases
- **Lead Queue Pattern**: Dedicated admin queue for cache management, triggers, and system heartbeats
  - **Automatic Creation**: Lead queue is always created automatically for each database
  - **Non-Scalable**: Cannot be scaled down or disabled - always maintains one worker
  - **Management Role**: Handles connection heartbeats, trigger monitoring, and cross-queue coordination
  - **Tag Assignment**: Always tagged with "Lead" and inherits tags from disabled queues

## EXISTING SUBSYSTEM STATE (UPDATED 9/10/2025 - TEST 27 OPERATIONAL)

**ARCHITECTURAL CHANGE:** The subsystem has evolved from the planned multi-queue-per-database approach to a **Database Queue Manager (DQM)** architecture where each database gets a single **Lead queue** that dynamically spawns **child worker queues** for different priority levels (slow/medium/fast/cache).

```c
// Configuration Layer - ENHANCED
src/config/config_defaults.c        // Multi-engine configuration defaults
                                      // Support for PostgreSQL, MySQL, SQLite, DB2
                                      // 5 database connection slots available
                                      // Environment variable configuration
                                      // Selective enable/disable per database

// Launch Readiness Checks ✅ COMPLETED 9/5/2025
src/config/config_databases.c       // Launch readiness validation implementation
src/launch/launch_databases.c       // Database readiness checks in launch sequence

// Launch/Landing Integration - EXISTING/ENHANCED
src/launch/launch_database.c        // Basic connectivity validation
src/landing/landing_database.c      // Config cleanup only

// DQM (Database Queue Manager) Architecture ✅ COMPLETED 9/8/2025
src/database/database_queue.h       // DQM infrastructure and Lead queue definitions
src/database/database_queue.c       // Full DQM implementation (~960 lines)
                                      // Lead queue spawns child queues dynamically
                                      // Thread-safe queue management and worker threads
                                      // Hierarchical queue architecture (Lead + children)

// Multi-Engine Interface Layer ✅ COMPLETED 9/5/2025
src/database/database.h             // DatabaseEngineInterface with function pointers
src/database/database_engine.h      // Engine abstraction layer header
src/database/database_engine.c      // Engine registry and unified API (~280 lines)
                                      // Support for PostgreSQL, SQLite, MySQL, DB2, AI engines
                                      // Connection management, query execution, transactions
                                      // Health checks and connection reset functionality

// Core Database Subsystem ✅ COMPLETED 9/8/2025
src/database/database.c             // Database subsystem core (~408 lines)
                                      // DQM integration and database management
                                      // PostgreSQL engine fully implemented and working
                                      // SQLite, MySQL, DB2 engines have placeholder implementations
```

## IMPLEMENTATION PHASES

### Phase 1: DQM (Database Queue Manager) Infrastructure ✅ **COMPLETED 9/8/2025**

**ARCHITECTURAL EVOLUTION:** Changed from multiple static queues per database to dynamic **Database Queue Manager (DQM)** architecture.

**Core Components (Implemented):**

```c
typedef struct DatabaseQueue {
    char* database_name;           // Database identifier (e.g., "Acuranzo")
    char* connection_string;       // Database connection string
    char* queue_type;              // Queue type: "Lead", "slow", "medium", "fast", "cache"

    // Single queue instance - type determined by queue_type
    Queue* queue;                  // The actual queue for this worker

    // Lead queue management (only used by Lead queues)
    DatabaseQueue** child_queues;  // Array of spawned queues (slow/medium/fast/cache)
    int child_queue_count;         // Number of active child queues
    int max_child_queues;          // Maximum child queues allowed
    pthread_mutex_t children_lock; // Protects child queue operations

    // Queue role flags
    volatile bool is_lead_queue;   // True if this is the Lead queue for the database
    volatile bool can_spawn_queues; // True if this queue can create additional queues

    // Worker thread management - single thread per queue
    pthread_t worker_thread;

    // Queue statistics
    volatile int active_connections;
    volatile int total_queries_processed;
    volatile int current_queue_depth;

    // Thread synchronization
    pthread_mutex_t queue_access_lock;
    sem_t worker_semaphore;        // Controls worker thread operation

    // Flags
    volatile bool shutdown_requested;
    volatile bool is_connected;
};

typedef struct DatabaseQueueManager {
    DatabaseQueue** databases;      // Array of database Lead queues
    size_t database_count;          // Number of databases managed
    size_t max_databases;           // Maximum supported databases
    // Round-robin distribution state and thread synchronization
};
```

**Key Implementation (Completed):**

- ✅ **DQM Architecture**: Each database gets one Lead queue that spawns child worker queues
- ✅ **Dynamic Queue Spawning**: Lead queues can spawn/destroy child queues based on workload
- ✅ **Hierarchical Management**: Lead queue coordinates child queues and manages database connections
- ✅ **Thread-Safe Operations**: All queue operations use proper synchronization primitives
- ✅ **Single Worker Thread Pattern**: Each queue (Lead or child) has exactly one worker thread
- ✅ **Statistics Monitoring**: Comprehensive queue depth, health checks, and performance metrics
- ✅ **Integration with Existing Systems**: Works with existing queue infrastructure and launch/landing

**DQM State Management:**

- **State Tracking**: Each queue maintains state (CONNECTING, CONNECTED, ERROR, DISCONNECTED)
- **State Reporting**: Enhanced logging with DQM component names: `DQM-Acuranzo [Lead] Connected`
- **Connection Establishment**: Lead queue handles database connection, child queues inherit connection string
- **Heartbeat Monitoring**: Lead queue manages periodic connection health checks for entire database
- **Child Queue Lifecycle**: Lead queue can spawn/destroy child queues dynamically based on configuration

**DQM Operational Specifications:**

- **Logging Label Format**: `DQM-<Database>-<QueueNumber>-<TagLetters>`
  - Queue number: Two-digit zero-padded integer (00, 01, 02, etc.)
  - Tag letters: One letter per assigned tag (L=Lead, S=Slow, M=Medium, F=Fast, C=Cache)
  - Examples: `DQM-Acuranzo-00-LSMFC` (Lead with all tags), `DQM-Acuranzo-01-F` (Fast queue)

- **Tag Management Logic**:
  - Lead DQM always retains 'L' tag (there can be only one Lead)
  - Initially, Lead gets all tags: LSMFC
  - When spawning child DQMs, Lead loses tags to children but keeps 'L'
  - Child DQMs get specific tags (e.g., Fast queue gets 'F')
  - If only one DQM exists, it handles all tags

- **Timer Loop Implementation**:
  - **Heartbeat Interval**: Default 30 seconds, configurable
  - **Immediate Check**: Connection validation starts immediately on DQM launch
  - **Startup Sequence**: DQM logs startup, then immediately attempts database connection
  - **Connection Retry**: If initial connection fails, retries every 30 seconds
  - **Heartbeat Messages**: Periodic logging of connection health status
  - **Event-Driven**: Thread can be woken by various events:
    - Other DQMs signaling queue full/empty
    - Database trigger notifications
    - Query completion notifications
  - **Health Check**: Tests database connectivity, attempts reconnection if failed

- **Dynamic Scaling Conditions**:
  - **Configuration**: Min/max counts specified per tag type
  - **Initial Launch**: Lead launches DQMs to meet minimum requirements
  - **Scale Up**: When all queues for a tag are non-empty and max not reached
  - **Scale Down**: When all queues for a tag become empty, reduce to minimum
  - **Notifications**: Lead DQM notified on query assignment/completion for scaling decisions

**DQM Architecture & Tag Flow Diagram:**

```mermaid
graph TD
    A[Database Launch] --> B[Lead DQM Created]
    B --> C[Lead DQM: 00-LSMFC]
    C --> D{Config Check}
    D --> E[Launch Min DQMs]
    E --> F[Fast DQM: 01-F]
    E --> G[Slow DQM: 02-S]
    F --> H[Lead Loses F Tag]
    G --> I[Lead Loses S Tag]
    H --> J[Lead Now: 00-LMC]
    I --> J
    J --> K[Heartbeat Timer<br/>30s interval]
    K --> L{DB Connected?}
    L -->|Yes| M[Monitor Queues]
    L -->|No| N[Attempt Reconnect]
    N --> O[Log Connection Issue]
    M --> P{All Fast Queues<br/>Non-Empty?}
    P -->|Yes| Q[Spawn Additional<br/>Fast DQM: 03-F]
    P -->|No| R{All Fast Queues<br/>Empty?}
    R -->|Yes| S[Terminate Fast DQMs<br/>to Minimum]
    R -->|No| T[Continue Monitoring]

    style B fill:#e1f5fe
    style C fill:#b3e5fc
    style J fill:#81d4fa
    style K fill:#4fc3f7
    style M fill:#29b6f6
```

### Phase 2: Multi-Engine Interface Layer ✅ **COMPLETED 9/5/2025**

**Database Engine Abstraction Interface (Implemented):**

```c
typedef struct DatabaseEngineInterface {
    DatabaseEngine engine_type;
    char* name;                           // Engine identifier ("postgresql", "sqlite", etc.)

    // Core connection management
    bool (*connect)(ConnectionConfig* config, DatabaseHandle** connection);
    bool (*disconnect)(DatabaseHandle* connection);
    bool (*health_check)(DatabaseHandle* connection);
    bool (*reset_connection)(DatabaseHandle* connection);

    // Query execution
    bool (*execute_query)(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
    bool (*execute_prepared)(DatabaseHandle* connection, PreparedStatement* stmt, QueryRequest* request, QueryResult** result);

    // Transaction management
    bool (*begin_transaction)(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
    bool (*commit_transaction)(DatabaseHandle* connection, Transaction* transaction);
    bool (*rollback_transaction)(DatabaseHandle* connection, Transaction* transaction);

    // Prepared statement management
    bool (*prepare_statement)(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt);
    bool (*unprepare_statement)(DatabaseHandle* connection, PreparedStatement* stmt);

    // Engine-specific utilities
    char* (*get_connection_string)(ConnectionConfig* config);
    bool (*validate_connection_string)(const char* connection_string);
    char* (*escape_string)(DatabaseHandle* connection, const char* input);
};

typedef struct DatabaseHandle {
    DatabaseEngine engine_type;
    void* connection_handle;              // Engine-specific handle (PGconn, sqlite3, etc.)
    ConnectionConfig* config;
    DatabaseConnectionStatus status;
    time_t connected_since;
    Transaction* current_transaction;
    PreparedStatement** prepared_statements;  // Array of prepared statements
    size_t prepared_statement_count;
    pthread_mutex_t connection_lock;      // Thread safety for connection operations
    volatile bool in_use;                 // Connection pool management
    time_t last_health_check;
    int consecutive_failures;
};
```

**Key Implementation (Completed):**

- ✅ DatabaseEngineInterface with comprehensive function pointers for all database operations
- ✅ DatabaseHandle for connection management with thread safety and health monitoring
- ✅ Supporting structs: ConnectionConfig, QueryRequest, QueryResult, PreparedStatement, Transaction
- ✅ Engine registry system for managing multiple database engines
- ✅ Unified API functions that work across all engines (database_engine_connect, database_engine_execute, etc.)
- ✅ Compilation tested and integrated into build system
- ✅ Thread-safe engine registration and lookup
- ✅ Connection lifecycle management with proper cleanup

**Engine-Specific Connection Strings:**

- **PostgreSQL**: `postgresql://user:password@host:port/database?sslmode=required`
- **SQLite**: `/path/to/database.db` or `:memory:`
- **MySQL/MariaDB**: `mysql://user:password@host:port/database?charset=utf8mb4`
- **IBM DB2**: Uses iSeries connection format with dead-locked detection

**Engine Implementation Examples:**

```c
typedef struct PostgresConnection {
    PGconn* connection;
    bool in_transaction;
    PreparedStatementCache* statements;
};

bool postgres_execute_query(QueryRequest* req, QueryResult* result) {
    // libpq integration with prepared statements and transaction handling
}
```

```c
typedef struct SQLiteConnection {
    sqlite3* db;
    char* db_path;
    PreparedStatementCache* statements;
};

bool sqlite_execute_query(QueryRequest* req, QueryResult* result) {
    // sqlite3_step integration with WAL mode and thread safety
}
```

```c
typedef struct MySQLConnection {
    MYSQL* connection;
    my_bool reconnect;
    PreparedStatementCache* statements;
};

bool mysql_execute_query(QueryRequest* req, QueryResult* result) {
    // mysql_real_query with auto-reconnect capability
}
```

```c
typedef struct DB2Connection {
    SQLHDBC connection;
    SQLHANDLE environment;
    PreparedStatementCache* statements;
};

bool db2_execute_query(QueryRequest* req, QueryResult* result) {
    // SQLExecDirect with deadlock detection and iSeries integration
}
```

## CURRENT IMPLEMENTATION STATUS (9/8/2025)

**PostgreSQL Engine:**

- Full implementation with libpq integration
- Working database launch through DQM architecture
- Connection management and health monitoring
- Thread-safe operations
- Successfully tested with first database launch

**DQM (Database Queue Manager) Architecture:**

- Lead queue spawns child worker queues dynamically
- Hierarchical queue management (Lead + children)
- Thread-safe queue operations
- Statistics monitoring and health checks
- Integration with launch/landing system

**Implementation Pattern for Each Engine:**

- Add engine-specific connection handling in `database_engine.c`
- Update `database_add_database()` to handle engine-specific connection strings
- Test library loading and basic connectivity
- Integrate with existing DQM architecture

### Phase 3: Query Caching & Bootstrap

**Cache Management:**

```c
typedef struct QueryTemplate {
    int query_id;
    char* sql_template;
    char* database_type;
    time_t last_modified;
};
```

**Bootstrap System:**

- **Tier 1**: Direct SQL bootstrap for self-contained databases
- **Tier 2**: Cross-hosted bootstrap using host database queries
- **Cache Consistency**: Trigger-based invalidation across dependent databases

#### Database Trigger Implementation

**Cross-Engine Trigger Architecture:**

```c
typedef struct DatabaseTrigger {
    char* trigger_name;
    DatabaseEngine* engine;
    TriggerType type;              // TABLE_CHANGE, CUSTOM_EVENT, DEADLOCK_DETECTED
    char* target_table;
    char* notification_channel;    // PostgreSQL: LISTEN channel, others: equivalent
    DatabaseCallback callback;     // C function to invoke on trigger
    void* user_data;              // Passed to callback
    SubscriptionState state;
};

typedef struct TriggerManager {
    DatabaseConnection* connection;
    TriggerSubscription* subscriptions;    // Array of active subscriptions
    pthread_t listener_thread;             // Thread monitoring for trigger events
    CallbackQueue* event_queue;            // Queue for processing trigger events
    bool shutdown;                         // Graceful shutdown flag
};
```

**Engine-Specific Trigger Implementations:**

**PostgreSQL Triggers:** Uses LISTEN/NOTIFY mechanism for real-time event delivery
**SQLite Triggers:** Polls trigger_events table for change detection (no built-in NOTIFY)
**MySQL/MariaDB Triggers:** Polls trigger_events table with JSON data format
**IBM DB2 Triggers:** Polls trigger_events table with deadlock detection and retry logic

**Key Implementation Approach:**

- PostgreSQL: Native notification channels via LISTEN/NOTIFY
- SQLite/MySQL/DB2: Polling-based approach with configurable intervals (1-5 seconds)
- Cross-engine: Unified interface for trigger registration, event processing, and cleanup

**Unified Trigger Management Interface:**

```c
typedef struct TriggerInterface {
    bool (*create_trigger)(DatabaseConnection*, TriggerConfig*, DatabaseTrigger**);
    bool (*drop_trigger)(DatabaseConnection*, const char* trigger_name);
    bool (*start_listener)(TriggerManager*, callback_func_t);
    bool (*stop_listener)(TriggerManager*);

    // Event dispatch system
    void (*dispatch_event)(TriggerManager*, const char* channel, const char* payload);
    bool (*register_callback)(TriggerManager*, const char* event_type, callback_func_t);
};

// Factory function to create engine-specific trigger interface
TriggerInterface* create_trigger_interface(DatabaseEngine* engine) {
    switch (engine->type) {
        case ENGINE_POSTGRESQL: return create_postgresql_trigger_interface();
        case ENGINE_SQLITE:     return create_sqlite_trigger_interface();
        case ENGINE_MYSQL:      return create_mysql_trigger_interface();
        case ENGINE_DB2:        return create_db2_trigger_interface();
        default: return NULL;
    }
}
```

**Trigger Event Processing:**

```c
typedef struct TriggerEvent {
    char* table_name;
    char* operation;     // INSERT, UPDATE, DELETE
    char* data;         // JSON payload of changed data
    time_t timestamp;
    uint64_t sequence_id; // For ordering guarantees
};

void trigger_dispatch_event(TriggerManager* mgr, TriggerEvent* event) {
    // Add to event queue for processing by callback thread
    event_queue_push(mgr->event_queue, event);

    // Invoke registered callbacks
    List* callbacks = get_callbacks_for_table(mgr, event->table_name);
    for (ListItem* item = callbacks->first; item; item = item->next) {
        DatabaseCallback cb = (DatabaseCallback)item->data;
        cb(event, mgr->user_data);
    }

    // Handle cache invalidation for affected queries
    invalidate_cache_for_table(event->table_name, event->operation);
}

// Example cache invalidation callback
void cache_invalidation_callback(TriggerEvent* event, void* user_data) {
    QueryCache* cache = (QueryCache*)user_data;

    // Invalidate cached results for this table
    query_cache_invalidate_table(cache, event->table_name);

    log_this("DATABASE", "Cache invalidated due to trigger event",
             LOG_LEVEL_DEBUG, true, true, true);
}
```

**Trigger Lifecycle Management:**

```c
bool register_database_trigger(DatabaseConnection* conn, DatabaseTrigger* trigger) {
    TriggerInterface* iface = create_trigger_interface(conn->engine);

    if (!iface) return false;

    // Create trigger in database
    if (!iface->create_trigger(conn, &trigger->config, &trigger)) {
        return false;
    }

    // Start listener thread if needed
    if (trigger->needs_listener) {
        return iface->start_listener(trigger->manager, trigger_callback_router);
    }

    return true;
}

void unregister_database_trigger(DatabaseTrigger* trigger) {
    TriggerInterface* iface = create_trigger_interface(trigger->connection->engine);

    // Stop listener
    if (trigger->needs_listener) {
        iface->stop_listener(trigger->manager);
    }

    // Drop trigger from database
    iface->drop_trigger(trigger->connection, trigger->name);

    // Cleanup resources
    free(trigger);
}
```

### Phase 4: Acuranzo Integration & Testing

- Query template system with payload storage
- Parameter injection for SQL generation
- End-to-end integration with PostgreSQL backend

### Phase 5: Multi-Engine Expansion

- Engines: SQLite, MySQL/MariaDB, IBM DB2, AI endpoints

### Phase 6: Production Hardening

- Security hardening
- Performance monitoring

## LAUNCH & LANDING INTEGRATION

**Launch Sequence:**

1. Configuration validation
2. Database engine initialization
3. Queue manager setup
4. Query definition loading
5. Worker thread startup
6. Trigger registration
7. Admin thread activation

**Landing Sequence:**

1. Stop accepting new queries
2. Wait for active query completion
3. Shutdown admin thread
4. Terminate worker threads
5. Unregister database triggers
6. Close all connections
7. Cleanup in-memory resources

**Queue Configuration Logic:**

- **Default Behavior**: If no queue configuration specified, create one of each: Slow, Medium, Fast, Cache
- **Zero Assignment**: If a queue type is configured with 0 workers, assign that tag to the Lead queue
- **Lead Queue**: Always created automatically with "Lead" tag, cannot be disabled
- **Tag Inheritance**: Lead queue inherits tags from queues set to 0 workers
- **Thread Launch**: Each queue launches in its own thread using same database credentials

## TESTING STRATEGY

### Blackbox Integration (Test 27.1-27.4) ✅ **OPERATIONAL 9/10/2025**

**Test 27.1: Engine Connectivity** *(25-30% coverage)*

- Basic connectivity validation for all engines
- Error scenario handling

**Test 27.2: Queue Operations** *(40-50% combined coverage)*

- Multi-queue distribution
- Thread-safe submission testing

**Test 27.3: Full Processing Pipeline** *(60-70% combined coverage)*

- Query ID caching end-to-end
- API-to-result workflow

**Test 27.4: Performance & Stress** *(75-85% final coverage)*

- Load testing
- Memory leak verification with Valgrind

### Unit Testing (Test 10 Unity Framework)

**Coverage Strategy:**

- **Production Code**: Pure business logic (no testing infrastructure)
- **Unit Tests**: External files calling production functions
- **Combined Coverage**: Files <100 lines: >50%, Files >100 lines: >75%

**Coverage Matrix:**

| Component | Blackbox (Test 27) | Unity (Test 10) | Combined Target |
|-----------|-------------------|----------------|-------------|
| Engine Connection | ~85% | ~10% | 95% |
| Queue Management | ~70% | ~25% | 95% |
| Query Processing | ~65% | ~30% | 95% |
| Cache Operations | ~60% | ~35% | 95% |
| Bootstrap Logic | ~90% | ~5% | 95% |
| **Overall** | **~62%** | **~18%** | **~80%** |

## ⚠️ RISK ANALYSIS & MITIGATION

**High-Risk Areas:**

1. **Query Template Caching**: Strict parameter enforcement, validation on reload
2. **Multi-Threaded Queue Synchronization**: Follow existing patterns, atomic operations
3. **Database Engine Abstraction**: Start with PostgreSQL, interface contracts upfront

**Medium-Risk Areas:**

1. **Memory Management**: Consistent patterns, existing leak tests
2. **Schema Evolution**: Semi-manual template updates, version metadata
3. **Performance Scaling**: Configurable limits, graceful degradation

## 🚀 IMPLEMENTATION ROADMAP

### Phase 1: DQM Infrastructure Implementation ✅ **COMPLETED 9/8/2025**

- Database Queue Manager (DQM) architecture
- Lead queue with dynamic child spawning
- Hierarchical queue management
- Thread-safe operations and synchronization
- Integration with existing queue system

### Phase 2: Multi-Engine Interface Layer Implementation ✅ **COMPLETED 9/5/2025**

- DatabaseEngineInterface with function pointers
- DatabaseHandle for connection management
- Supporting structs and unified API
- Engine registry system
- Thread-safe operations and cleanup

### Phase 2.1.0: PostgreSQL Engine ✅ **COMPLETED & WORKING 9/8/2025**

- libpq integration with dynamic loading
- Connection management and health monitoring
- Prepared statements and transaction support
- JSON result serialization
- Full DQM integration and working database launch

### Phase 2.2: SQLite Engine Implementation (1-2 weeks)

- SQLite3 integration with dynamic loading
- File-based database support
- WAL mode configuration
- Connection pooling for concurrent access
- DQM integration and testing

### Phase 2.3: MySQL Engine Implementation (1-2 weeks)

- MySQL/MariaDB client integration
- Auto-reconnect capability
- Connection pooling
- Prepared statement support
- DQM integration and testing

### Phase 2.4: DB2 Engine Implementation (2-3 weeks)

- IBM DB2 client integration
- Deadlock detection and handling
- iSeries connection support
- Advanced transaction management
- DQM integration and testing

### Phase 3: Query Caching (1-2 weeks)

- Template cache loading
- Parameter injection engine

### Phase 4: Acuranzo Integration (2 weeks)

- Query template creation
- End-to-end integration

### Phase 5: Multi-Engine Expansion (3-4 weeks)

- SQLite, MySQL/MariaDB, IBM DB2 engines
- Engine-specific optimizations

### Phase 6: Production Hardening (2 weeks)

- Security hardening
- Performance monitoring

## ✅ ARCHITECTURAL DECISIONS RESOLVED

### Core Decisions

**Queue Strategy**: Multiple queue paths (slow/medium/fast/cache) with routing logic

**Connection Strategy**: Persistent per-queue connections, scaling through server instances

**Schema Evolution**: Client responsibility, queries assume compatibility

**Third-Party Schemas**: Explicitly supported, cross-schema queries allowed

### Integration Decisions

**Helium Relationship**: Standard database access via mechanisms

**API Surface**: Main endpoint `/api/query/:database/:query_id/:queue_path/:params`

**Query Optimization**: Basic caching with configurable expiration

**Resource Limits**: Queue depth limits trigger scaling events

## TESTING & VALIDATION APPROACH

### Query Processing Tests

- Template parameter injection
- Query ID transformation
- JSON result formatting
- Queue performance

### Integration Tests

- REST API functionality
- Engine compatibility
- Schema support

### Performance Tests

- Connection efficiency
- Throughput benchmarking
- Memory monitoring

## 🧪 SINGLE COMPREHENSIVE DATABASE TEST ✅ **OPERATIONAL 9/10/2025**

**test_27_databases_parallel_engine_tests.sh** - All Engines Parallel Operational Test

**Flow:**

1. Verify engine libraries available
2. Identify test datasources (local files, containers, remote)
3. Connect to all 5 engines simultaneously
4. Execute complex fixed queries
5. Utilize queuing system
6. Analyze database state before/after
7. Free resources and validate responses
8. Comprehensive cleanup

**Success Criteria:**

- All 5 engines establish connections
- Same query executes on all engines
- Queue processing works correctly
- Clean resource cleanup
- Consistent JSON formatting

### Test Infrastructure Requirements

- Docker containers for databases
- SQLite local file management
- AI mock endpoints
- Memory monitoring tools

## 🚀 LAUNCH AND LANDING INTEGRATION

### Current State Analysis

**Existing Files:**

- `src/launch/launch_database.c` - Config validation stub
- `src/landing/landing_database.c` - Cleanup stub
- `src/config/config_defaults.c` - Basic defaults

**Limitations:**

- Launch: Only config validation
- Landing: Only config cleanup
- Treat database as configuration handler

### Bootstrap Configuration Architecture

#### Tier 1: Direct Bootstrap

- Use case: Self-contained databases
- Direct SQL execution from environment variables

#### Tier 2: Parameterized Bootstrap

- Use case: Cross-hosted databases, legacy integration
- Host database query with parameters

**Query Cache Consistency:**

- Trigger-based invalidation across databases
- Lead queue manages admin tasks

### Enhanced Database Defaults Structure

**JSON Configuration:**

```json
{
  "Databases": {
    "DefaultWorkers": 2,
    "ConnectionCount": 5,
    "MaxConnectionsPerPool": 16,
    "DefaultQueues": {
      "Slow": 1,
      "Medium": 2,
      "Fast": 4,
      "Cache": 1
    },
    "Connections": [
      {
        "Enabled": true,
        "Name": "Acuranzo",
        "Type": "${env.ACURANZO_TYPE}",
        "Workers": 2,
        "Queues": {
          "Slow": 2,
          "Medium": 0,
          "Fast": 4,
          "Cache": 1
        },
        "ConnectionPooling": {"MaxConnections":16, "MinConnections":2},
        "QueryTables": {"PrimaryTable":"acuranzo_queries"}
      }
    ]
  }
}
```

**Environment Variables Template:**

```bash
# Primary Database
ACURANZO_TYPE=postgresql
ACURANZO_HOST=localhost
ACURANZO_USER=hydrogen_user

# Secondary Databases
CANVAS_TYPE=mysql
CANVAS_HOST=example.com
HELIUM_TYPE=sqlite
```

## 📈 IMPLEMENTATION PRIORITIES

### Phase 6A: Launch/Landing Enhancement

1. Validate current stubs
2. Enhance launch_database.c
3. Enhance landing_database.c
4. Update config_defaults.c
5. Enhance launch readiness

### Phase 6B: Configuration Framework

1. Design JSON structure
2. Implement config defaults
3. Environment variable validation
4. Configuration validation

## 🧪 UNITY FRAMEWORK UNIT TESTING STRATEGY

**Coverage Goals:**

- Blackbox (Test 27): Majority coverage via integration testing
- Unity (Test 10): Fill gaps for error paths and internal logic
- Target: 50-60% for files <100 lines, 75-85% for larger files

**Unity Test Structure:**

- External to production codebase
- External directory: `tests/unity/database/`
- Files: `database_connection_unittest.c`, `database_queue_unittest.c`, etc.

**Test Pattern:**

```c
#include "../../src/database/database_connection.h"

TEST_GROUP(DatabaseConnectionTests);
TEST(DatabaseConnectionTests, connect_with_valid_credentials);
TEST(DatabaseConnectionTests, handle_connection_timeout);
```

**Testing Timeline:**

1. Start blackbox tests (Test 27.1-27.4)
2. Fill coverage gaps with Unity tests
3. Maintain combined coverage thresholds

## 🏗️ ARCHITECTURAL CONTEXT & TROUBLESHOOTING GUIDANCE

### Key Architectural Principles

**Queue-Based Architecture Rationale:**

- **Multi-Queue Strategy**: Each database supports slow/medium/fast/cache queues to handle different query priorities and performance requirements
- **Persistent Connections**: Each queue maintains dedicated connections rather than pooling to ensure predictable performance
- **Thread Safety**: All queue operations use consistent locking patterns from existing WebSocket implementation

**Engine Abstraction Framework:**

- **PostgreSQL Foundation**: Chosen as first engine due to widest feature set and SQL standard compliance
- **Interface Contracts**: All engines implement the same result format and error handling for seamless switching
- **SQL Dialect Handling**: Query templates include engine-specific variants handled by load-time injection

**Bootstrap System Design:**

- **Bootstrapping Chicken & Egg**: Need database to load queries, but need queries to access database
- **Tier System**: Tier 1 (direct SQL from env vars), Tier 2 (cross-hosted via host database)
- **Trigger-Based Consistency**: Database triggers propagate changes across dependent databases automatically

**API Surface Simplification:**

- **Query ID System**: Client-side SQL templates referenced by IDs eliminate schema-dependent payload changes
- **Queue Path Selection**: Client specifies performance preference (slow/fast) that maps to physical queues
- **JSON Result Standardization**: All database engines convert results to consistent JSON format

### Implementation Guidance

**Key Patterns to Follow:**

- Synchronization: Use existing lock hierarchies from WebSocket subsystem
- Connection Strategy: Persistent per-queue connections for predictable performance  
- Bootstrap: Always maintain Tier 1 (direct SQL) databases to avoid dependency cycles
- Testing: External Unity tests only, no production code hooks
- Schema Changes: Treat as deployment events with gradual rollout

### Performance Considerations

**Queue Depth Management:**

- **Trigger Scaling**: Queue overflow should trigger automatic scaling (additional server instances)
- **Resource Limits**: Configurable depth limits prevent resource exhaustion
- **Load Balancing**: Client-side distribution to avoid queue hotspots

**Cache Effectiveness:**

- **Expiration Design**: Basic time-based expiration (minutes) configurable per query
- **Trigger Integration**: Database triggers invalidate cache entries automatically
- **Cross-Database Consistency**: Leader queue coordinates cache invalidation across database instances

**Connection Optimization:**

- **Persistent Strategy**: No connection pooling overhead, guaranteed response times
- **Health Monitoring**: Leader thread performs periodic health checks and reconnections
- **Resource Cleanup**: Comprehensive shutdown sequences prevent connection leaks

**Measurement Points:**

- **Throughput**: Queries per second across different queue paths
- **Latency**: End-to-end time from API call to result return
- **Resource Usage**: Memory, CPU, and network consumption patterns

### Integration Patterns

**Helium Relationship:**

- **Non-Special Treatment**: Just another database through standard mechanisms
- **Shared Query System**: Uses same query indexing as PostgreSQL, MySQL, etc.
- **Deployment Flexibility**: Can be SQLite file or full standalone database depending on requirements

**Multitenant Considerations:**

- **Host Database Pattern**: One master database (usually a fast PostgreSQL) hosts queries for multiple tenants
- **Trigger Propagation**: Changes in host database trigger cache invalidation in dependent databases
- **Bootstrap Coordination**: Tier 2 bootstrap ensures consistent startup across compressed systems

**Deployment Flexibility:**

- **Environment-Driven**: All configuration through environment variables for 12-factor compliance
- **Docker Ready**: Test infrastructure uses containers for consistent environments
- **Scaling Strategy**: Horizontal scaling through additional server instances, not vertical within servers

### Debugging Checklists

**Launch Sequence Issues:**

- ✓ Environment variables loaded correctly
- ✓ Database connectivity validated
- ✓ Query table structure exists and populated
- ✓ Trigger creation permissions granted
- ✓ Worker thread startup without deadlocks
- ✓ Admin thread initialization successful

**Query Execution Problems:**

- ✓ Query ID resolves to template
- ✓ Parameter injection successful
- ✓ Connection available in worker thread
- ✓ Result serialization handles all datatypes
- ✓ Queue routing matches client expectations

**Cache Consistency Failures:**

- ✓ Trigger registration captured changes
- ✓ Cross-database notification sent
- ✓ Cache invalidation propagated
- ✓ Bootstrap query completed successfully
- ✓ Template reload didn't break parsing

**Performance Degradation:**

- ✓ Queue depth monitoring in place
- ✓ Cross-thread synchronization not blocking
- ✓ Connection pool not exhausted
- ✓ Cache effectiveness above 80%
- ✓ Memory usage within bounds

## 🚀 LESSONS LEARNED & BEST PRACTICES

### Dynamic Library Loading Strategy ✅ **CRITICAL INSIGHT**

Key Decision: Use dlopen/dlsym for ALL database libraries

- **✅ DO NOT** modify CMakeLists.txt to link database libraries statically
- **✅ DO** load all database client libraries dynamically at runtime using `dlopen()`
- **✅ DO** implement proper error handling when libraries are not available
- **✅ DO** allow the system to function even when specific database libraries are missing

**Implementation Pattern:**

```c
// Load library dynamically
void* lib_handle = dlopen("libpq.so.5", RTLD_LAZY);
if (!lib_handle) {
    // Graceful degradation - log warning but continue
    log_this(SR_DATABASE, "PostgreSQL library not available", LOG_LEVEL_WARN);
    return false;
}

// Load function pointers
PQconnectdb_ptr = (PQconnectdb_t)dlsym(lib_handle, "PQconnectdb");
// ... load all required functions

// Use function pointers instead of direct calls
void* conn = PQconnectdb_ptr(conninfo);
```

**Benefits:**

- System remains functional even without specific database installations
- No static linking dependencies in build system
- Runtime flexibility for different deployment environments
- Proper error handling and graceful degradation
