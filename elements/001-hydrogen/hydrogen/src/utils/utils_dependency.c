/*
 * Simplified library dependency checking utilities
 */
// Global includes
#include <src/hydrogen.h>

// Local includes
#include "utils_dependency.h"

extern const char *jansson_version_str(void);

// Lua includes for version checking
#include <lua.h>

// Thread includes for parallel database checks
#include <pthread.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdlib.h>

// Database dependency configuration
typedef struct {
    const char *name;
    const char *command;
    const char *expected;
    bool required;
} DatabaseDependencyConfig;

// Thread data for parallel database checks
typedef struct {
    const DatabaseDependencyConfig *config;
    char buffer[256];
    const char *found;
    LibraryStatus status;
} ThreadData;

 typedef struct {
     const char *name;
     const char **paths;
     const char **version_funcs;
     const char *expected;
     bool is_core;
     bool required;
 } LibConfig;
 
 static const char *pthread_paths[] = {"libpthread.so", "/lib64/libpthread.so.0", "/usr/lib/libpthread.so", NULL};
 static const char *jansson_paths[] = {"libjansson.so", "/lib64/libjansson.so.4", "/usr/lib/libjansson.so", "/usr/lib/x86_64-linux-gnu/libjansson.so.4", NULL};
 static const char *microhttpd_paths[] = {"libmicrohttpd.so", "/lib64/libmicrohttpd.so.12", "/usr/lib/libmicrohttpd.so", "/usr/lib/x86_64-linux-gnu/libmicrohttpd.so.12", NULL};
 static const char *libm_paths[] = {"libm.so", "/lib64/libm.so.6", "/usr/lib/libm.so", NULL};
 static const char *libwebsockets_paths[] = {"libwebsockets.so", "/lib64/libwebsockets.so.19", "/usr/lib/libwebsockets.so", "/usr/lib/x86_64-linux-gnu/libwebsockets.so.19", NULL};
 static const char *openssl_paths[] = {"libssl.so", "/lib64/libssl.so.3", "/usr/lib/libssl.so", "/usr/lib/x86_64-linux-gnu/libssl.so.3", NULL};
 static const char *brotli_paths[] = {"libbrotlidec.so", "/lib64/libbrotlidec.so.1", "/usr/lib/libbrotlidec.so", "/usr/lib/x86_64-linux-gnu/libbrotlidec.so.1", NULL};
 static const char *libtar_paths[] = {"libtar.so", "/usr/lib64/libtar.so.1", "/usr/lib64/libtar.so", "/lib64/libtar.so.1", "/usr/lib/libtar.so", "/usr/lib/x86_64-linux-gnu/libtar.so", NULL};
 static const char *lua_paths[] = {"liblua.so", "/lib64/liblua.so.5.4", "/usr/lib/liblua.so", "/usr/lib/x86_64-linux-gnu/liblua.so.5.4", NULL};
 
 static const char *jansson_funcs[] = {"jansson_version_str", NULL};
 static const char *microhttpd_funcs[] = {"MHD_get_version", NULL};
 static const char *libwebsockets_funcs[] = {"lws_get_library_version", NULL};
 static const char *openssl_funcs[] = {"OpenSSL_version", "SSLeay_version", NULL};
 static const char *brotli_funcs[] = {"BrotliDecoderVersion", NULL};
 static const char *libtar_funcs[] = {"libtar_version", NULL};
 static const char *lua_funcs[] = {NULL};

// Database configurations
static const DatabaseDependencyConfig db_configs[] = {
    {"DB2", "db2level", "11.1.3.3", false},
    {"PostgreSQL", "pg_config --version", "17.6", false},
    {"MySQL", "mysql_config --version", "8.0.42", false},
    {"SQLite", "sqlite3 --version", "3.46.1", false}
};

// File-based cache for database version results (valid for 7 days)
#define CACHE_TIMEOUT_SECONDS 604800
#define CACHE_DIR "~/.cache/hydrogen/dependency"

