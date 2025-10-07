/*
 * Database Queue Infrastructure
 *
 * Implements multi-queue architecture for database operations as described in DATABASE_PLAN.md Phase 1.
 * Extends the base queue system with database-specific functionality.
 */

#ifndef DATABASE_QUEUE_H
#define DATABASE_QUEUE_H

// Project includes
#include "../hydrogen.h"
#include "../queue/queue.h"
#include "database_types.h"

// Forward declarations to avoid circular dependencies
typedef struct DatabaseHandle DatabaseHandle;
typedef struct DatabaseEngineInterface DatabaseEngineInterface;

// Queue types per database connection for different priority levels
#define QUEUE_TYPE_SLOW    "slow"
#define QUEUE_TYPE_MEDIUM  "medium"
#define QUEUE_TYPE_FAST    "fast"
#define QUEUE_TYPE_CACHE   "cache"

// Forward declaration for self-referencing structure
typedef struct DatabaseQueue DatabaseQueue;

// Database-specific queue wrapper that manages multiple queues
struct DatabaseQueue {
    char* database_name;           // Database identifier (e.g., "Acuranzo")
    char* connection_string;       // Database connection string
    DatabaseEngine engine_type;    // Database engine type (PostgreSQL, SQLite, MySQL, DB2)
    char* queue_type;              // Queue type: "Lead", "slow", "medium", "fast", "cache"
    char* bootstrap_query;         // Bootstrap query from config (only used by Lead queues)

    // Single queue instance - type determined by queue_type
    Queue* queue;                  // The actual queue for this worker

    // Worker thread management - single thread per queue
    pthread_t worker_thread;
    volatile bool worker_thread_started; // True if worker thread has been started

    // Queue role flags
    volatile bool is_lead_queue;   // True if this is the Lead queue for the database
    volatile bool can_spawn_queues; // True if this queue can create additional queues

    // Tag management for logging and identification
    char* tags;                    // Current tags assigned to this DQM (e.g., "LSMFC", "F", "L")
    int queue_number;              // Sequential queue number for this database (00, 01, 02, etc.)

    // Queue statistics
    volatile int active_connections;
    volatile int total_queries_processed;
    volatile int current_queue_depth;

    // Thread synchronization
    pthread_mutex_t queue_access_lock;
    sem_t worker_semaphore;        // Controls worker thread operation

    // Lead queue management (only used by Lead queues)
    DatabaseQueue** child_queues;  // Array of spawned queues (slow/medium/fast/cache)
    int child_queue_count;         // Number of active child queues
    int max_child_queues;          // Maximum child queues allowed
    pthread_mutex_t children_lock; // Protects child queue operations

    // Timer loop management for heartbeat and monitoring
    volatile time_t last_heartbeat; // Timestamp of last heartbeat check
    volatile time_t last_connection_attempt; // Timestamp of last connection attempt
    volatile int heartbeat_interval_seconds; // Configurable heartbeat interval (default 30)

    // Persistent database connection for this queue
    DatabaseHandle* persistent_connection; // Maintained connection for query execution
    pthread_mutex_t connection_lock;       // Protects persistent connection access

    // Flags
    volatile bool shutdown_requested;
    volatile bool is_connected;
    volatile bool bootstrap_completed;  // True when bootstrap query has completed (Lead queues only)
    volatile bool initial_connection_attempted;  // True when initial connection attempt is complete (Lead queues only)

    // Bootstrap completion synchronization (Lead queues only)
    pthread_mutex_t bootstrap_lock;
    pthread_cond_t bootstrap_cond;

    // Initial connection attempt synchronization (Lead queues only)
    pthread_mutex_t initial_connection_lock;
    pthread_cond_t initial_connection_cond;
};

// Database queue manager that coordinates multiple databases
typedef struct DatabaseQueueManager {
    DatabaseQueue** databases;      // Array of database queues
    size_t database_count;          // Number of databases managed
    size_t max_databases;           // Maximum supported databases

    // Round-robin distribution state
    volatile size_t next_database_index;

    // Manager-wide statistics
    volatile long long total_queries;
    volatile long long successful_queries;
    volatile long long failed_queries;

    // Thread pool management
    pthread_mutex_t manager_lock;
    bool initialized;
} DatabaseQueueManager;

