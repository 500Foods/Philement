/*
 * Network interface management for the Hydrogen server.
 * 
 * Provides functionality for discovering network interfaces, managing IP addresses,
 * and finding available network ports. Supports both IPv4 and IPv6 addressing,
 * with the ability to track multiple interfaces and their associated addresses.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <net/if.h>      
#include <netinet/in.h> 

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#define MAC_LEN 6
#define MAX_IPS 50
#define MAX_INTERFACES 50

typedef struct {
    char name[IF_NAMESIZE];
    char mac[18];
    char ips[MAX_IPS][INET6_ADDRSTRLEN];
    int ip_count;
} interface_t;

typedef struct {
    int primary_index;
    int count;
    interface_t interfaces[MAX_INTERFACES];
} network_info_t;

int find_available_port(int start_port);
network_info_t *get_network_info();
void free_network_info(network_info_t *info);

#endif // NETWORK_H