static int cache_hits = 0;  // Track cache hits across all database checks

// Get the cache file path for a specific database
char* get_cache_file_path(const char *db_name) {
    const struct passwd *pw = getpwuid(getuid());
    if (!pw) return NULL;

    const char *home = pw->pw_dir;
    char *cache_path = malloc(strlen(home) + strlen("/.cache/hydrogen/dependency/") + strlen(db_name) + 1);
    if (!cache_path) return NULL;

    sprintf(cache_path, "%s/.cache/hydrogen/dependency/%s", home, db_name);
    return cache_path;
}

// Ensure cache directory exists
bool ensure_cache_dir(void) {
    const struct passwd *pw = getpwuid(getuid());
    if (!pw) return false;

    const char *home = pw->pw_dir;
    char cache_dir[PATH_MAX];

    // Create ~/.cache if it doesn't exist
    snprintf(cache_dir, sizeof(cache_dir), "%s/.cache", home);
    struct stat st;
    if (stat(cache_dir, &st) != 0) {
        if (mkdir(cache_dir, 0755) != 0) return false;
    }

    // Create ~/.cache/hydrogen if it doesn't exist
    snprintf(cache_dir, sizeof(cache_dir), "%s/.cache/hydrogen", home);
    if (stat(cache_dir, &st) != 0) {
        if (mkdir(cache_dir, 0755) != 0) return false;
    }

    // Create ~/.cache/hydrogen/dependency if it doesn't exist
    snprintf(cache_dir, sizeof(cache_dir), "%s/.cache/hydrogen/dependency", home);
    if (stat(cache_dir, &st) != 0) {
        if (mkdir(cache_dir, 0755) != 0) return false;
    }

    return true;
}

// Load cached version for a specific database
const char* load_cached_version(const char *db_name, char *buffer, size_t size) {
    char *cache_path = get_cache_file_path(db_name);
    if (!cache_path) return NULL;

    FILE *fp = fopen(cache_path, "r");
    if (!fp) {
        free(cache_path);
        return NULL;
    }

    // Read version and timestamp from file
    char version[256];
    time_t timestamp;
    if (fscanf(fp, "%255s %ld", version, &timestamp) == 2) {
        time_t now = time(NULL);
        if (now - timestamp < CACHE_TIMEOUT_SECONDS) {
            // Cache is valid
            strncpy(buffer, version, size - 1);
            buffer[size - 1] = '\0';
            fclose(fp);
            free(cache_path);
            return buffer;
        }
    }

    fclose(fp);
    free(cache_path);
    return NULL;
}

