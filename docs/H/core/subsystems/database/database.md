# Database Subsystem Documentation

## Overview

Hydrogen's database subsystem provides a unified interface for multiple database engines through a sophisticated **Database Queue Manager (DQM)** architecture. This system enables seamless integration with PostgreSQL, SQLite, MySQL, and DB2 databases while maintaining high performance and reliability.

## Architecture

The database subsystem is built around the **Lead DQM** pattern:

```hierarchy
Database Instance
└── Lead DQM (Queue 00)
    ├── Slow Queue (Queue 01+)
    ├── Medium Queue (Queue 02+)
    ├── Fast Queue (Queue 03+)
    └── Cache Queue (Queue 04+)
```

### Core Components

- **[Lead DQM](/docs/H/core/reference/lead_dqm.md)** - Central coordinator for each database instance
- **Child Queue Managers** - Specialized queues for different priority levels
- **Multi-Engine Interface** - Unified API across all supported database engines
- **Dynamic Queue Scaling** - Automatic spawning/shutdown based on workload

## Implementation Status

### ✅ **Completed & Operational**

**Phase 1: DQM Infrastructure** *(Completed 9/8/2025)*

- Lead DQM architecture with dynamic child queue spawning
- Hierarchical queue management with proper tag inheritance
- Thread-safe operations with comprehensive synchronization
- Dynamic library loading (no static linking dependencies)
- Integration with existing Hydrogen queue infrastructure

**Phase 2: Multi-Engine Interface** *(Completed 9/5/2025)*

- DatabaseEngineInterface with comprehensive function pointers
- DatabaseHandle for connection management and health monitoring
- Supporting data structures (ConnectionConfig, QueryRequest, QueryResult, etc.)
- Engine registry system for managing multiple database types

**All Database Engines** *(Completed & Operational 10/2/2025)*

- **PostgreSQL Engine** - Full libpq integration with dynamic loading
- **SQLite Engine** - File-based databases with WAL mode configuration
- **MySQL Engine** - Auto-reconnect capability and connection pooling
- **DB2 Engine** - iSeries support with deadlock detection and retry logic

**All engines feature:**

- Lead DQM launch and bootstrap query execution
- Dynamic library loading with graceful degradation
- Unified interface contracts for seamless engine switching
- Connection management with health monitoring and automatic recovery
- Prepared statements and transaction support
- JSON result serialization and comprehensive error handling
- Thread-safe operations with proper synchronization

**Test Infrastructure** *(Operational 9/10/2025)*

- test_30_database.sh - All Engines Parallel Operational Test
- Database connectivity validation and queue operations testing
- Performance & stress testing with memory leak verification
- Multi-engine compatibility validation

### 🔄 **Remaining Work**

#### Phase 3: Query Caching (Next Priority)

- Template cache loading from database
- Parameter injection engine for dynamic SQL generation
- Cache invalidation mechanisms

#### Phase 4: API Integration

- Complete `/api/query/*` endpoint implementation
- Query template management and parameter injection
- End-to-end integration testing

#### Phase 3: Query Caching

- Template cache loading from database
- Parameter injection engine for dynamic SQL generation

## Key Features

### Lead DQM Architecture

Each database gets exactly one **Lead DQM** that:

- Manages the database connection lifecycle
- Spawns child queues dynamically based on workload
- Coordinates query routing and load balancing
- Implements comprehensive health monitoring and recovery
- Provides structured logging with DQM labels (e.g., `DQM-Acuranzo-00-LSMFC`)

### Multi-Engine Support

**Supported Engines:**

- **PostgreSQL** - Full implementation with advanced features ✅
- **SQLite** - File-based databases with WAL mode ✅
- **MySQL/MariaDB** - Auto-reconnect and connection pooling ✅
- **IBM DB2** - iSeries support with deadlock detection ✅

**Connection String Examples:**

- PostgreSQL: `postgresql://user:pass@host:5432/database`
- SQLite: `/path/to/database.db` or `:memory:`
- MySQL: `mysql://user:pass@host:3306/database`

### Thread Safety & Performance

- **Single Thread per Queue** - Predictable performance and resource usage
- **Comprehensive Synchronization** - Mutex and semaphore-based thread safety
- **Dynamic Scaling** - Automatic queue spawning/shutdown based on workload
- **Connection Reuse** - Persistent connections per queue for optimal performance
- **Health Monitoring** - 30-second heartbeat intervals with automatic recovery

### Query Watchdog and Transient Failure Recovery

The database subsystem wraps every query (bootstrap, migration, Conduit, health check) with a client-side watchdog and an engine-agnostic retry layer. Together these handle two failure modes that engine-side timeouts cannot:

1. **Client-side network hangs** — a wedged TCP socket, a dropped VPN tunnel, a silent replica failover. Server-side `statement_timeout` cannot fire because the server never receives the query or its reply never makes it back.
2. **Transient transport errors** — momentary connection drops, lock-wait timeouts, serialization failures. Retrying these inside the same operation is far cheaper than failing the request and forcing the caller to retry.

