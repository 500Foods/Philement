/*
 * Global state management for the Hydrogen printer server.
 * 
 * Manages the global state and shared resources of the system, including
 * thread handles, shutdown flags, and synchronization primitives. This
 * module is critical for coordinating the lifecycle of all components.
 * 
 * State Management:
 * - Runtime flags for system state
 * - Component shutdown coordination
 * - Thread handle tracking
 * - Resource lifecycle management
 * 
 * Thread Coordination:
 * - Mutex for state changes
 * - Condition variable for notifications
 * - Atomic operations for flags
 * - Thread handle tracking
 * 
 * Shutdown Sequence:
 * - Individual component flags
 * - Coordinated shutdown order
 * - Resource cleanup tracking
 * - Thread termination sync
 * 
 * Resource Management:
 * - Configuration state
 * - Network resources
 * - Service handles
 * - Memory tracking
 * 
 * Dependencies:
 * - Used by all major components
 * - Critical for system stability
 * - Required for clean shutdown
 * - Core to resource management
 */

#include "state.h"

// Core state flags implementing coordinated lifecycle management
//
// Design choices for global state:
// 1. Atomic Operations
//    - sig_atomic_t for signal safety
//    - Prevents partial updates
//    - Ensures visibility across cores
//
// 2. Minimal State
//    - Binary flags only
//    - No complex state machines
//    - Clear state transitions
//
// 3. Thread Coordination
//    - Mutex for state changes
//    - Condition variable for waiting
//    - Broadcast notifications
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t shutting_down = 0;
pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

// Component-specific shutdown flags with dependency awareness
//
// Shutdown flag design prioritizes:
// 1. Dependency Order
//    - mDNS first (stop advertising)
//    - Network services next
//    - Core systems last
//
// 2. Safety
//    - Atomic operations only
//    - Independent state tracking
//    - Prevents deadlocks
//
// 3. Observability
//    - Clear shutdown progress
//    - Component state tracking
//    - Debugging support
volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;
volatile sig_atomic_t log_queue_shutdown = 0;
volatile sig_atomic_t mdns_server_shutdown = 0;
volatile sig_atomic_t websocket_server_shutdown = 0;

// System thread handles with lifecycle management
//
// Thread handle centralization enables:
// 1. Resource Management
//    - Consistent cleanup
//    - Join operations
//    - Handle validation
//
// 2. Shutdown Coordination
//    - Ordered termination
//    - Resource release
//    - Deadlock prevention
//
// 3. State Tracking
//    - Component health
//    - Resource usage
//    - System monitoring
pthread_t log_thread;
pthread_t print_queue_thread;
pthread_t mdns_thread;
pthread_t web_thread;
pthread_t websocket_thread;

// Shared resource handles with centralized management
//
// Resource management strategy:
// 1. Access Control
//    - Single source of truth
//    - Controlled initialization
//    - Safe deallocation
//
// 2. Lifecycle Management
//    - Dependency tracking
//    - Ordered initialization
//    - Clean shutdown
//
// 3. Memory Safety
//    - Clear ownership
//    - Null safety
//    - Leak prevention
AppConfig *app_config = NULL;
mdns_t *mdns = NULL;
network_info_t *net_info = NULL;