// Query metadata for database operations
typedef struct DatabaseQuery {
    char* query_id;                // Unique query identifier
    char* query_template;          // Parameterized SQL template
    char* parameter_json;          // JSON parameters for injection
    int queue_type_hint;           // Suggested queue type (0=slow, 1=medium, 2=fast, 3=cache)
    time_t submitted_at;
    time_t processed_at;
    int retry_count;
    char* error_message;
} DatabaseQuery;

// Queue type enumeration for consistent indexing
typedef enum {
    DB_QUEUE_SLOW = 0,
    DB_QUEUE_MEDIUM = 1,
    DB_QUEUE_FAST = 2,
    DB_QUEUE_CACHE = 3,
    DB_QUEUE_MAX_TYPES
} DatabaseQueueType;

// External reference to global queue manager
extern DatabaseQueueManager* global_queue_manager;

// Function prototypes

// Initialize database queue infrastructure
bool database_queue_system_init(void);
void database_queue_system_destroy(void);

// Database queue management
DatabaseQueue* database_queue_create_lead(const char* database_name, const char* connection_string, const char* bootstrap_query);
DatabaseQueue* database_queue_create_worker(const char* database_name, const char* connection_string, const char* queue_type, const char* dqm_label);
void database_queue_destroy(DatabaseQueue* db_queue);

// Queue manager operations
DatabaseQueueManager* database_queue_manager_create(size_t max_databases);
void database_queue_manager_destroy(DatabaseQueueManager* manager);
bool database_queue_manager_add_database(DatabaseQueueManager* manager, DatabaseQueue* db_queue);
DatabaseQueue* database_queue_manager_get_database(DatabaseQueueManager* manager, const char* name);

// Query operations
bool database_queue_submit_query(DatabaseQueue* db_queue, DatabaseQuery* query);
DatabaseQuery* database_queue_process_next(DatabaseQueue* db_queue);
DatabaseQuery* database_queue_await_result(DatabaseQueue* db_queue, const char* query_id, int timeout_seconds);

// Worker thread management (single generic worker)
void* database_queue_worker_thread(void* arg);
bool database_queue_start_worker(DatabaseQueue* db_queue);
void database_queue_stop_worker(DatabaseQueue* db_queue);

// Lead queue management
bool database_queue_spawn_child_queue(DatabaseQueue* lead_queue, const char* queue_type);
bool database_queue_shutdown_child_queue(DatabaseQueue* lead_queue, const char* queue_type);
void database_queue_manage_child_queues(DatabaseQueue* lead_queue);

// Statistics and monitoring
size_t database_queue_get_depth(DatabaseQueue* db_queue);
size_t database_queue_get_depth_with_designator(DatabaseQueue* db_queue, const char* designator);
void database_queue_get_stats(DatabaseQueue* db_queue, char* buffer, size_t buffer_size);
bool database_queue_health_check(DatabaseQueue* db_queue);

// Utility functions
const char* database_queue_type_to_string(int queue_type);
int database_queue_type_from_string(const char* type_str);
DatabaseQueueType database_queue_select_type(const char* queue_path_hint);

// Tag management functions
bool database_queue_set_tags(DatabaseQueue* db_queue, const char* tags);
char* database_queue_get_tags(const DatabaseQueue* db_queue);
bool database_queue_add_tag(DatabaseQueue* db_queue, char tag);
bool database_queue_remove_tag(DatabaseQueue* db_queue, char tag);
char* database_queue_generate_label(DatabaseQueue* db_queue);

// Timer and heartbeat functions
void database_queue_start_heartbeat(DatabaseQueue* db_queue);
bool database_queue_check_connection(DatabaseQueue* db_queue);
void database_queue_perform_heartbeat(DatabaseQueue* db_queue);
void database_queue_execute_bootstrap_query(DatabaseQueue* db_queue);

// Heartbeat utility functions (for testing)
DatabaseEngine database_queue_determine_engine_type(const char* connection_string);
char* database_queue_mask_connection_string(const char* connection_string);
void database_queue_signal_initial_connection_complete(DatabaseQueue* db_queue);

// Launch synchronization functions
bool database_queue_wait_for_initial_connection(DatabaseQueue* db_queue, int timeout_seconds);

// Debug functions
// void debug_dump_connection(const char* label, const DatabaseHandle* conn, const char* dqm_label);
// void debug_dump_engine(const char* label, DatabaseEngineInterface* engine, const char* dqm_label);

#endif // DATABASE_QUEUE_H
