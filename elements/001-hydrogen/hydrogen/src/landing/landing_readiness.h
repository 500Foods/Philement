/*
 * Landing Readiness Header
 * 
 * This module defines the interfaces for checking subsystem readiness
 * before shutdown. Each subsystem must implement a readiness check
 * that verifies it can be safely shut down.
 */

#ifndef LANDING_READINESS_H
#define LANDING_READINESS_H

// System includes
#include <stdbool.h>

// Local includes
#include "landing.h"

// Handle all readiness checks
ReadinessResults handle_readiness_checks(void);

// Individual subsystem readiness checks
LandingReadiness check_logging_landing_readiness(void);
LandingReadiness check_database_landing_readiness(void);
LandingReadiness check_terminal_landing_readiness(void);
LandingReadiness check_mdns_server_landing_readiness(void);
LandingReadiness check_mdns_client_landing_readiness(void);
LandingReadiness check_mail_relay_landing_readiness(void);
LandingReadiness check_swagger_landing_readiness(void);
LandingReadiness check_webserver_landing_readiness(void);
LandingReadiness check_websocket_landing_readiness(void);
LandingReadiness check_print_landing_readiness(void);
LandingReadiness check_payload_landing_readiness(void);
LandingReadiness check_threads_landing_readiness(void);
LandingReadiness check_network_landing_readiness(void);
LandingReadiness check_api_landing_readiness(void);
LandingReadiness check_subsystem_registry_landing_readiness(void);

#endif /* LANDING_READINESS_H */