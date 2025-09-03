# DATABASE SUBSYSTEM IMPLEMENTATION PLAN

## ESSENTIAL PREREQUISITES

**Required Reading:**

- [RECIPE.md](../../RECIPE.md) - C development standards and build processes
- [tests/README.md](../../tests/README.md) - Testing framework overview
- [docs/reference/database_architecture.md](../reference/database_architecture.md) - Architecture specs

**Key Dependencies:**

- [src/queue/queue.c](../../src/queue/queue.c) - Queue infrastructure to extend
- [src/config/config_databases.c](../../src/config/config_databases.c) - Existing config system
- [CMAKE/README.md](../../cmake/README.md) - Build system for database engines

## ARCHITECTURE OVERVIEW

Hydrogen serves as a database gateway supporting PostgreSQL, SQLite, MySQL, DB2 with AI-ready query architecture. Key patterns:

- **Queue-Based Processing**: slow/medium/fast queues per database with priority routing
- **Query ID System**: REST API passes query numbers + parameters for schema independence
- **Cross-Database Hosting**: One database can serve queries for multiple downstream databases
- **Lead Queue Pattern**: Dedicated admin queue for cache management and triggers

## EXISTING SUBSYSTEM STATE

```c
// Current stubs (need enhancement)
src/launch/launch_database.c        // Basic connectivity validation
src/landing/landing_database.c      // Config cleanup only
src/config/config_defaults.c        // Basic database defaults
```

## IMPLEMENTATION PHASES

### Phase 1: Queue Infrastructure

**Core Components:**

```c
typedef struct DatabaseQueue {
    char* name;
    Queue* queue;
    pthread_mutex_t lock;
    DatabaseConnection* connection;
};
```

**Key Implementation:**

- Multi-queue system per database (slow/medium/fast/cache)
- Thread-safe query submission and round-robin distribution
- Worker threads with persistent connections

### Phase 2: Multi-Engine Interface Layer

**Database Engine Abstraction Interface:**

```c
typedef struct DatabaseEngine {
    char* name;                           // Engine identifier ("postgresql", "sqlite", etc.)
    bool (*connect)(ConnectionConfig*, DatabaseConnection**);
    bool (*disconnect)(DatabaseConnection*);
    bool (*execute_query)(DatabaseConnection*, QueryRequest*, QueryResult**);
    bool (*execute_prepared)(DatabaseConnection*, PreparedStatement*, QueryResult**);
    bool (*begin_transaction)(DatabaseConnection*, IsolationLevel);
    bool (*commit_transaction)(DatabaseConnection*, Transaction*);
    bool (*rollback_transaction)(DatabaseConnection*, Transaction*);
    bool (*health_check)(DatabaseConnection*);     // Connection validation
    bool (*reset_connection)(DatabaseConnection*); // Recovery after errors
};

typedef struct DatabaseConnection {
    DatabaseEngine* engine;
    void* connection_handle;              // Engine-specific handle (PGconn, sqlite3, etc.)
    ConnectionState state;
    time_t connected_since;
    Transaction* current_transaction;
    QueryCache* prepared_statements;      // Prepared statement cache
    SubscriptionManager* triggers;        // Active triggers/subscriptions
};
```

**Engine-Specific Connection Strings:**

- **PostgreSQL**: `postgresql://user:password@host:port/database?sslmode=required`
- **SQLite**: `/path/to/database.db` or `:memory:`
- **MySQL/MariaDB**: `mysql://user:password@host:port/database?charset=utf8mb4`
- **IBM DB2**: Uses iSeries connection format with dead-locked detection

**Engine Implementation Examples:**

**PostgreSQL Engine:**

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

**SQLite Engine:**

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

**MySQL/MariaDB Engine:**

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

**IBM DB2 Engine:**

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

**PostgreSQL Triggers:**

```sql
-- Database-level trigger setup
CREATE OR REPLACE FUNCTION notify_table_change()
RETURNS TRIGGER AS $$
BEGIN
  PERFORM pg_notify('table_updates',
    json_build_object(
      'table', TG_TABLE_NAME,
      'operation', TG_OP,
      'modified_at', NOW()
    )::text
  );
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Register trigger
CREATE TRIGGER {trigger_name}_notification
  AFTER INSERT OR UPDATE OR DELETE ON {table_name}
  FOR EACH ROW EXECUTE FUNCTION notify_table_change();

-- Listen in C code
PQexec(connection, "LISTEN table_updates");
```

