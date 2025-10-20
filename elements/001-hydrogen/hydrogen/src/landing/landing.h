/*
 * Landing System Header
 * 
 * This module defines the interfaces for the landing (shutdown) system.
 * 
 */

#ifndef LANDING_H
#define LANDING_H

// Restart control
extern volatile sig_atomic_t restart_requested;
extern volatile int restart_count;

// Core landing functions
extern char** get_program_args(void);
bool check_all_landing_readiness(void); // Check readiness of all subsystems for landing
void report_registry_landing_status(void); // Report final registry status during landing

void handle_sighup(void);  // SIGHUP signal handler for restart
void handle_sigint(void);  // SIGINT signal handler for shutdown

// Internal landing functions (exposed for unit testing)
typedef int (*LandingFunction)(void);
LandingFunction get_landing_function(const char* subsystem_name);
bool land_approved_subsystems(ReadinessResults* results);

ReadinessResults handle_landing_readiness(void);
bool handle_landing_plan(const ReadinessResults* results);
void handle_landing_review(const ReadinessResults* results, time_t start_time);

// Landing review helper functions (exposed for unit testing)
void report_thread_cleanup_status(void);
void report_final_landing_summary(const ReadinessResults* results);

// Payload landing helper functions
void free_payload_resources(void);

// Resources landing helper functions
void free_resources_resources(void);

// Shutdown functions
void shutdown_network_subsystem(void);

// Thread management helper functions (exposed for unit testing)
int get_thread_subsystem_id(void);
void free_thread_resources(void);

void shutdown_registry(void);
void shutdown_payload(void);
void shutdown_threads(void);
void shutdown_network(void);
void shutdown_database(void);
void shutdown_logging(void);
void shutdown_webserver(void);
void shutdown_api(void);
void shutdown_swagger(void);
void shutdown_websocket(void);
void shutdown_terminal(void);
void shutdown_mdns_server(void);
void shutdown_mdns_client(void);
void shutdown_mail_relay(void);
void shutdown_resources(void);
void shutdown_notify(void);
void shutdown_oidc(void);
void shutdown_print_queue(void);

// Landing functions
int land_api_subsystem(void);
int land_network_subsystem(void);
int land_mail_relay_subsystem(void);
int land_database_subsystem(void);
int land_mdns_client_subsystem(void);
int land_logging_subsystem(void);
int land_mdns_server_subsystem(void);
int land_payload_subsystem(void);
int land_print_subsystem(void);
int land_swagger_subsystem(void);
int land_terminal_subsystem(void);
int land_webserver_subsystem(void);
int land_websocket_subsystem(void);
int land_threads_subsystem(void);
int land_resources_subsystem(void);
int land_oidc_subsystem(void);
int land_notify_subsystem(void);
int land_registry_subsystem(bool is_restart);

// Landing readiness checks
LaunchReadiness check_registry_landing_readiness(void);
LaunchReadiness check_print_landing_readiness(void);
LaunchReadiness check_mail_relay_landing_readiness(void);
LaunchReadiness check_mdns_client_landing_readiness(void);
LaunchReadiness check_mdns_server_landing_readiness(void);
LaunchReadiness check_terminal_landing_readiness(void);
LaunchReadiness check_websocket_landing_readiness(void);
LaunchReadiness check_swagger_landing_readiness(void);
LaunchReadiness check_api_landing_readiness(void);
LaunchReadiness check_webserver_landing_readiness(void);
LaunchReadiness check_database_landing_readiness(void);
LaunchReadiness check_logging_landing_readiness(void);
LaunchReadiness check_network_landing_readiness(void);
LaunchReadiness check_payload_landing_readiness(void);
LaunchReadiness check_threads_landing_readiness(void);
LaunchReadiness check_resources_landing_readiness(void);
LaunchReadiness check_oidc_landing_readiness(void);
LaunchReadiness check_notify_landing_readiness(void);

// Free resources allocated during OIDC launch
void free_oidc_resources(void);

// Free resources allocated during notify launch
void free_notify_resources(void);

#endif /* LANDING_H */
