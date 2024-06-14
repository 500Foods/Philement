#ifndef NITRO_NETWORK_H
#define NITRO_NETWORK_H

#include <stdint.h>
#include <net/if.h>      
#include <netinet/in.h> 

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#define NITRO_MAC_LEN 6
#define NITRO_MAX_IPS 50
#define NITRO_MAX_INTERFACES 50

typedef struct {
    char name[IF_NAMESIZE];
    char mac[18];
    char ips[NITRO_MAX_IPS][INET6_ADDRSTRLEN];
    int ip_count;
} nitro_interface_t;

typedef struct {
    int primary_index;
    int count;
    nitro_interface_t interfaces[NITRO_MAX_INTERFACES];
} nitro_network_info_t;

int nitro_find_available_port(int start_port);
nitro_network_info_t *nitro_get_network_info();
void nitro_free_network_info(nitro_network_info_t *info);

#endif // NITRO_NETWORK_H