```c
// PostgreSQL listener implementation
void* postgres_trigger_listener(void* arg) {
    TriggerManager* mgr = (TriggerManager*)arg;
    PGconn* conn = (PGconn*)mgr->connection->connection_handle;

    PGnotify* notify;
    while (!mgr->shutdown) {
        PQconsumeInput(conn);
        while ((notify = PQnotifies(conn)) != NULL) {
            // Parse notification payload and dispatch callback
            trigger_dispatch_event(mgr, notify->relname, notify->payload);
            PQfreemem(notify);
        }
        // Poll with timeout
        struct pollfd pfd = { .fd = PQsocket(conn), .events = POLLIN };
        poll(&pfd, 1, 1000);  // 1 second timeout
    }
    return NULL;
}
```

**SQLite Triggers:**

```sql
-- SQLite trigger for table changes
CREATE TRIGGER {trigger_name}_notification
  AFTER INSERT OR UPDATE OR DELETE ON {table_name}
BEGIN
  INSERT INTO trigger_events (table_name, operation, timestamp)
  VALUES ('{table_name}', CASE
    WHEN NEW IS NULL THEN 'DELETE'
    WHEN OLD IS NULL THEN 'INSERT'
    ELSE 'UPDATE'
  END, strftime('%Y-%m-%d %H:%M:%f', 'now'));
END;
```

```c
// SQLite trigger polling approach (no built-in NOTIFY)
void* sqlite_trigger_poller(void* arg) {
    TriggerManager* mgr = (TriggerManager*)arg;
    sqlite3* db = (sqlite3*)mgr->connection->connection_handle;
    char* last_timestamp = get_last_processed_timestamp(mgr);

    while (!mgr->shutdown) {
        // Poll trigger_events table for new entries
        char* sql = "SELECT * FROM trigger_events WHERE timestamp > ? ORDER BY timestamp";
        sqlite3_stmt* stmt = prepare_statement(db, sql);

        // Process new entries and dispatch callbacks
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            trigger_dispatch_event(mgr,
                sqlite3_column_text(stmt, 0),  // table_name
                sqlite3_column_text(stmt, 1),  // operation
                sqlite3_column_text(stmt, 2)); // timestamp
        }
        sqlite3_finalize(stmt);
        sleep(1);  // Poll interval
    }
    return NULL;
}
```

**MySQL/MariaDB Triggers:**

```sql
-- MySQL trigger setup
DELIMITER //
CREATE TRIGGER {trigger_name}_notification
  AFTER INSERT ON {table_name}
  FOR EACH ROW
BEGIN
  INSERT INTO trigger_events (table_name, operation, data, timestamp)
  VALUES ('{table_name}', 'INSERT', JSON_OBJECT('id', NEW.id), NOW());
END//

CREATE TRIGGER {trigger_name}_notification_update
  AFTER UPDATE ON {table_name}
  FOR EACH ROW
BEGIN
  INSERT INTO trigger_events (table_name, operation, data, timestamp)
  VALUES ('{table_name}', 'UPDATE', JSON_OBJECT('id', NEW.id), NOW());
END//

DELIMITER ;
```

```c
// MySQL trigger polling implementation
void* mysql_trigger_poller(void* arg) {
    TriggerManager* mgr = (TriggerManager*)arg;
    MYSQL* conn = (MYSQL*)mgr->connection->connection_handle;

    while (!mgr->shutdown) {
        // Poll trigger_events table for new entries
        char* query = "SELECT table_name, operation, data, timestamp "
                      "FROM trigger_events WHERE processed = FALSE "
                      "ORDER BY timestamp LIMIT 100";

        if (mysql_query(conn, query) == 0) {
            MYSQL_RES* result = mysql_store_result(conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                trigger_dispatch_event(mgr,
                    row[0], // table_name
                    row[1], // operation
                    row[2]); // data
                // Mark as processed
                mark_trigger_event_processed(conn, row[3]); // timestamp
            }
            mysql_free_result(result);
        }
        sleep(1);  // Poll interval
    }
    return NULL;
}
```

**IBM DB2 Triggers:**

```sql
-- DB2 trigger setup
CREATE TRIGGER {trigger_name}_INSERT_NOTIFICATION
  AFTER INSERT ON {table_name}
  REFERENCING NEW AS N
  FOR EACH ROW
  INSERT INTO trigger_events (table_name, operation, id, timestamp)
  VALUES ('{table_name}', 'INSERT', N.ID, CURRENT_TIMESTAMP);

CREATE TRIGGER {trigger_name}_UPDATE_NOTIFICATION
  AFTER UPDATE ON {table_name}
  REFERENCING NEW AS N
  FOR EACH ROW
  INSERT INTO trigger_events (table_name, operation, id, timestamp)
  VALUES ('{table_name}', 'UPDATE', N.ID, CURRENT_TIMESTAMP);

CREATE TRIGGER {trigger_name}_DELETE_NOTIFICATION
  AFTER DELETE ON {table_name}
  REFERENCING OLD AS O
  FOR EACH ROW
  INSERT INTO trigger_events (table_name, operation, id, timestamp)
  VALUES ('{table_name}', 'DELETE', O.ID, CURRENT_TIMESTAMP);
```

