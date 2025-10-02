/*
 * Launch Readiness System
 * 
 * DESIGN PRINCIPLES:
 * - This file is a lightweight orchestrator only - no subsystem-specific code
 * - All subsystems are equal in importance - no hierarchy
 * - Each subsystem independently determines its own readiness
 * - Processing order is for consistency only, not priority
 * 
 * ROLE:
 * This module coordinates readiness checks by:
 * - Calling each subsystem's readiness check function
 * - Collecting results without imposing hierarchy
 * - Maintaining consistent processing order
 * 
 * Key Points:
 * - No subsystem has special status in readiness checks
 * - Each subsystem determines its own readiness criteria
 * - Order of checks is for consistency only
 * - All readiness checks are equally important
 * 
 * Implementation:
 * All subsystem-specific readiness logic belongs in respective launch_*.c
 * files (e.g., launch_network.c, launch_webserver.c), maintaining proper
 * separation of concerns.
 *
 * Note: While the registry is checked first for technical reasons,
 * this does not imply any special status or priority. All subsystems
 * are equally important to the launch process.
 */

// Global includes
#include "../hydrogen.h"
#include <dlfcn.h>
#include <link.h>
#include <limits.h>
#include <regex.h>
#include <string.h>

// Local includes
#include "launch.h"

// Helper function to compare version strings
int version_matches(const char* loaded_version, const char* expected_version) {
    if (!loaded_version || !expected_version) return 0;
    if (strcmp(loaded_version, "version-unknown") == 0) return 0;

    // Simple major.minor version comparison (ignore patch level)
    int major1, minor1, major2, minor2;
    if (sscanf(loaded_version, "%2d.%2d", &major1, &minor1) != 2) return 0;
    if (sscanf(expected_version, "%2d.%2d", &major2, &minor2) != 2) return 0;

    return (major1 == major2 && minor1 == minor2);
}

// Parse major version from version string
static int parse_major_version(const char* v) {
    int maj = -1;
    sscanf(v, "%d", &maj);
    return maj;
}

// Check if IP is RFC1918 or local
static int is_rfc1918_or_local_ip(const char* v) {
    int a, b, c, d;
    if (sscanf(v, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) return 0;
    if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255) return 0;
    if (a == 10) return 1;
    if (a == 127) return 1;
    if (a == 192 && b == 168) return 1;
    if (a == 169 && b == 254) return 1;
    if (a == 172 && (b >= 16 && b <= 31)) return 1;
    return 0;
}

// Check if version is plausible for DB2
static int is_plausible_db2_version(const char* v, int dots) {
    int maj = parse_major_version(v);
    if (maj < 8 || maj > 15) return 0;  // drop nonsense like 27.*
    if (dots < 1 || dots > 3) return 0;
    return 1;
}

// Check if DB2-related keywords are nearby
static int has_db2_keywords_nearby(const char* hay, size_t start, size_t end) {
    (void)end;  // Suppress unused parameter warning
    static const char* kws[] = {"DB2", "IBM", "Data Server", "Driver", "ODBC", "CLI", "db2"};
    size_t lo = start > 400 ? start - 400 : 0;
    for (int i = 0; i < (int)(sizeof(kws) / sizeof(kws[0])); ++i)
        if (strstr(hay + lo, kws[i])) return 1;
    return 0;
}

// Score DB2 version hit
static int score_db2_version(const char* hay, size_t start, size_t end, int dots, const char* vstr) {
    // Hard drops first
    char pre = (start > 0) ? hay[start - 1] : '\0';
    char post = hay[end];
    if (pre >= '0' && pre <= '9') return -99999;  // started mid-number
    if (!has_db2_keywords_nearby(hay, start, end)) return -99999;  // must be near DB2-ish text
    if (dots == 3 && is_rfc1918_or_local_ip(vstr)) return -99999;
    if (!is_plausible_db2_version(vstr, dots)) return -99999;

    // Base score: more dots is better
    int score = dots * 100;

    // Prefer majors we actually expect
    int maj = parse_major_version(vstr);
    if (maj == 11) score += 25;  // DB2 11.x is very common

    // Heuristic nudge: 11.1.*.* and 11.5.*.* are typical
    int minor = -1; sscanf(vstr, "%*d.%d", &minor);
    if (maj == 11 && (minor == 1 || minor == 5)) score += 15;

    // Light penalty if next/prev is URL-ish
    if (pre == '/' || pre == ':' || pre == '@' || post == '/' || post == ':') score -= 10;

    return score;
}

