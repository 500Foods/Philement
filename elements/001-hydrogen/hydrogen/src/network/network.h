/*
 * Network Management for Remote 3D Printer Control
 * 
 * Why Robust Networking Matters:
 * 1. Real-time Control
 *    - Command transmission
 *    - Status monitoring
 *    - Emergency stops
 *    - Progress updates
 * 
 * 2. Remote Monitoring
 *    Why These Features?
 *    - Print progress tracking
 *    - Temperature monitoring
 *    - Error detection
 *    - Quality assurance
 * 
 * 3. Security Design
 *    Why So Careful?
 *    - Access control
 *    - Command validation
 *    - Network isolation
 *    - Data protection
 * 
 * 4. Interface Management
 *    Why This Approach?
 *    - Multiple connection types
 *    - Fallback paths
 *    - Address management
 *    - Service discovery
 * 
 * 5. Connection Reliability
 *    Why These Features?
 *    - Print job continuity
 *    - Command reliability
 *    - Status consistency
 *    - Error recovery
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
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

/*
 * Core network functions
 * Why These Functions?
 * - Port management for services
 * - Network interface discovery
 * - Resource cleanup
 * - Graceful shutdown
 */
int find_available_port(int start_port);
network_info_t *get_network_info();
void free_network_info(network_info_t *info);
bool network_shutdown(void);  // Gracefully shuts down all network interfaces

#endif // NETWORK_H