```c
// DB2 trigger polling with deadlock detection
void* db2_trigger_poller(void* arg) {
    TriggerManager* mgr = (TriggerManager*)arg;
    SQLHDBC hdbc = (SQLHDBC)mgr->connection->connection_handle;

    while (!mgr->shutdown) {
        SQLCHAR query[] = "SELECT table_name, operation, id, timestamp "
                         "FROM trigger_events WHERE processed = 0 "
                         "ORDER BY timestamp FETCH FIRST 100 ROWS ONLY";

        SQLHSTMT hstmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
        if (ret == SQL_SUCCESS) {
            // Execute query with deadlock detection
            ret = SQLExecDirect(hstmt, query, SQL_NTS);

            if (ret == SQL_ERROR) {
                SQLCHAR sqlstate[6];
                SQLINTEGER native_error;
                SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sqlstate, &native_error,
                              NULL, 0, NULL);
                // Handle deadlock detection and retry
                if (strcmp((char*)sqlstate, "40001") == 0) {
                    handle_db2_deadlock(mgr);
                }
            } else {
                // Process results
                SQLBindCol(hstmt, 1, SQL_C_CHAR, table_name, sizeof(table_name), NULL);
                while (SQLFetch(hstmt) == SQL_SUCCESS) {
                    trigger_dispatch_event(mgr, table_name, operation, id);
                    mark_trigger_processed(hdbc, timestamp);
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
        sleep(1);  // Poll interval
    }
    return NULL;
}
```

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

**Performance Considerations:**

- **Polling Intervals**: Configurable polling frequency (1-5 seconds typically)
- **Event Batching**: Group multiple events for efficient processing
- **Connection Reuse**: Maintain dedicated connection for trigger polling
- **Deadlock Handling**: Automatic retry with exponential backoff for DB2
- **Memory Management**: Event queue size limits and cleanup policies

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

**Enhanced Launch Requirements:**

```c
int launch_database_subsystem(void) {
    // Phase 1-7: Validation through admin thread startup
    // Return 1 on success, 0 on failure with logging
}
```

## TESTING STRATEGY

### Blackbox Integration (Test 27.1-27.4)

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

### Phase 1: Queue Infrastructure (2-3 weeks)

- Queue manager implementation
- Multi-queue system
- Thread-safe submission
- Memory management validation

### Phase 2: PostgreSQL Engine (2 weeks)

- libpq integration
- Connection management
- Prepared statements
- JSON result serialization

### Phase 3: Query Caching (1-2 weeks)

- Template cache loading
- Parameter injection engine

### Phase 4: Acuranzo Integration (2 weeks)

- Query template creation
- End-to-end integration

### Phase 5: Multi-Engine Expansion (3-4 weeks)

- Remaining engines
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

## 🧪 SINGLE COMPREHENSIVE DATABASE TEST

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
    "Connections": [
      {
        "Enabled": true,
        "Name": "Acuranzo",
        "Type": "${env.ACURANZO_TYPE}",
        "Workers": {"Slow":1, "Medium":2, "Fast":4, "Cache":1},
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

### Common Implementation Traps & Solutions

**Synchronization Issues:**

- **Trap**: Race conditions between queue threads and admin tasks
- **Solution**: Use consistent lock hierarchy from existing patterns; atomic operations for counters
- **Guidance**: Follow WebSocket subsystem's thread-safe patterns exactly

**Connection Pooling Confusion:**

- **Trap**: Attempting to add dynamic pooling when design specifies persistent connections
- **Solution**: Stick to the resolved design - persistent connections per queue for predictability
- **Guidance**: Scaling handled by multiple server instances, not within-queue scaling

**Bootstrap Dependency Cycles:**

- **Trap**: Infinite loops when databases depend on each other for bootstrap
- **Solution**: Always have at least one direct SQL bootstrap database (Tier 1)
- **Guidance**: Use env vars for initial bootstrap, transition to full cross-hosted after first database operational

**Engine Feature Parity Assumptions:**

- **Trap**: Assuming all engines support same SQL features (procedures, triggers, etc.)
- **Solution**: Interface contracts define what features are required/guaranteed vs. optional
- **Guidance**: Start with PostgreSQL ecosystem, handle feature gaps gracefully

**Testing Pattern Misalignment:**

- **Trap**: Unity tests with internal hook patterns vs. external-only strategy
- **Solution**: Keep all unit tests external, no production code changes for testing
- **Guidance**: Match coverage strategy - Blackbox (Test 27) provides broad coverage, Unity fills specific gaps

**Schema Evolution Over-Synchronization:**

- **Trap**: Manual schema changes causing extended downtime during template reload
- **Solution**: Semi-manual update process with version-aware templates and gradual rollout
- **Guidance**: Treat schema changes as deployment events, not runtime operations

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
