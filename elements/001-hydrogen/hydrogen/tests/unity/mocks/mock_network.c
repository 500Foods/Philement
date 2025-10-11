/*
 * Mock network functions for unit testing
 *
 * This file provides mock implementations of network functions
 * used in mDNS and other network-related code to enable unit testing
 * without external network dependencies.
 */

#include "mock_network.h"
#include <src/network/network.h>
#include <src/mdns/mdns_keys.h>
#include <stdlib.h>
#include <string.h>

// Function prototypes
network_info_t *mock_get_network_info(void);
network_info_t *mock_filter_enabled_interfaces(network_info_t *raw_net_info, void *app_config);
void mock_free_network_info(network_info_t *info);
int mock_create_multicast_socket(int family, const char *group, const char *if_name);
void mock_network_set_get_network_info_result(network_info_t *result);
void mock_network_set_filter_enabled_interfaces_result(network_info_t *result);
void mock_network_set_create_multicast_socket_result(int result);
void mock_network_reset_all(void);

// Static variables to store mock state
static network_info_t *mock_get_network_info_result = NULL;
static network_info_t *mock_filter_enabled_interfaces_result = NULL;
static int mock_create_multicast_socket_result = 0;

// Mock implementation of get_network_info
network_info_t *mock_get_network_info(void) {
    return mock_get_network_info_result;
}

// Mock implementation of filter_enabled_interfaces
network_info_t *mock_filter_enabled_interfaces(network_info_t *raw_net_info, void *app_config) {
    (void)raw_net_info;  // Suppress unused parameter warning
    (void)app_config;    // Suppress unused parameter warning
    return mock_filter_enabled_interfaces_result;
}

// Mock implementation of free_network_info
void mock_free_network_info(network_info_t *info) {
    if (info) {
        // In a real mock, we might want to track freed memory
        // For now, just mark as freed
        (void)info;
    }
}

// Mock implementation of create_multicast_socket
int mock_create_multicast_socket(int family, const char *group, const char *if_name) {
    (void)family;  // Suppress unused parameter warning
    (void)group;   // Suppress unused parameter warning
    (void)if_name; // Suppress unused parameter warning
    return mock_create_multicast_socket_result;
}

// Mock control functions
void mock_network_set_get_network_info_result(network_info_t *result) {
    mock_get_network_info_result = result;
}

void mock_network_set_filter_enabled_interfaces_result(network_info_t *result) {
    mock_filter_enabled_interfaces_result = result;
}

void mock_network_set_create_multicast_socket_result(int result) {
    mock_create_multicast_socket_result = result;
}

void mock_network_reset_all(void) {
    mock_get_network_info_result = NULL;
    mock_filter_enabled_interfaces_result = NULL;
    mock_create_multicast_socket_result = 0;
}