The watchdog is one process-wide background thread that wakes once per second. It maintains a registry of in-flight queries and, for any entry whose age exceeds its configured timeout, logs an `ALERT` and invokes the engine's `cancel_inflight` hook. The hook is engine-specific and is the only piece that needs the database client library to do something the abstraction layer cannot do alone:

| Engine | Cancel mechanism | Thread-safety | Notes |
|---|---|---|---|
| **PostgreSQL** | `PQgetCancel` + `PQcancel` over a fresh TCP connection to the server | Fully cross-thread; uses a separate connection so the wedged socket is never touched | Most reliable of the bunch; designed for exactly this |
| **SQLite** | `sqlite3_interrupt` (sets a flag on the connection struct) | Fully cross-thread; documented safe | No I/O at all |
| **MySQL / MariaDB** | `mysql_kill(thread_id)` on the same connection | Cross-thread (libmysqlclient has internal mutexes) | Best-effort. If the socket is dead this can block the watchdog thread — a future enhancement would add a secondary admin connection |
| **DB2** | `SQLCancel` on the in-flight statement handle (tracked in `DB2Connection::active_stmt` under a mutex) | Cross-thread per ODBC spec | Requires accurate statement tracking; the seven `SQLFreeHandle` sites in `db2/query.c` all call `db2_active_stmt_clear` first |

If any engine's `cancel_inflight` function pointer is `NULL` (e.g. `dlsym` failed to load the cancel symbol at startup), the watchdog silently skips the cancel for that engine and continues logging the ALERT heartbeat. Operators see a one-time `ALERT` at startup naming the missing function.

#### Heartbeat

The first ALERT fires on the transition from healthy to expired. Subsequent ALERTs for the same hung entry fire every 30s so a monitor sees a regular heartbeat instead of either spam (every second) or silence (once and gone). The format is:

```log
[DQM-YDB-00-SMFC] Database query exceeded watchdog timeout: query_id=bootstrap_query, timeout=30s, elapsed=35s
```

The `elapsed=` value increases with each heartbeat, making it obvious the operation is still wedged.

#### Retry on transport/timeout

The engine abstraction layer (`database_engine_execute`) wraps each engine call in a bounded retry loop controlled by `QueryRequest.max_retries`. Between attempts the thread sleeps with exponential backoff (1s, 2s, 4s, 8s, 16s, capped at 30s). The retry decision is driven by `QueryResult.error_class`:

| `error_class` | Source of classification | Retry? |
|---|---|---|
| `DB_ERR_TRANSPORT` | SQLSTATE class `08` (connection_exception), `40` (deadlock/serialization), `53` (insufficient_resources); `mysql_query` "server has gone away", "Lost connection", "Can't connect", "Connection refused" substrings; `pg_result == NULL` path | Yes |
| `DB_ERR_TIMEOUT` | SQLSTATE class `57` (operator_intervention, includes query_canceled and `statement_timeout`); MySQL "Lock wait timeout", "Query execution was interrupted" substrings; `check_timeout_expired` path | Yes |
| `DB_ERR_OTHER` | Default; everything that is not transport or timeout (syntax error, schema mismatch, constraint violation, auth failure) | **No** |

This is the safeguard the user requested: "if there's an actual error returned, then that's not really the same kind of thing." A generated-on-purpose failure in a test (e.g. `query has no destination for result`) gets `DB_ERR_OTHER` and fails immediately, without burning time on retries that won't help. A real network hiccup gets `DB_ERR_TRANSPORT` and is retried with backoff.

#### How retries and the watchdog interact

The watchdog registration covers the *entire* retry series, not each individual attempt. `start_time` is set when `database_engine_execute` first enters; the `effective_timeout_seconds` is the total operation budget across all attempts plus backoff. If attempt 1 hangs the full 30s budget, the watchdog fires the cancel and the engine returns. The retry layer classifies the result (typically `DB_ERR_TRANSPORT` or `DB_ERR_TIMEOUT`) and sleeps for 1s before attempt 2. Attempt 2 starts fresh with a fresh engine call, but the watchdog entry is still the same one — so the elapsed time keeps climbing and the cancel-hook-once-per-entry flag prevents repeated cancels on the same registration.

#### Default configuration

