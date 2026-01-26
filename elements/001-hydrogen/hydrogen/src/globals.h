// globals.h

#ifndef GLOBALS_H
#define GLOBALS_H

// VERSIONING /////////////////////////////////////////////////////////////////////////////////////
// These should be set by build system, eg: cmake

#ifndef VERSION
    #define VERSION "unknown"   
#endif

#ifndef RELEASE
    #define RELEASE "unknown"   
#endif

#ifndef BUILD_TYPE
    #define BUILD_TYPE "unknown"
#endif

// SECRETS ////////////////////////////////////////////////////////////////////////////////////////

#define SECRET_KEY_LENGTH 32                            // Used for JWTs and equivalents
#define PAYLOAD_MARKER "<<< HERE BE ME TREASURE >>>"    // Not really secret, just a marker
#define HYDROGEN_AUTH_SCHEME "Key"                      // Websocket authentication scheme

// LIMITS /////////////////////////////////////////////////////////////////////////////////////////

#define MAX_SERVICE_THREADS 1024

#define MIN_MEMORY_MB 64
#define MAX_MEMORY_MB 16384
#define MIN_RESOURCE_BUFFER_SIZE 1024
#define MAX_RESOURCE_BUFFER_SIZE (1024 * 1024 * 1024)
#define MIN_THREADS 2
#define MAX_THREADS MAX_SERVICE_THREADS
#define MIN_STACK_SIZE (16 * 1024)
#define MAX_STACK_SIZE (8 * 1024 * 1024)
#define MIN_OPEN_FILES 64
#define MAX_OPEN_FILES 65536
#define MIN_LOG_SIZE_MB 1
#define MAX_LOG_SIZE_MB 10240
#define MIN_CHECK_INTERVAL_MS 100
#define MAX_CHECK_INTERVAL_MS 60000

#define MAX_VERSION_STRING 64
#define MAX_SYSINFO_STRING 256
#define MAX_PATH_STRING 1024
#define MAX_TYPE_STRING 32
#define MAX_DESC_STRING 256
#define MAX_PERCENTAGE_STRING 32

#define MIN_MESSAGE_SIZE 128
#define MAX_MESSAGE_SIZE 16384      // 16KB

#define MIN_SHUTDOWN_WAIT 1000      // 1 second
#define MAX_SHUTDOWN_WAIT 30000     // 30 seconds
#define MIN_JOB_TIMEOUT 30000       // 30 seconds
#define MAX_JOB_TIMEOUT 3600000     // 1 hour

#define DEFAULT_LINE_BUFFER_SIZE 4096           // 4KB for line buffers
#define DEFAULT_LOG_BUFFER_SIZE 8192            // 8KB for log messages
#define DEFAULT_POST_PROCESSOR_BUFFER_SIZE 8192 // 8KB for post processing
#define DEFAULT_COMMAND_BUFFER_SIZE 4096        // 4KB for commands
#define DEFAULT_RESPONSE_BUFFER_SIZE 16384      // 16KB for responses

// SUBSYSTEM REGISTRY /////////////////////////////////////////////////////////////////////////////

// Tracked but not Subsystems
#define SR_SERVER           "Server"
#define SR_STARTUP          "Startup"
#define SR_SHUTDOWN         "Shutdown"
#define SR_RESTART          "Restart"
#define SR_CRASH            "Crash"
#define SR_DEPCHECK         "DepCheck"
#define SR_CONFIG           "Config"
#define SR_CONFIG_CURRENT   "Config-Current"
#define SR_LAUNCH           "Launch"
#define SR_LANDING          "Landing"
#define SR_STATUS           "Status"
#define SR_QUEUES           "Queues"
#define SR_MUTEXES          "Mutexes"         