// Function to get version information from dynamically loaded libraries
char* get_library_version(void* handle, const char* lib_name) {
    if (!handle || !lib_name) return NULL;

    char* version_info = NULL;
    char version_str[256] = {0};

    if (strcmp(lib_name, "MySQL") == 0) {
        // MySQL: Try mysql_get_client_version function - use proper casting approach
        typedef unsigned long (*mysql_version_func)(void);
        union { void* obj_ptr; mysql_version_func func_ptr; } ptr_union;
        ptr_union.obj_ptr = dlsym(handle, "mysql_get_client_version");
        mysql_version_func mysql_get_client_version = ptr_union.func_ptr;
        if (mysql_get_client_version && ptr_union.obj_ptr != NULL) {
            unsigned long version = mysql_get_client_version();
            unsigned long major = (version / 10000UL) % 100UL;
            unsigned long minor = (version / 100UL) % 100UL;
            unsigned long patch = version % 100UL;
            snprintf(version_str, sizeof(version_str), "%lu.%lu.%lu", major, minor, patch);
        }
    }
    else if (strcmp(lib_name, "PostgreSQL") == 0) {
        // PostgreSQL: Try PQlibVersion function - use union to avoid pedantic warnings
        typedef int (*pq_version_func)(void);
        union { void* obj_ptr; pq_version_func func_ptr; } ptr_union;
        ptr_union.obj_ptr = dlsym(handle, "PQlibVersion");
        pq_version_func PQlibVersion = ptr_union.func_ptr;
        if (PQlibVersion && ptr_union.obj_ptr != NULL) {
            int version = PQlibVersion();
            int major = version / 10000;
            int minor = (version / 100) % 100;
            int patch = version % 100;
            snprintf(version_str, sizeof(version_str), "%d.%d.%d", major, minor, patch);
        }
    }
    else if (strcmp(lib_name, "SQLite") == 0) {
        // SQLite: Try sqlite3_libversion function - use union to avoid pedantic warnings
        typedef const char* (*sqlite3_version_func)(void);
        union { void* obj_ptr; sqlite3_version_func func_ptr; } ptr_union;
        ptr_union.obj_ptr = dlsym(handle, "sqlite3_libversion");
        sqlite3_version_func sqlite3_libversion = ptr_union.func_ptr;
        if (sqlite3_libversion && ptr_union.obj_ptr != NULL) {
            const char* version = sqlite3_libversion();
            if (version) {
                snprintf(version_str, sizeof(version_str), "%s", version);
            }
        }
    }
    else if (strcmp(lib_name, "DB2") == 0) {
        // DB2: Try common DB2/SqlAnywhere version functions first
        typedef const char* (*db2_version_func)(void);

        // Try various DB2 version functions
        const char* version_funcs[] = {
            "db_version", "sqlany_version", "db_info", "sqlany_build",
            "db_version_info", "sqle_client_version", NULL
        };

        for (int i = 0; version_funcs[i] != NULL; i++) {
            union { void* obj_ptr; db2_version_func func_ptr; } ptr_union;
            ptr_union.obj_ptr = dlsym(handle, version_funcs[i]);
            db2_version_func db_version = ptr_union.func_ptr;

            if (db_version && ptr_union.obj_ptr != NULL) {
                const char* version = db_version();
                if (version && strlen(version) > 0) {
                    // Extract version number from possible format variations
                    char clean_version[64] = {0};
                    if (sscanf(version, "%63[^;(),]", clean_version) > 0) {
                        snprintf(version_str, sizeof(version_str), "%s", clean_version);
                    } else {
                        snprintf(version_str, sizeof(version_str), "%s", version);
                    }
                    break;
                }
            }
        }

        // If function-based version detection failed, try file scanning
        if (strlen(version_str) == 0) {
            // Get the library file path - prefer dladdr(dlsym("SQLAllocHandle"))
            char lib_path[PATH_MAX] = {0};
            void* sym = dlsym(handle, "SQLAllocHandle");
            if (sym) {
                Dl_info di = {0};
                if (dladdr(sym, &di) && di.dli_fname && di.dli_fname[0] == '/') {
                    strncpy(lib_path, di.dli_fname, sizeof(lib_path) - 1);
                }
            }

            // Fallback to manual path finding if dladdr failed
            if (!lib_path[0]) {
                FILE* maps = fopen("/proc/self/maps", "r");
                if (maps) {
                    char line[8192];
                    while (fgets(line, sizeof(line), maps)) {
                        if (strstr(line, "libdb2.so")) {
                            char* s = strchr(line, '/');
                            if (s) {
                                size_t len = strlen(s);
                                while (len && (s[len-1] == '\n' || s[len-1] == ' ')) s[--len] = 0;
                                if (len < sizeof(lib_path)) {
                                    strcpy(lib_path, s);
                                }
                                break;
                            }
                        }
                    }
                    fclose(maps);
                }
            }

            if (lib_path[0]) {
                // Scan file for version strings
                FILE* lib_file = fopen(lib_path, "rb");
                if (lib_file) {
                    regex_t rx;
                    regcomp(&rx, "([0-9]{1,2}\\.[0-9]{1,3}(\\.[0-9]{1,3}){0,2})", REG_EXTENDED);

                    char buf[1<<16];
                    char carry[512] = {0};
                    size_t carry_len = 0;

                    char best_version[64] = {0};
                    int best_score = -1;

                    while (1) {
                        size_t n = fread(buf, 1, sizeof(buf), lib_file);
                        if (!n) break;

                        static char str[1<<18];
                        size_t k = 0;
                        if (carry_len) { memcpy(str, carry, carry_len); k = carry_len; carry_len = 0; }
                        for (size_t i = 0; i < n; ++i) {
                            unsigned char c = (unsigned char)buf[i];
                            if ((c >= 32 && c < 127) || c=='\n' || c=='\r' || c=='\t')
                                str[k++] = (char)c;
                            else if (k && str[k-1] != '\n')
                                str[k++] = '\n';
                            if (k >= sizeof(str)-2) { str[k++] = '\n'; break; }
                        }
                        str[k] = 0;

                        size_t tail = (k > sizeof(carry) ? sizeof(carry) : k);
                        if (tail) { memcpy(carry, str + k - tail, tail); carry_len = tail; }

                        const char* p = str;
                        size_t off = 0;
                        regmatch_t m[1];
                        while (!regexec(&rx, p, 1, m, 0)) {
                            size_t s, e;
                            int len;

                            // Safely convert regex offsets to size_t with bounds checking
                            int rm_so = (int)m[0].rm_so;
                            int rm_eo = (int)m[0].rm_eo;

                            // Skip invalid matches
                            if (rm_so < 0 || rm_eo < 0 || rm_so >= rm_eo) {
                                regoff_t rm_eo_val = m[0].rm_eo;
                                p += (size_t)rm_eo_val;
                                off += (size_t)rm_eo_val;
                                continue;
                            }

                            // Convert to size_t safely
                            s = (size_t)rm_so;
                            e = (size_t)rm_eo;
                            s += off;
                            e += off;
                            len = (int)(e - s);

                            if (len > 0 && len < 63) {
                                char v[64];
                                memcpy(v, p + (size_t)rm_so, (size_t)len);
                                v[len] = 0;
                                int dots = 0;
                                for (int i = 0; i < len; i++) {
                                    if (v[i] == '.') dots++;
                                }

                                // Score the hit
                                int score = score_db2_version(str, s, e, dots, v);
                                if (score > best_score) {
                                    strcpy(best_version, v);
                                    best_score = score;
                                }
                            }
                            p += (size_t)m[0].rm_eo;
                            off += (size_t)m[0].rm_eo;
                        }
                    }
                    fclose(lib_file);
                    regfree(&rx);

                    if (best_score >= 0) {
                        snprintf(version_str, sizeof(version_str), "%s", best_version);
                    }
                }
            }
        }

        if (strlen(version_str) == 0) {
            snprintf(version_str, sizeof(version_str), "version-unknown");
        }
    }

    if (strlen(version_str) > 0) {
        version_info = strdup(version_str);
    }

    return version_info;  // Caller must free this
}

