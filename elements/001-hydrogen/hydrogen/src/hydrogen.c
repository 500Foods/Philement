/**
 * @file hydrogen.c
 * @brief Main entry point for the Hydrogen Server
 */

// Global includes
#include <src/hydrogen.h>

// Internal Headers
#include <src/handlers/handlers.h>
#include <src/launch/launch.h>
#include <src/landing/landing.h>

/* Global Variables and External Declarations */
extern void signal_handler(int sig);
extern volatile sig_atomic_t server_running;
extern pthread_mutex_t terminate_mutex;
extern pthread_cond_t terminate_cond;
extern ServiceThreads logging_threads;
pthread_t main_thread_id;

/**
 * @brief Main entry point for the Hydrogen server
 */

int main(int argc, char *argv[]) {

    // Initialize locale
    setlocale(LC_NUMERIC, "");

    // Handle command-line options
    const char* config_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printf("Hydrogen ver %s rel %s\n\n", VERSION, RELEASE);
            printf("hydrogen [JSON config file] [options]\n\n");
            printf("--help    This screen\n");
            printf("--version Version information\n\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("Hydrogen ver %s rel %s\n", VERSION, RELEASE);
            return 0;
        } else if (argv[i][0] == '-') {
            // Unknown option
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Use --help for usage information\n");
            return 1;
        } else {
            // First non-option argument is the config file
            if (config_path == NULL) {
                config_path = argv[i];
            } else {
                // Multiple config files specified
                fprintf(stderr, "Multiple config files specified\n");
                fprintf(stderr, "Use --help for usage information\n");
                return 1;
            }
        }
    }

    // Get the executable size
    get_executable_size(argv);

     // Store program arguments for restart
     store_program_args(argc, argv);
     if (prctl(PR_SET_DUMPABLE, 1) == -1) {
         log_this(SR_STARTUP, "Failed to set dumpable: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
     }
 
     struct rlimit core_limit = { 10 * 1024 * 1024, 10 * 1024 * 1024 };
     if (setrlimit(RLIMIT_CORE, &core_limit) == -1) {
         log_this(SR_STARTUP, "Failed to enable core dumps: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
     }
 
     main_thread_id = pthread_self();
 
     /* Initialize signal handlers
      * Three types of signals are handled:
      * 1. Normal termination signals (SIGINT, SIGTERM, SIGHUP)
      *    - Used for graceful shutdown
      *    - SA_RESTART: Automatically restart interrupted system calls
      *    - SA_NODEFER: Allow signal to be received again while in handler
      */
     struct sigaction sa;
     sa.sa_handler = signal_handler; // Assuming signal_handler is defined elsewhere
     sigfillset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART | SA_NODEFER;
     if (sigaction(SIGINT, &sa, NULL) == -1 ||
         sigaction(SIGTERM, &sa, NULL) == -1 ||
         sigaction(SIGHUP, &sa, NULL) == -1) {
         fprintf(stderr, "Failed to set up signal handlers\n");
         return 1;
     }
 
     /* 2. Fatal crash signals (SIGSEGV, SIGABRT, SIGFPE)
      *    - Handled by crash_handler with detailed core dump generation
      *    - SA_SIGINFO: Provides extended signal information
      *    - SA_RESTART: Automatically restart interrupted system calls
      */
     struct sigaction sa_crash;
     memset(&sa_crash, 0, sizeof(sa_crash));
     sa_crash.sa_flags = SA_SIGINFO | SA_RESTART;
     sa_crash.sa_sigaction = crash_handler;
     sigemptyset(&sa_crash.sa_mask);
     if (sigaction(SIGSEGV, &sa_crash, NULL) == -1 ||
         sigaction(SIGABRT, &sa_crash, NULL) == -1 ||
         sigaction(SIGFPE, &sa_crash, NULL) == -1) {
         log_this(SR_STARTUP, "Failed to set crash signal handlers: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
         return 1;
     }

     /* 3. Test crash signal handler (SIGUSR1)
      *    - Used for development/testing of crash handler
      *    - Triggers a controlled crash via null pointer dereference
      *    - SA_RESTART: Automatically restart interrupted system calls
      */
     struct sigaction sa_test;
     sa_test.sa_handler = test_crash_handler;
     sigemptyset(&sa_test.sa_mask);
     sa_test.sa_flags = SA_RESTART;
     if (sigaction(SIGUSR1, &sa_test, NULL) == -1) {
         log_this(SR_STARTUP, "Failed to set test crash signal handler: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
     }

     /* 4. Config dump signal handler (SIGUSR2)
      *    - Used to dump current configuration to logs
      *    - Provides runtime inspection of configuration
      *    - SA_RESTART: Automatically restart interrupted system calls
      */
     struct sigaction sa_dump;
     sa_dump.sa_handler = config_dump_handler;
     sigemptyset(&sa_dump.sa_mask);
     sa_dump.sa_flags = SA_RESTART;
     if (sigaction(SIGUSR2, &sa_dump, NULL) == -1) {
         log_this(SR_STARTUP, "Failed to set config dump signal handler: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
     }

     if (!startup_hydrogen(config_path)) {
         return 1;
     }
 
     struct timespec ts;
     while (server_running) {
         clock_gettime(CLOCK_REALTIME, &ts);
         ts.tv_sec += 1;
         pthread_mutex_lock(&terminate_mutex);
         int wait_result = pthread_cond_timedwait(&terminate_cond, &terminate_mutex, &ts);
         pthread_mutex_unlock(&terminate_mutex);
         if (wait_result != 0 && wait_result != ETIMEDOUT) {
             log_this(SR_CRASH, "Unexpected error in main event loop: %d", LOG_LEVEL_ERROR, 1, wait_result);
         }
     }
 
     add_service_thread(&logging_threads, main_thread_id);
     graceful_shutdown();
     remove_service_thread(&logging_threads, main_thread_id);
 
     return 0;
 }