// The primary 19 Subsystems
#define SR_REGISTRY         "Registry"
#define SR_THREADS          "Threads"
#define SR_PAYLOAD          "Payload"
#define SR_API              "API"
#define SR_WEBSOCKET        "WebSocket"
#define SR_WEBSERVER        "WebServer"
#define SR_SWAGGER          "Swagger"
#define SR_MAIL_RELAY       "MailRelay"
#define SR_MDNS_CLIENT      "mDNSClient"
#define SR_MDNS_SERVER      "mDNSServer"
#define SR_TERMINAL         "Terminal"
#define SR_PRINT            "Print"
#define SR_DATABASE         "Database"
#define SR_LOGGING          "Logging"
#define SR_NETWORK          "Network"
#define SR_RESOURCES        "Resources"
#define SR_OIDC             "OIDC"
#define SR_AUTH             "Auth"
#define SR_NOTIFY           "Notify"
#define SR_MIRAGE           "Mirage"

// Additional sub-Subsystem Tracking
#define SR_WEBSOCKET_LIB    "WebSocket-Lib"     // Low-level libwebsockets diagnostics
#define SR_THREADS_LIB      "Threads-Lib"       // Tracks thread changes
#define SR_BERYLLIUM        "Beryllium"         // G-code parsing

#define INITIAL_REGISTRY_CAPACITY 20
#define MAX_DEPENDENCIES 20
#define MAX_SUBSYSTEMS 18  // Total number of subsystems (Registry, Payload, Threads, Network, Database, Logging, WebServer, API, Swagger, WebSocket, Terminal, mDNS Server, mDNS Client, Mail Relay, Print, Resources, OIDC, Notify)

// LOGGING ////////////////////////////////////////////////////////////////////////////////////////

#define NUM_PRIORITY_LEVELS 7
extern int MAX_PRIORITY_LABEL_WIDTH;   // All log level names are 5 characters
extern int MAX_SUBSYSTEM_LABEL_WIDTH;  // Default minimum width

typedef struct {
    int value;
    const char* label;
} PriorityLevel;

extern PriorityLevel DEFAULT_PRIORITY_LEVELS[];

#define LOG_LINE_BREAK "――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――"   

#define LOG_LEVEL_TRACE   0  // Log everything possible 
#define LOG_LEVEL_DEBUG   1  // Debug-level messages
#define LOG_LEVEL_STATE   2  // General information, normal operation 
#define LOG_LEVEL_ALERT   3  // Pay attention to these, not deal-breakers 
#define LOG_LEVEL_ERROR   4  // Likely need to do something here 
#define LOG_LEVEL_FATAL   5  // Can't continue 
#define LOG_LEVEL_QUIET   6  // Used primarily for logging UI work 

#define DEFAULT_CONSOLE_ENABLED  true
#define DEFAULT_FILE_ENABLED     true
#define DEFAULT_SYSLOG_ENABLED   false
#define DEFAULT_DATABASE_ENABLED false
#define DEFAULT_NOTIFY_ENABLED   false

#define DEFAULT_CONSOLE_LEVEL  0 // TRACE
#define DEFAULT_FILE_LEVEL     1 // DEBUG
#define DEFAULT_SYSLOG_LEVEL   2 // STATE
#define DEFAULT_DATABASE_LEVEL 2 // STATE
#define DEFAULT_NOTIFY_LEVEL   4 // ERROR

#define DEFAULT_LOG_ENTRY_SIZE        1024  /* Size of individual log entries */
#define DEFAULT_MAX_LOG_MESSAGE_SIZE  2048  /* Size of JSON-formatted messages */

#define LOG_BUFFER_SIZE 500
#define MAX_LOG_LINE_LENGTH DEFAULT_LOG_ENTRY_SIZE

// Global startup log level for filtering during initialization
extern int startup_log_level;

// Initialize startup log level from environment variable
void init_startup_log_level(void);

// QUEUES /////////////////////////////////////////////////////////////////////////////////////////

#define MIN_QUEUED_JOBS 1
#define MAX_QUEUED_JOBS 1000
#define MIN_CONCURRENT_JOBS 1
#define MAX_CONCURRENT_JOBS 16

// Queue size limits
#define DEFAULT_MAX_QUEUE_SIZE 10000            // 10K items
#define DEFAULT_MAX_QUEUE_MEMORY_MB 256         // 256MB default
#define DEFAULT_MAX_QUEUE_BLOCKS 1024           // 1K blocks
#define DEFAULT_QUEUE_TIMEOUT_MS 30000          // 30 seconds

