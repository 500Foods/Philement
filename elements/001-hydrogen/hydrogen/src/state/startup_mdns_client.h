/*
 * mDNS Client Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the mDNS client subsystem.
 * The mDNS client enables discovery of network services and devices.
 */

#ifndef HYDROGEN_STARTUP_MDNS_CLIENT_H
#define HYDROGEN_STARTUP_MDNS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize mDNS client subsystem
 * 
 * This function initializes the mDNS client for service discovery.
 * It provides network discovery capabilities:
 * - Discover other printers on the network
 * - Find available print services
 * - Locate network resources
 * - Enable auto-configuration
 * 
 * Requires network connectivity and proper permissions for
 * multicast DNS operations.
 * 
 * @return 1 on success, 0 on failure
 */
int init_mdns_client_subsystem(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_MDNS_CLIENT_H */