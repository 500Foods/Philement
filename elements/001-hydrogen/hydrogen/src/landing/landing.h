/*
 * Landing System Header
 * 
 * This module defines the interfaces for the landing (shutdown) system.
 * It coordinates the shutdown sequence through specialized modules:
 * - landing_readiness.c - Handles readiness checks
 * - landing_plan.c - Coordinates landing sequence
 * - landing_review.c - Reports landing status
 */

#ifndef LANDING_H
#define LANDING_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <signal.h>  // For sig_atomic_t

// Project includes
#include "landing_readiness.h"

// Restart control
extern volatile sig_atomic_t restart_requested;
extern volatile int restart_count;

// Core landing functions
bool check_all_landing_readiness(void);
void handle_sighup(void);  // SIGHUP signal handler for restart
void handle_sigint(void);  // SIGINT signal handler for shutdown

// Individual subsystem landing functions
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

// Payload landing helper functions
void free_payload_resources(void);

// Program argument access for restart functionality
char** get_program_args(void);

#endif /* LANDING_H */