// Queue validation limits
#define MIN_QUEUE_SIZE 10
#define MAX_QUEUE_SIZE 1000000
#define MIN_QUEUE_MEMORY_MB 64
#define MAX_QUEUE_MEMORY_MB 16384
#define MIN_QUEUE_BLOCKS 64
#define MAX_QUEUE_BLOCKS 16384
#define MIN_QUEUE_TIMEOUT_MS 1000
#define MAX_QUEUE_TIMEOUT_MS 300000

// Early initialization limits - subset of full configuration
#define EARLY_MAX_QUEUE_BLOCKS MIN_QUEUE_BLOCKS  // Use minimum from config
#define EARLY_BLOCK_LIMIT (MIN_QUEUE_BLOCKS / 2) // Half of minimum blocks

// PRIORITIES /////////////////////////////////////////////////////////////////////////////////////

#define MIN_PRIORITY 0
#define MAX_PRIORITY 100
#define MIN_PRIORITY_SPREAD 10  // Minimum difference between priority levels

#define DEFAULT_PRIORITY_EMERGENCY 0
#define DEFAULT_PRIORITY_DEFAULT 1
#define DEFAULT_PRIORITY_MAINTENANCE 2
#define DEFAULT_PRIORITY_SYSTEM 3

// ARCHIVE and COMPRESSION ////////////////////////////////////////////////////////////////////////

#define TAR_BLOCK_SIZE 512
#define TAR_NAME_SIZE 100
#define TAR_SIZE_OFFSET 124
#define TAR_SIZE_LENGTH 12

#define BROTLI_WINDOW_SIZE 22               // Window size for Brotli (10-24)
#define BROTLI_SMALL_THRESHOLD 5120         // 5KB
#define BROTLI_MEDIUM_THRESHOLD 512000      // 500KB
#define BROTLI_LEVEL_SMALL 11               // Highest compression for small content
#define BROTLI_LEVEL_MEDIUM 6               // Medium compression for medium content
#define BROTLI_LEVEL_LARGE 4                // Lower compression for large content

// NETWORKING ////////////////////////////////////////////////////////////////////////////////////

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#define MAC_LEN 6
#define MAX_IPS 50
#define MAX_INTERFACES 50

#ifndef NI_MAXHOST
    #define NI_MAXHOST 1025
#endif

#ifndef NI_NUMERICHOST
    #define NI_NUMERICHOST 0x02
#endif

// ICMP (Ping) ////////////////////////////////////////////////////////////////////////////////////

#define PING_TIMEOUT_SEC 1
#define PING_PACKET_SIZE 64

#ifndef ICMP_ECHO
    #define ICMP_ECHO 8
#endif

#ifndef ICMP_ECHOREPLY
    #define ICMP_ECHOREPLY 0
#endif

// WEBSERVER //////////////////////////////////////////////////////////////////////////////////////

#define MAX_ENDPOINTS 32

// WebServer Validation limits 
#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_THREAD_POOL_SIZE 1
#define MAX_THREAD_POOL_SIZE 64
#define MIN_CONNECTIONS 1
#define MAX_CONNECTIONS 10000
#define MIN_CONNECTIONS_PER_IP 1
#define MAX_CONNECTIONS_PER_IP 1000
#define MIN_CONNECTION_TIMEOUT 1
#define MAX_CONNECTION_TIMEOUT 3600

// DATABASES //////////////////////////////////////////////////////////////////////////////////////
#define MAX_DATABASES 10           
#define MAX_QUERIES_PER_REQUEST 20                                                                

// WEBSOCKET SERVER ///////////////////////////////////////////////////////////////////////////////

#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_EXIT_WAIT_SECONDS 1
#define MAX_EXIT_WAIT_SECONDS 60
#define WEBSOCKET_MIN_MESSAGE_SIZE 1024        // 1KB
#define WEBSOCKET_MAX_MESSAGE_SIZE 0x40000000  // 1GB

// SMTP ///////////////////////////////////////////////////////////////////////////////////////////

