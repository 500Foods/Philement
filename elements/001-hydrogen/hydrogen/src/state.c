#include "state.h"

// Application state flags
volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t shutting_down = 0;
pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

// Component shutdown flags
volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;
volatile sig_atomic_t log_queue_shutdown = 0;
volatile sig_atomic_t mdns_server_shutdown = 0;
volatile sig_atomic_t websocket_server_shutdown = 0;

// Queue Threads
pthread_t log_thread;
pthread_t print_queue_thread;
pthread_t mdns_thread;
pthread_t web_thread;
pthread_t websocket_thread;

// Shared resources
AppConfig *app_config = NULL;
mdns_t *mdns = NULL;
network_info_t *net_info = NULL;