// Save cache to file for a specific database
void save_cache(const char *db_name, const char *version) {
    if (!ensure_cache_dir()) return;

    char *cache_path = get_cache_file_path(db_name);
    if (!cache_path) return;

    FILE *fp = fopen(cache_path, "w");
    if (fp) {
        time_t now = time(NULL);
        fprintf(fp, "%s %ld\n", version, now);
        fclose(fp);
    }

    free(cache_path);
}

 static const LibConfig lib_configs[] = {
     {"pthreads", pthread_paths, NULL, "1.0", true, true},
     {"jansson", jansson_paths, jansson_funcs, "2.13.1", false, true},
     {"microhttpd", microhttpd_paths, microhttpd_funcs, "1.0.2", false, false},
     {"libm", libm_paths, NULL, "2.0", true, true},
     {"libwebsockets", libwebsockets_paths, libwebsockets_funcs, "4.3.3", false, false},
     {"OpenSSL", openssl_paths, openssl_funcs, "3.2.4", false, false},
     {"libbrotlidec", brotli_paths, brotli_funcs, "1.1.0", false, false},
     {"libtar", libtar_paths, libtar_funcs, "1.2.20", false, false},
     {"lua", lua_paths, lua_funcs, "5.4", false, false}
 };
 
 static const char *get_status_string(LibraryStatus status) {
     switch (status) {
         case LIB_STATUS_GOOD: return "Good";
         case LIB_STATUS_WARNING: return "Less Good";
         case LIB_STATUS_CRITICAL: return "Trouble awaits";
         case LIB_STATUS_UNKNOWN: return "Unknown";
     }
     return "Unknown";
 }
 
 static LibraryStatus determine_status(const char *expected, const char *found, bool required) {
     if (!found || !strcmp(found, "None")) return required ? LIB_STATUS_CRITICAL : LIB_STATUS_WARNING;
     if (!strcmp(found, "NoVersionFound")) return required ? LIB_STATUS_WARNING : LIB_STATUS_GOOD;
     if (!expected || strstr(found, expected) != NULL) return LIB_STATUS_GOOD;
     return LIB_STATUS_WARNING;
 }
 
 static void log_status(const char *name, const char *expected, const char *found, const char *method, LibraryStatus status) {
     int level = (status == LIB_STATUS_GOOD) ? LOG_LEVEL_DEBUG :
                 (status == LIB_STATUS_WARNING) ? LOG_LEVEL_DEBUG :
                 (status == LIB_STATUS_CRITICAL) ? LOG_LEVEL_FATAL : LOG_LEVEL_ERROR;
     log_this(SR_DEPCHECK, "― %s. Expecting: %s Found: %s (%s) Status: %s", level, 5,
        name, 
        expected ? expected : "(default)", 
        found ? found : "None",
        method, 
        get_status_string(status));
 }
 
 static const char *parse_db2_version(const char *output, char *buffer, size_t size) {
     // Parse DB2 output: Look for pattern in "DB2 v11.1.3.3"
     char *version_start = (char *)strstr(output, "DB2 v");
     if (version_start) {
         version_start += 5; // Skip "DB2 v"
         size_t i = 0;
         // Copy until we hit space, newline, carriage return, quote, or comma
         while (i < size - 1 && version_start[i] &&
                version_start[i] != ' ' && version_start[i] != '\n' &&
                version_start[i] != '\r' && version_start[i] != '"' &&
                version_start[i] != ',') {
             buffer[i] = version_start[i];
             i++;
         }
         buffer[i] = '\0';
         // Remove any trailing quotes or commas that might be copied
         while (i > 0 && (buffer[i-1] == '"' || buffer[i-1] == ',')) {
             buffer[--i] = '\0';
         }
         return buffer;
     }
     return "None";
 }

 static const char *parse_postgresql_version(const char *output, char *buffer, size_t size) {
     // Parse PostgreSQL output: "PostgreSQL 17.6"
     char *version_start = (char *)strstr(output, "PostgreSQL ");
     if (version_start) {
         version_start += 11; // Skip "PostgreSQL "
         size_t i = 0;
         while (i < size - 1 && version_start[i] && version_start[i] != ' ' &&
                version_start[i] != '\n' && version_start[i] != '\r') {
             buffer[i] = version_start[i];
             i++;
         }
         buffer[i] = '\0';
         return buffer;
     }
     return "None";
 }

 static const char *parse_mysql_version(const char *output, char *buffer, size_t size) {
     // Parse MySQL output: "8.0.42"
     // MySQL output is just the version number, no prefix
     size_t i = 0;
     while (i < size - 1 && output[i] && output[i] != ' ' &&
            output[i] != '\n' && output[i] != '\r') {
         buffer[i] = ((char *)output)[i];
         i++;
     }
     buffer[i] = '\0';
     return buffer;
 }

 static const char *parse_sqlite_version(const char *output, char *buffer, size_t size) {
     // Parse SQLite output: "3.46.1 2024-08-13 09:16:08..."
     // Take only the version number before the first space
     size_t i = 0;
     while (i < size - 1 && output[i] && output[i] != ' ' &&
            output[i] != '\n' && output[i] != '\r') {
         buffer[i] = ((char *)output)[i];
         i++;
     }
     buffer[i] = '\0';
     return buffer;
 }

 static const char *get_database_version(const DatabaseDependencyConfig *config, char *buffer, size_t size) {
     if (!config || !buffer || !size) return "None";

     // Check for environment variable to disable cache (for testing)
     const char *disable_cache = getenv("HYDROGEN_DEP_CACHE");
     if (!disable_cache || strcmp(disable_cache, "1") != 0) {
         // Check cache first
         const char *cached_result = load_cached_version(config->name, buffer, size);
         if (cached_result) {
             cache_hits++;
             return cached_result;
         }
     }

     buffer[0] = '\0';

     // Add timeout to prevent slow commands from blocking startup
     FILE *fp = popen(config->command, "r");
     if (!fp) return "None";

     // Set up timeout for the command (30 seconds max)
     struct timeval timeout;
     timeout.tv_sec = 30;
     timeout.tv_usec = 0;

     fd_set readfds;
     FD_ZERO(&readfds);
     int fd = fileno(fp);
     FD_SET(fd, &readfds);

     char output[1024];
     size_t bytes_read = 0;

     // Read with timeout
     if (select(fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
         bytes_read = fread(output, 1, sizeof(output) - 1, fp);
     }
     output[bytes_read] = '\0';
     pclose(fp);

     if (bytes_read == 0) return "None";

     // Parse based on database type
     const char *result;
     if (strcmp(config->name, "DB2") == 0) {
         result = parse_db2_version(output, buffer, size);
     } else if (strcmp(config->name, "PostgreSQL") == 0) {
         result = parse_postgresql_version(output, buffer, size);
     } else if (strcmp(config->name, "MySQL") == 0) {
         result = parse_mysql_version(output, buffer, size);
     } else if (strcmp(config->name, "SQLite") == 0) {
         result = parse_sqlite_version(output, buffer, size);
     } else {
         return "None";
     }

     // Cache the result if we got a valid response
     if (strcmp(result, "None") != 0) {
         save_cache(config->name, result);
     }

     return result;
 }

 // Thread function for parallel database version checking
 static void *check_database_thread(void *arg) {
     ThreadData *data = (ThreadData *)arg;
     data->found = get_database_version(data->config, data->buffer, sizeof(data->buffer));
     data->status = determine_status(data->config->expected, data->found, data->config->required);
     return NULL;
 }

 static const char *get_version(const LibConfig *config, char *buffer, size_t size, const char **method) {
     if (!config || !buffer || !size || !method) return "None";
     buffer[0] = '\0';
     *method = "N/A";
 
     if (config->is_core) {
         *method = "COR";
         return config->expected;
     }

     if (strcmp(config->name, "lua") == 0) {
         // Use compile-time LUA_VERSION macro for Lua version
         const char *lua_ver = LUA_VERSION;
         if (lua_ver) {
             // LUA_VERSION is like "Lua 5.4", extract the version part
             const char *version_start = strstr(lua_ver, "Lua ");
             if (version_start) {
                 version_start += 4; // Skip "Lua "
                 size_t len = strnlen(version_start, size - 1);
                 if (len > 0 && len < size - 1) {
                     strncpy(buffer, version_start, len);
                     buffer[len] = '\0';
                     *method = "HDR";
                     return buffer;
                 }
             } else {
                 // If it doesn't start with "Lua ", use the whole string
                 size_t len = strnlen(lua_ver, size - 1);
                 if (len > 0 && len < size - 1) {
                     strncpy(buffer, lua_ver, len);
                     buffer[len] = '\0';
                     *method = "HDR";
                     return buffer;
                 }
             }
         }
         return "NoVersionFound";
     }
 
     for (const char **path = config->paths; *path; path++) {
         void *handle = dlopen(*path, RTLD_LAZY);
         if (!handle) {
             log_this(SR_DEPCHECK, "― %s. Failed to open at %s: %s", LOG_LEVEL_TRACE, 3, config->name, *path, dlerror());
             continue;
         }
 
         const char *version = "NoVersionFound";
         *method = "DLO";
 
         if (config->version_funcs) {
             for (const char **func_name = config->version_funcs; *func_name; func_name++) {
                 if (strcmp(config->name, "jansson") == 0 && strcmp(*func_name, "jansson_version_str") == 0) {
                     const char *temp = jansson_version_str();
                     if (temp) {
                         size_t len = strnlen(temp, size - 1);
                         if (len > 0 && len < size - 1) {
                             strncpy(buffer, temp, len);
                             buffer[len] = '\0';
                             version = buffer;
                             *method = "API";
                             log_this(SR_DEPCHECK, "― %s. Found: %s via direct call", LOG_LEVEL_TRACE, 2, config->name, version);
                             dlclose(handle);
                             return version;
                         }
                     }
                     continue;
                 }
 
                 dlerror();
                 void *func_ptr = dlsym(handle, *func_name);
                 const char *err = dlerror();
                 if (err || !func_ptr) {
                     log_this(SR_DEPCHECK, "― %s. dlsym(%s) failed: %s", LOG_LEVEL_TRACE, 2, config->name, *func_name, err ? err : "NULL");
                     continue;
                 }
 
                 if (strcmp(config->name, "libtar") == 0 && strcmp(*func_name, "libtar_version") == 0) {
                     const char *version_str = (const char *)func_ptr;
                     log_this(SR_DEPCHECK, "― %s.: Raw version_str at %p", LOG_LEVEL_TRACE, 2, config->name, func_ptr);
                     volatile char probe = *version_str; // Force read to catch segfault
                     if (probe != '\0') {
                         size_t len = strnlen(version_str, size - 1);
                         log_this(SR_DEPCHECK, "― %s. Version string length: %zu", LOG_LEVEL_TRACE, 2, config->name, len);
                         if (len > 0 && len < size - 1) {
                             strncpy(buffer, version_str, len);
                             buffer[len] = '\0';
                             version = buffer;
                             *method = "SYM";
                             log_this(SR_DEPCHECK, "― %s. Found: %s via %s (data symbol)", LOG_LEVEL_TRACE, 3, config->name, version, *func_name);
                             dlclose(handle);
                             return version;
                         }
                     }
                     version = "NoVersionFound";
                     log_this(SR_DEPCHECK, "― %s. Problem: %s is empty or inaccessible", LOG_LEVEL_TRACE, 2, config->name, *func_name);
                     dlclose(handle);
                     return version;
                 }

 
                 if (strcmp(config->name, "libbrotlidec") == 0 && strcmp(*func_name, "BrotliDecoderVersion") == 0) {
                     uint32_t (*brotli_func)(void);
                     *(void **)(&brotli_func) = func_ptr;
                     uint32_t ver = brotli_func();
                     snprintf(buffer, size, "%u.%u.%u",
                              ver >> 24, (ver >> 12) & 0xFFF, ver & 0xFFF);
                     version = buffer;
                     *method = "SYM";
                     log_this(SR_DEPCHECK, "― %s. Found: %s via %s", LOG_LEVEL_TRACE, 3, config->name, version, *func_name);
                     dlclose(handle);
                     return version;
                 }

 

                 union {
                     const char *(*void_func)(void);
                     const char *(*int_func)(int);
                     void *ptr;
                 } func;
                 func.ptr = func_ptr;

                 const char *temp = NULL;
                 if (strcmp(config->name, "OpenSSL") == 0) {
                     temp = func.int_func(0);
                 } else {
                     temp = func.void_func();
                 }
                 if (!temp) {
                     log_this(SR_DEPCHECK, "― %s. Problem: Function %s returned NULL", LOG_LEVEL_TRACE, 2, config->name, *func_name);
                     continue;
                 }

                 size_t len = strnlen(temp, size - 1);
                 if (len > 0 && len < size - 1) {
                     bool valid = true;
                     for (size_t i = 0; i < len; i++) {
                         volatile char probe = temp[i];
                         if (probe < 32 || probe > 126) {
                             valid = false;
                             break;
                         }
                     }
                     if (valid) {
                         strncpy(buffer, temp, len);
                         buffer[len] = '\0';
                         version = buffer;
                         *method = "SYM";
                         log_this(SR_DEPCHECK, "― %s. Found: %s via %s", LOG_LEVEL_TRACE, 3, config->name, version, *func_name);
                         dlclose(handle);
                         return version;
                     }
                 }
                 version = "NoVersionFound";
             }
         }
         dlclose(handle);
         return version;
     }
     if (strcmp(config->name, "libtar") == 0) {
        log_this(SR_DEPCHECK, "― libtar not found; installed at /usr/lib64/libtar.so.1? Run 'ldd ./hydrogen' and 'sudo ldconfig'", LOG_LEVEL_TRACE, 0);
     }
     return "None";
 }
 
 bool is_library_available(const char *lib_name) {
     if (!lib_name) return false;
     void *handle = dlopen(lib_name, RTLD_LAZY);
     if (handle) {
         dlclose(handle);
         return true;
     }
     return false;
 }
 
 void check_library_dependency(const char *name, const char *expected_with_v, bool is_required) {
     if (!name) return;
     const char *expected = expected_with_v && expected_with_v[0] == 'v' ? expected_with_v + 1 : expected_with_v;
 
     for (size_t i = 0; i < sizeof(lib_configs) / sizeof(lib_configs[0]); i++) {
         if (strcmp(lib_configs[i].name, name)) continue;
         char buffer[256];
         const char *method;
         const char *found = get_version(&lib_configs[i], buffer, sizeof(buffer), &method);
         LibraryStatus status = lib_configs[i].is_core ? LIB_STATUS_GOOD :
                                determine_status(expected, found, is_required);
         log_status(name, expected, found, method, status);
         return;
     }
     log_status(name, expected, "None", "N/A", is_required ? LIB_STATUS_CRITICAL : LIB_STATUS_WARNING);
 }
 
 int check_library_dependencies(const AppConfig *config) {
     // Start precision timing
     struct timeval depcheck_start, depcheck_end;
     gettimeofday(&depcheck_start, NULL);


     log_this(SR_DEPCHECK, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
     log_this(SR_DEPCHECK, "DEPENDENCY CHECKS", LOG_LEVEL_DEBUG, 0);
     int critical_count = 0;
 
     bool web = config ? (config->webserver.enable_ipv4 || config->webserver.enable_ipv6) : false;
     bool ws = config ? (config->websocket.enable_ipv4 || config->websocket.enable_ipv6) : false;
     bool sec = config ? (web || ws) : false;
     bool print = config ? config->print.enabled : false;
 
     for (size_t i = 0; i < sizeof(lib_configs) / sizeof(lib_configs[0]); i++) {
         LibConfig lib = lib_configs[i];
         lib.required = lib.is_core || (strcmp(lib.name, "microhttpd") == 0 && web) ||
                        (strcmp(lib.name, "libbrotlidec") == 0 && web) ||
                        (strcmp(lib.name, "libwebsockets") == 0 && ws) ||
                        (strcmp(lib.name, "OpenSSL") == 0 && sec) ||
                        (strcmp(lib.name, "libtar") == 0 && print);
 
         char buffer[256];
         const char *method;
         const char *found = get_version(&lib, buffer, sizeof(buffer), &method);
         LibraryStatus status = lib.is_core ? LIB_STATUS_GOOD : determine_status(lib.expected, found, lib.required);
         log_status(lib.name, lib.expected, found, method, status);
         if (status == LIB_STATUS_CRITICAL && lib.required) critical_count++;
     }

     // Check database dependencies in parallel using threads
     struct timeval db_start, db_end;
     gettimeofday(&db_start, NULL);
     cache_hits = 0;  // Reset counter for this run

     size_t db_count = sizeof(db_configs) / sizeof(db_configs[0]);
     ThreadData thread_data[db_count];
     pthread_t threads[db_count];

     // Create threads for each database check
     for (size_t i = 0; i < db_count; i++) {
         thread_data[i].config = &db_configs[i];
         thread_data[i].buffer[0] = '\0';
         thread_data[i].found = NULL;
         thread_data[i].status = LIB_STATUS_UNKNOWN;
         if (pthread_create(&threads[i], NULL, check_database_thread, &thread_data[i]) != 0) {
             // Fallback to sequential if thread creation fails
             thread_data[i].found = get_database_version(thread_data[i].config, thread_data[i].buffer, sizeof(thread_data[i].buffer));
             thread_data[i].status = determine_status(thread_data[i].config->expected, thread_data[i].found, thread_data[i].config->required);
         }
     }

     // Join threads and log results
     for (size_t i = 0; i < db_count; i++) {
         if (pthread_join(threads[i], NULL) == 0) {
             // Thread completed successfully
         } else {
             // If join fails, data might already be set from fallback
         }
         log_status(thread_data[i].config->name, thread_data[i].config->expected, thread_data[i].found, "CMD", thread_data[i].status);
         if (thread_data[i].status == LIB_STATUS_CRITICAL && thread_data[i].config->required) critical_count++;
     }

     // Calculate database check timing
     gettimeofday(&db_end, NULL);
     double db_time = (double)(db_end.tv_sec - db_start.tv_sec) + (double)(db_end.tv_usec - db_start.tv_usec) / 1000000.0;

     // Calculate total dependency check timing
     gettimeofday(&depcheck_end, NULL);
     double total_time = (double)(depcheck_end.tv_sec - depcheck_start.tv_sec) + (double)(depcheck_end.tv_usec - depcheck_start.tv_usec) / 1000000.0;

     log_this(SR_DEPCHECK, "Timing Checks: %.3fs (%d/%d cached), Total: %.3fs", LOG_LEVEL_DEBUG, 4, db_time, cache_hits, db_count, total_time);
     log_this(SR_DEPCHECK, "Critical Issues: %d", LOG_LEVEL_DEBUG, 1, critical_count);
     log_this(SR_DEPCHECK, "DEPENDENCY CHECKS COMPLETE", LOG_LEVEL_DEBUG, 0);
     return critical_count;
 }
 
 LibraryHandle *load_library(const char *lib_name, int dlopen_flags) {
     if (!lib_name) return NULL;
     LibraryHandle *handle = calloc(1, sizeof(LibraryHandle));
     if (!handle) return NULL;
 
     handle->name = strdup(lib_name);
     handle->handle = dlopen(lib_name, dlopen_flags);
     handle->is_loaded = handle->handle != NULL;
     handle->status = handle->is_loaded ? LIB_STATUS_GOOD : LIB_STATUS_WARNING;
     handle->version = strdup(handle->is_loaded ? "Unknown" : "None");
     return handle;
 }
 
 bool unload_library(LibraryHandle *handle) {
     if (!handle) return false;
     bool success = true;
     if (handle->is_loaded && dlclose(handle->handle) != 0) success = false;
     free((void*)handle->name);
     free((void*)handle->version);
     free(handle);
     return success;
 }
 
 void *get_library_function(LibraryHandle *handle, const char *function_name) {
     if (!handle || !handle->is_loaded || !function_name) return NULL;
     dlerror();
     void *func = dlsym(handle->handle, function_name);
     return dlerror() ? NULL : func;
 }
