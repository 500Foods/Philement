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

- **[Lead DQM](reference/lead_dqm.md)** - Central coordinator for each database instance
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

- **[DATABASE_PLAN.md](plans/DATABASE_PLAN.md)** - Comprehensive implementation plan and status
- **[Lead DQM Guide](reference/lead_dqm.md)** - Developer guide for Lead DQM implementation and operation
- **[Database Architecture](reference/database_architecture.md)** - System architecture overview
- **[Database Configuration](reference/database_configuration.md)** - Configuration reference

### Testing Documentation

- **[test_30_database.md](../tests/docs/test_30_database.md)** - All Engines Parallel Operational Test
- **[Database Migration Tests](../tests/docs/test_31_migrations.md)** - Migration validation testing

### Reference Materials

- **[Database Types](../src/database/database_types.h)** - Type definitions and structures
- **[Database Engine Interface](../src/database/database_engine.c)** - Multi-engine abstraction layer

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