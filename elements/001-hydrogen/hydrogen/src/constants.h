/* constants.c
 *
 * Global constants
 *
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Define RELEASE if not provided by compiler
#ifndef RELEASE
    #define RELEASE "unknown"
#endif

// Define BUILD_TYPE if not provided by compiler
#ifndef BUILD_TYPE
    #define BUILD_TYPE "unknown"
#endif

// Resource limits
#define MIN_MEMORY_MB 64
#define MAX_MEMORY_MB 16384
#define MIN_RESOURCE_BUFFER_SIZE 1024
#define MAX_RESOURCE_BUFFER_SIZE (1024 * 1024 * 1024)
#define MIN_THREADS 2
#define MAX_THREADS 1024
#define MIN_STACK_SIZE (16 * 1024)
#define MAX_STACK_SIZE (8 * 1024 * 1024)
#define MIN_OPEN_FILES 64
#define MAX_OPEN_FILES 65536
#define MIN_LOG_SIZE_MB 1
#define MAX_LOG_SIZE_MB 10240
#define MIN_CHECK_INTERVAL_MS 100
#define MAX_CHECK_INTERVAL_MS 60000

// Notification configuration limits
#define MIN_SMTP_PORT 1
#define MAX_SMTP_PORT 65535
#define MIN_SMTP_TIMEOUT 1
#define MAX_SMTP_TIMEOUT 300
#define MIN_SMTP_RETRIES 0
#define MAX_SMTP_RETRIES 10

// OIDC configuration limits
#define MIN_OIDC_PORT 1024
#define MAX_OIDC_PORT 65535
#define MIN_TOKEN_LIFETIME 300        // 5 minutes
#define MAX_TOKEN_LIFETIME 86400      // 24 hours
#define MIN_REFRESH_LIFETIME 3600     // 1 hour
#define MAX_REFRESH_LIFETIME 2592000  // 30 days
#define MIN_KEY_ROTATION_DAYS 1
#define MAX_KEY_ROTATION_DAYS 90
// Queue validation limits
#define MIN_QUEUED_JOBS 1
#define MAX_QUEUED_JOBS 1000
#define MIN_CONCURRENT_JOBS 1
#define MAX_CONCURRENT_JOBS 16

// Priority validation limits
#define MIN_PRIORITY 0
#define MAX_PRIORITY 100
#define MIN_PRIORITY_SPREAD 10  // Minimum difference between priority levels

// Buffer validation limits
#define MIN_MESSAGE_SIZE 128
#define MAX_MESSAGE_SIZE 16384  // 16KB

// Timeout validation limits (in milliseconds)
#define MIN_SHUTDOWN_WAIT 1000      // 1 second
#define MAX_SHUTDOWN_WAIT 30000     // 30 seconds
#define MIN_JOB_TIMEOUT 30000       // 30 seconds
#define MAX_JOB_TIMEOUT 3600000     // 1 hour

// Motion control validation limits
#define MIN_SPEED 0.1
#define MAX_SPEED 1000.0
#define MIN_ACCELERATION 0.1
#define MAX_ACCELERATION 5000.0
#define MIN_JERK 0.01
#define MAX_JERK 100.0

// Network constants
#ifndef NI_MAXHOST
    #define NI_MAXHOST 1025
#endif

#ifndef NI_NUMERICHOST
    #define NI_NUMERICHOST 0x02
#endif

// Validation limits
#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_EXIT_WAIT_SECONDS 1
#define MAX_EXIT_WAIT_SECONDS 60
#define WEBSOCKET_MIN_MESSAGE_SIZE 1024        // 1KB
#define WEBSOCKET_MAX_MESSAGE_SIZE 0x40000000  // 1GB

#endif /* CONSTANTS_H */

