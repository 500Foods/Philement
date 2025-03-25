/*
 * Safety-Critical State Management for 3D Printer Control
 * 
 */

#include "state.h"

// Core state flags

volatile sig_atomic_t server_starting = 1;  // Start as true, will be set to false once startup complete
volatile sig_atomic_t server_running = 0;
volatile sig_atomic_t server_stopping = 0;

pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

// Component-specific shutdown flags with dependency awareness

volatile sig_atomic_t log_queue_shutdown = 0;
volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t websocket_server_shutdown = 0;
volatile sig_atomic_t mdns_server_system_shutdown = 0;
volatile sig_atomic_t mdns_client_system_shutdown = 0;
volatile sig_atomic_t smtp_relay_system_shutdown = 0;
volatile sig_atomic_t swagger_system_shutdown = 0;
volatile sig_atomic_t terminal_system_shutdown = 0;
volatile sig_atomic_t print_system_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;

// System thread handles with lifecycle management

pthread_t log_thread;
pthread_t web_thread;
pthread_t websocket_thread;
pthread_t mdns_server_thread;
pthread_t print_queue_thread;

// Thread tracking structures 

ServiceThreads logging_threads;
ServiceThreads web_threads;
ServiceThreads websocket_threads;
ServiceThreads mdns_server_threads;
ServiceThreads print_threads;

// Shared resource handles

AppConfig *app_config = NULL;
mdns_server_t *mdns_server = NULL;
network_info_t *net_info = NULL;