#define MAX_OUTBOUND_SERVERS 5

#define MIN_SMTP_PORT 1
#define MAX_SMTP_PORT 65535
#define MIN_SMTP_TIMEOUT 1
#define MAX_SMTP_TIMEOUT 300
#define MIN_SMTP_RETRIES 0
#define MAX_SMTP_RETRIES 10

// MDNS ///////////////////////////////////////////////////////////////////////////////////////////

#define MDNS_PORT 5353                  // Standard mDNS port is 5353
#define MDNS_GROUP_V4 "224.0.0.251"     // IPv4 multicast group
#define MDNS_GROUP_V6 "ff02::fb"        // IPv6 multicast group
#define MDNS_TTL 255                    // Default TTL for announcements

#define MDNS_TYPE_A 1                   // Host address (IPv4)
#define MDNS_TYPE_PTR 12                // Domain name pointer (service discovery)
#define MDNS_TYPE_TXT 16                // Text strings (metadata)
#define MDNS_TYPE_AAAA 28               // Host address (IPv6)
#define MDNS_TYPE_SRV 33                // Service location
#define MDNS_TYPE_ANY 255               // Request for all records

#define MDNS_CLASS_IN 1                 // Internet class records
#define MDNS_FLAG_RESPONSE 0x8400       // Response packet flag
#define MDNS_FLAG_AUTHORITATIVE 0x0400  // Authoritative answer flag
#define MDNS_MAX_PACKET_SIZE 1500       // MTU-compatible size

// OIDC ///////////////////////////////////////////////////////////////////////////////////////////

#define OIDC_PASSWORD_HASH_LENGTH 64
#define OIDC_SALT_LENGTH 32
#define OIDC_ACCESS_TOKEN_LENGTH 64
#define OIDC_REFRESH_TOKEN_LENGTH 64
#define OIDC_AUTHORIZATION_CODE_LENGTH 32
#define OIDC_KEY_ID_LENGTH 32

#define MIN_OIDC_PORT 1024
#define MAX_OIDC_PORT 65535
#define MIN_TOKEN_LIFETIME 300        // 5 minutes
#define MAX_TOKEN_LIFETIME 86400      // 24 hours
#define MIN_REFRESH_LIFETIME 3600     // 1 hour
#define MAX_REFRESH_LIFETIME 2592000  // 30 days
#define MIN_KEY_ROTATION_DAYS 1
#define MAX_KEY_ROTATION_DAYS 90

// 3D PRINTING/////////////////////////////////////////////////////////////////////////////////////

#define MIN_SPEED 0.1                   // mm/s
#define MAX_SPEED 1000.0                // mm/s
#define MIN_ACCELERATION 0.1            // mm/s^2
#define MAX_ACCELERATION 5000.0         // mm/s^2
#define MIN_JERK 0.01                   // mm/s^3
#define MAX_JERK 100.0                  // mm/s^3

#define Z_VALUES_CHUNK_SIZE 1000        // Initial allocation size for Z values array
#define DEFAULT_MAX_LAYERS 1000         // Maximum number of layers
#define MIN_LAYERS 1                    // Minimum number of layers
#define MAX_LAYERS 10000                // Absolute maximum layers

#define MAX_LINE_LENGTH 1024           // Maximum length of a line in G-code

#define DEFAULT_FEEDRATE   7500.0       // mm/min
#define DEFAULT_FILAMENT_DIAMETER 1.75  // mm
#define DEFAULT_FILAMENT_DENSITY 1.04   // g/cm^3

// MATH ///////////////////////////////////////////////////////////////////////////////////////////

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// OTHER //////////////////////////////////////////////////////////////////////////////////////////

// Fallback definition for CLOCK_MONOTONIC if not defined
#ifndef CLOCK_MONOTONIC
    #define CLOCK_MONOTONIC 1
#endif

// Constants for ID generation
#define ID_CHARS "BCDFGHKPRST"  // Consonants for readable IDs
#define ID_LEN 5                // Balance of uniqueness and readability

// UUID generation for unique filenames
#define UUID_STR_LEN 37

#endif /* GLOBALS_H */