// Log all messages from a readiness check
void log_readiness_messages(LaunchReadiness* readiness) {
    if (!readiness || !readiness->messages) return;

    // Log each message (first message is the subsystem name)
    for (int i = 0; readiness->messages[i] != NULL; i++) {
        int level = LOG_LEVEL_DEBUG;

        // Use appropriate log level based on the message content
        // if (strstr(readiness->messages[i], "No-Go") != NULL) {
        //     level = LOG_LEVEL_ALERT;
        // }

        // Print the message directly (formatting is already in the message)
        log_this(SR_LAUNCH, "%s", level, 1, readiness->messages[i]);
    }
}

// Clean up all messages from a readiness check
void cleanup_readiness_messages(LaunchReadiness* readiness) {
    if (!readiness || !readiness->messages) return;
    
    // Free each message
    for (int i = 0; readiness->messages[i] != NULL; i++) {
        free((void*)readiness->messages[i]);
        readiness->messages[i] = NULL;
    }
    
    // Free the messages array
    free(readiness->messages);
    readiness->messages = NULL;
}

// Helper function to process a subsystem's readiness check
void process_subsystem_readiness(ReadinessResults* results, size_t* index,
                                      const char* name, LaunchReadiness readiness) {
    // First log all messages
    log_readiness_messages(&readiness);
    
    // Record results before cleanup
    results->results[*index].subsystem = name;
    results->results[*index].ready = readiness.ready;
    
    if (readiness.ready) {
        results->total_ready++;
        results->any_ready = true;
    } else {
        results->total_not_ready++;
    }
    results->total_checked++;
    
    // Then clean up all messages
    cleanup_readiness_messages(&readiness);
    
    (*index)++;
}

