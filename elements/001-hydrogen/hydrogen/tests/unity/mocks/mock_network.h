/*
 * Mock network functions for unit testing
 *
 * This file provides mock implementations of network functions
 * used in mDNS and other network-related code to enable unit testing
 * without external network dependencies.
 */

#ifndef MOCK_NETWORK_H
#define MOCK_NETWORK_H

#include <stddef.h>

#ifdef USE_MOCK_NETWORK

#include <src/network/network.h>

// Override network functions with our mocks
#define get_network_info mock_get_network_info
#define filter_enabled_interfaces mock_filter_enabled_interfaces
#define free_network_info mock_free_network_info
#define create_multicast_socket mock_create_multicast_socket

// Mock function prototypes
network_info_t *mock_get_network_info(void);
network_info_t *mock_filter_enabled_interfaces(const network_info_t *raw_net_info, const struct AppConfig *app_config);
void mock_free_network_info(network_info_t *info);
int mock_create_multicast_socket(int family, const char *group, const char *if_name);

// Mock control functions for tests
void mock_network_set_get_network_info_result(network_info_t *result);
void mock_network_set_filter_enabled_interfaces_result(network_info_t *result);
void mock_network_set_create_multicast_socket_result(int result);
void mock_network_reset_all(void);

#endif // USE_MOCK_NETWORK

#endif // MOCK_NETWORK_H