/*
 * Landing Plan Header
 * 
 * This module defines the interfaces for coordinating the landing (shutdown) sequence.
 * It provides functions for:
 * - Making Go/No-Go decisions for each subsystem shutdown
 * - Tracking landing status and counts
 * - Coordinating with the registry for subsystem deregistration
 */

#ifndef LANDING_PLAN_H
#define LANDING_PLAN_H

// System includes
#include <stdbool.h>

// Local includes
#include "landing.h"

// Execute the landing plan and make Go/No-Go decisions for shutdown
bool handle_landing_plan(const ReadinessResults* results);

// Shutdown functions
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
void shutdown_print_queue(void);

#endif /* LANDING_PLAN_H */