/*
 * Coordinate readiness checks for all subsystems.
 * Each subsystem's specific readiness logic lives in its own launch_*.c file.
 */
ReadinessResults handle_readiness_checks(void) {
    ReadinessResults results = {0};
    size_t index = 0;
    
    // Begin LAUNCH READINESS logging section
    log_this(SR_LAUNCH, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_LAUNCH, "LAUNCH READINESS", LOG_LEVEL_DEBUG, 0);

    process_subsystem_readiness(&results, &index, SR_REGISTRY, check_registry_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_PAYLOAD, check_payload_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_THREADS, check_threads_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_NETWORK, check_network_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_DATABASE, check_database_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_LOGGING, check_logging_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_WEBSERVER, check_webserver_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_API, check_api_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_SWAGGER, check_swagger_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_WEBSOCKET, check_websocket_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_TERMINAL, check_terminal_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_MDNS_SERVER, check_mdns_server_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_MDNS_CLIENT, check_mdns_client_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_MAIL_RELAY, check_mail_relay_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_PRINT, check_print_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_RESOURCES, check_resources_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_OIDC, check_oidc_launch_readiness());
    process_subsystem_readiness(&results, &index, SR_NOTIFY, check_notify_launch_readiness());
    
    log_this(SR_LAUNCH, "LAUNCH READINESS COMPLETE", LOG_LEVEL_DEBUG, 0);

    return results;
}
