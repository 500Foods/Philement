/*
 * Launch System Header
 * 
 * DESIGN PRINCIPLES:
 * - All subsystems are equal in importance
 * - No subsystem has inherent priority over others
 * - Dependencies determine what's needed, not importance
 * - The processing order below is for consistency only
 * 
 * This module defines the interfaces for the launch system and its subsystems.
 * All subsystem initialization functions follow the pattern launch_*_subsystem
 * and return 1 on success, 0 on failure.
 * 
 * Standard Processing Order (for consistency, not priority):
 * - 01. Registry (must be first, handles subsystem registration)
 * - 02. Payload
 * - 03. Threads
 * - 04. Network
 * - 05. Database
 * - 06. Logging
 * - 07. WebServer
 * - 08. API
 * - 09. Swagger
 * - 10. WebSockets
 * - 11. Terminal
 * - 12. mDNS Server
 * - 13. mDNS Client
 * - 14. MailRelay
 * - 15. Print
 * 
 * Each subsystem:
 * - Determines its own readiness independently
 * - Manages its own initialization
 * - Launches when its dependencies are met
 * - Is equally important in the system
 */

#ifndef LAUNCH_H
#define LAUNCH_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

// Project includes
#include <src/config/config.h>
#include <src/threads/threads.h>
#include <src/state/state_types.h>  // For SubsystemState and LaunchReadiness

// Core launch functions
ReadinessResults handle_readiness_checks(void);
bool check_all_launch_readiness(void);
bool handle_launch_plan(const ReadinessResults* results);
void handle_launch_review(const ReadinessResults* results);

// Main system startup function
int startup_hydrogen(const char* config_path);

// Utility functions
char* get_current_config_path(void);
LaunchReadiness get_api_readiness(void);

// Dynamic message array utilities for readiness functions
void add_launch_message(const char*** messages, size_t* count, size_t* capacity, const char* message);
void finalize_launch_messages(const char*** messages, const size_t* count, size_t* capacity);
void free_launch_messages(const char** messages, size_t count);
void set_readiness_messages(LaunchReadiness* readiness, const char** messages);

// Subsystem state checking functions
bool is_subsystem_launchable_by_name(const char* name);


void free_threads_resources(void);
void free_payload_resources(void);

// External declarations
extern volatile sig_atomic_t mail_relay_system_shutdown;
extern volatile sig_atomic_t mdns_client_system_shutdown;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t network_system_shutdown;
extern volatile sig_atomic_t print_system_shutdown;
extern volatile sig_atomic_t terminal_system_shutdown;

// Forward deeclarations
void free_webserver_resources(void);
void register_websocket(void);
bool validate_protocol(const char* protocol);
bool validate_key(const char* key);

// Forward declarations from launch_payload.c
extern bool launch_payload(const AppConfig *config, const char *marker);
extern bool check_payload_exists(const char *marker, size_t *size);
extern bool validate_payload_key(const char *key);

// Forward declarations of static functions
void log_readiness_messages(LaunchReadiness* readiness);
void cleanup_readiness_messages(LaunchReadiness* readiness);
void process_subsystem_readiness(ReadinessResults* results, size_t* index,
                                      const char* name, LaunchReadiness readiness);

// General library version extraction function for use in all launch files
char* get_library_version(void* handle, const char* lib_name);

// Version comparison utility function
int version_matches(const char* loaded_version, const char* expected_version);

// Database subsystem validation functions
void validate_database_configuration(const DatabaseConfig* db_config, const char*** messages,
                                    size_t* count, size_t* capacity, bool* overall_readiness,
                                    int* postgres_count, int* mysql_count, int* sqlite_count, int* db2_count);
void check_database_library_dependencies(const char*** messages, size_t* count, size_t* capacity, bool* overall_readiness,
                                       int postgres_count, int mysql_count, int sqlite_count, int db2_count);
bool validate_database_connections(const DatabaseConfig* db_config, const char*** messages,
                                  size_t* count, size_t* capacity);

// Forward declarations for validation helpers for launch_resources.c
bool validate_memory_limits(const ResourceConfig* config, int* msg_count, const char** messages);
bool validate_queue_settings(const ResourceConfig* config, int* msg_count, const char** messages);
bool validate_thread_limits(const ResourceConfig* config, int* msg_count, const char** messages);
bool validate_file_limits(const ResourceConfig* config, int* msg_count, const char** messages);
bool validate_monitoring_settings(const ResourceConfig* config, int* msg_count, const char** messages);

// Forward declaration for register_network function
void register_network(void);

// Forward declaration for register_webserver function
void register_webserver(void);

// Forward declaration for register_mdns_server_for_launch function
void register_mdns_server_for_launch(void);

// Forward declaration for register_mdns_client_for_launch function
void register_mdns_client_for_launch(void);

// Forward declarations for DB2 version detection functions
int parse_major_version(const char* v);
int is_rfc1918_or_local_ip(const char* v);
int is_plausible_db2_version(const char* v, int dots);
int has_db2_keywords_nearby(const char* hay, size_t start, size_t end);
int score_db2_version(const char* hay, size_t start, size_t end, int dots, const char* vstr);

// Subsystem readiness checks (in standard order)
LaunchReadiness check_registry_launch_readiness(void);  // Must be first
LaunchReadiness check_payload_launch_readiness(void);
LaunchReadiness check_threads_launch_readiness(void);
LaunchReadiness check_network_launch_readiness(void);
LaunchReadiness check_database_launch_readiness(void);
LaunchReadiness check_logging_launch_readiness(void);
LaunchReadiness check_webserver_launch_readiness(void);
LaunchReadiness check_api_launch_readiness(void);
LaunchReadiness check_swagger_launch_readiness(void);
LaunchReadiness check_websocket_launch_readiness(void);
LaunchReadiness check_terminal_launch_readiness(void);
LaunchReadiness check_mdns_server_launch_readiness(void);
LaunchReadiness check_mdns_client_launch_readiness(void);
LaunchReadiness check_mail_relay_launch_readiness(void);
LaunchReadiness check_print_launch_readiness(void);
LaunchReadiness check_notify_launch_readiness(void);
LaunchReadiness check_oidc_launch_readiness(void);
LaunchReadiness check_resources_launch_readiness(void);

// Subsystem launch functions
int launch_registry_subsystem(bool is_restart);  // Must be first
int launch_payload_subsystem(void);
int launch_threads_subsystem(void);
int launch_network_subsystem(void);
int launch_notify_subsystem(void);
int launch_database_subsystem(void);
int launch_logging_subsystem(void);
int launch_webserver_subsystem(void);
int launch_api_subsystem(void);
int launch_swagger_subsystem(void);
int launch_websocket_subsystem(void);
int launch_terminal_subsystem(void);
int launch_mdns_server_subsystem(void);
int launch_mdns_client_subsystem(void);
int launch_mail_relay_subsystem(void);
int launch_print_subsystem(void);
int launch_oidc_subsystem(void);
int launch_resources_subsystem(void);

// Log early startup information (before any initialization)
void log_early_info(void);

// Convert subsystem name to uppercase
char* get_uppercase_name(const char* name);

// Launch approved subsystems in registry order
bool launch_approved_subsystems(ReadinessResults* results);

#endif /* LAUNCH_H */
