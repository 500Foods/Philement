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

// Core application state flags
// keep_running: Main program loop control
// shutting_down: System shutdown indicator
// Condition variables for thread coordination
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t shutting_down = 0;
pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

// Individual component shutdown flags
// Each flag controls the shutdown of a specific subsystem
// Order is important for clean shutdown sequence
// All flags are atomic for thread safety
volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;
volatile sig_atomic_t log_queue_shutdown = 0;
volatile sig_atomic_t mdns_server_shutdown = 0;
volatile sig_atomic_t websocket_server_shutdown = 0;

// Thread handles for all system components
// Used for:
// - Thread lifecycle management
// - Shutdown coordination
// - Resource cleanup
pthread_t log_thread;
pthread_t print_queue_thread;
pthread_t mdns_thread;
pthread_t web_thread;
pthread_t websocket_thread;

// Global shared resources
// Managed centrally for:
// - Consistent access
// - Lifecycle control
// - Resource cleanup
AppConfig *app_config = NULL;
mdns_t *mdns = NULL;
network_info_t *net_info = NULL;