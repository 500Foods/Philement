/*
 * mDNS Server Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the mDNS server subsystem.
 * The mDNS server handles service advertisement and discovery on the local network.
 */

#ifndef HYDROGEN_STARTUP_MDNS_SERVER_H
#define HYDROGEN_STARTUP_MDNS_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize mDNS server subsystem
 * 
 * This function initializes the mDNS server for service advertisement.
 * It provides zero-configuration networking capabilities:
 * - Dynamic service advertisement
 * - Automatic port configuration
 * - Service filtering based on availability
 * - IPv4 and IPv6 support
 * 
 * Requires network connectivity and proper permissions for
 * multicast DNS operations.
 * 
 * @return 1 on success, 0 on failure
 */
int init_mdns_server_subsystem(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_MDNS_SERVER_H */