All four knobs are read from the `Databases` object in `hydrogen.json` (see [Database Configuration](/docs/H/core/reference/database_configuration.md#query-watchdog-settings)):

| Setting | Default | Meaning |
|---|---|---|
| `BootstrapTimeoutSeconds` | 30 | Per-query timeout for the bootstrap query and the orphan-DROP |
| `BootstrapRetries` | 3 | Number of retry attempts on `DB_ERR_TRANSPORT`/`DB_ERR_TIMEOUT` for those queries |
| `WatchdogMinSeconds` | 30 | Lower clamp on any `QueryRequest.timeout_seconds` |
| `WatchdogMaxSeconds` | 3600 | Upper clamp on any `QueryRequest.timeout_seconds` |

The watchdog's startup log line shows the effective bounds:

```log
[SR-DATABASE] Database query watchdog initialized (min=30s, max=3600s, default=30s, heartbeat=30s)
```

#### What was deliberately left for later

- A secondary admin MySQL connection for KILL (today's `mysql_kill` on the same connection can block the watchdog thread if the socket is dead).
- Loading `mysql_errno` for proper numeric classification (today's MySQL classification is substring-based on the error message).
- Per-query timeout overrides via a config knob (the per-request `timeout_seconds` field on `QueryRequest` is the way to set this today; the bootstrap path is the only caller that does).

## Usage

### Basic Database Configuration

```json
{
  "Databases": {
    "DefaultWorkers": 2,
    "Connections": [
      {
        "Name": "Acuranzo",
        "Engine": "postgresql",
        "Host": "localhost",
        "Port": 5432,
        "Database": "acuranzo_prod",
        "User": "hydrogen_user",
        "Queues": {
          "Slow": {"Min": 1, "Max": 3},
          "Medium": {"Min": 2, "Max": 5},
          "Fast": {"Min": 1, "Max": 8},
          "Cache": {"Min": 1, "Max": 2}
        }
      }
    ]
  }
}
```

### API Integration

**Query Endpoint:** `/api/query/:database/:query_id/:queue_path/:params`

**Queue Path Options:**

- `/slow` - Background processing, batch operations
- `/medium` - Standard query processing
- `/fast` - High-priority, interactive queries
- `/cache` - Cached or pre-computed results

## Documentation Links

### Core Documentation

- **[DATABASE_PLAN.md](/docs/H/plans/complete/DATABASE_PLAN_COMPLETE.md)** - Comprehensive implementation plan and status
- **[Lead DQM Guide](/docs/H/core/reference/lead_dqm.md)** - Developer guide for Lead DQM implementation and operation
- **[Database Architecture](/docs/H/core/reference/database_architecture.md)** - System architecture overview
- **[Database Configuration](/docs/H/core/reference/database_configuration.md)** - Configuration reference

### Testing Documentation

- **[test_30_database.md](/docs/H/tests/test_30_database.md)** - All Engines Parallel Operational Test
- **[Database Migration Tests](/docs/H/tests/test_31_migrations.md)** - Migration validation testing

### Reference Materials

- **[Database Types](/elements/001-hydrogen/hydrogen/src/database/database_types.h)** - Type definitions and structures
- **[Database Engine Interface](/elements/001-hydrogen/hydrogen/src/database/database_engine.c)** - Multi-engine abstraction layer

## Development Guidelines

### Adding New Database Engines

1. **Implement Engine Interface** - Create engine-specific connection and query functions
2. **Add Dynamic Loading** - Use dlopen/dlsym for runtime library loading
3. **Register Engine** - Add to engine registry system
4. **Create Tests** - Add comprehensive test coverage
5. **Update Documentation** - Document engine-specific features and configuration

### Best Practices

1. **Always Use Lead DQM Pattern** - Don't create multiple queues per database manually
2. **Monitor Queue Depths** - Watch for queues that consistently hit limits
3. **Configure Appropriate Limits** - Set realistic min/max queue counts
4. **Handle Connection Failures** - Implement proper retry and recovery logic
5. **Use Structured Logging** - Leverage DQM labeling system for debugging

### Troubleshooting

**Common Issues:**

- **Connection Failures** - Check database credentials and network connectivity
- **Child Queue Issues** - Verify configuration allows queue creation
- **Performance Problems** - Monitor queue depth and routing logic
- **Memory Leaks** - Check proper cleanup in shutdown sequences

**Debug Logging:**

```bash
# Enable database debug logging
export LOG_LEVEL_DATABASE=1

# Monitor specific database
tail -f /var/log/hydrogen.log | grep "DQM-Acuranzo"
```

## Current Capabilities

The database subsystem currently provides:

✅ **Production-Ready Multi-Engine Support**

- **All Database Engines Operational**: PostgreSQL, SQLite, MySQL, and DB2
- **Full Lead DQM implementation** with dynamic scaling across all engines
- **Comprehensive error handling and recovery** with engine-specific optimizations
- **Thread-safe operations throughout** with proper synchronization
- **Extensive monitoring and structured logging** with DQM labeling system
- **Integration with existing Hydrogen infrastructure** and launch/landing system
- **Dynamic library loading** for deployment flexibility and graceful degradation
- **Bootstrap query execution** working across all database engines

🔄 **Development-Ready Architecture**

- **Unified API surface** providing consistent interface across all engines
- **Comprehensive testing framework** in place for all database types
- **Extensive documentation and developer guides** for all components
- **Engine registry system** for managing multiple database types dynamically

📈 **80% Implementation Complete**

- Core infrastructure: 100% ✅
- All database engines: 100% ✅
- Testing framework: 100% ✅
- Query caching: Next priority
- API integration: Pending
- Production hardening: Final phase

## Support

For questions or issues related to the database subsystem:

1. **Check Documentation** - Review the relevant guides and references above
2. **Review Test Output** - Run test_30_database.sh for diagnostic information
3. **Examine Logs** - Look for DQM-labeled entries in system logs
4. **Verify Configuration** - Ensure database and queue configurations are correct

The database subsystem is designed to be **developer-friendly** with comprehensive documentation, clear error messages, and extensive logging to support debugging and maintenance activities.