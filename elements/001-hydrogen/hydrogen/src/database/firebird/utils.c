/*
 * Firebird Database Engine - Utility Functions Implementation
 *
 * Implements Firebird utility functions: connection / DPB string building,
 * URL validation, isc_status extraction, and string escaping.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>

#include "types.h"
#include "connection.h"
#include "utils.h"

/*
 * Build the Firebird DBP (Database Parameter Buffer) for isc_attach_database.
 *
 * A binary DPB has the format:
 *   [isc_dpb_version1] [dpb_key] [length] [data] ... (repeating key/length/data)
 *
 * The dbname (database path) is NOT part of the DPB — it is passed as the
 * 3rd argument to isc_attach_database (the "dbname" positional argument).
 *
 * Returns a malloc'd binary buffer the caller must free (or NULL on failure).
 */
char* firebird_build_attach_string(const ConnectionConfig* config, size_t* out_dpb_len) {
    if (!config) {
        return NULL;
    }

    /*
     * Estimate buffer size: version byte + overhead per entry.
     * Each entry: 1 (key) + 1 (length) + data. User/password up to 255 each.
     */
    size_t bufsize = 512;
    char* buf = calloc(1, bufsize);
    if (!buf) {
        return NULL;
    }

    size_t used = 0;

    // DPB starts with version byte
    buf[used++] = (char)FB_DPB_VERSION1;

    // Helper macro to append a DPB entry: key, length, data
    #define DPB_APPEND(key, data, data_len) do { \
        if (used + 2 + (data_len) > bufsize) { \
            size_t newsize = used + 2 + (data_len) + 256; \
            char* tmp = realloc(buf, newsize); \
            if (!tmp) { free(buf); return NULL; } \
            buf = tmp; \
            bufsize = newsize; \
        } \
        buf[used++] = (char)(key); \
        buf[used++] = (char)(data_len); \
        memcpy(buf + used, (data), (data_len)); \
        used += (data_len); \
    } while(0)

    if (config->username && *config->username) {
        size_t ulen = strlen(config->username);
        if (ulen > 255) ulen = 255;
        DPB_APPEND(FB_DPB_USER_NAME, config->username, ulen);
    }

    if (config->password && *config->password) {
        size_t plen = strlen(config->password);
        if (plen > 255) plen = 255;
        DPB_APPEND(FB_DPB_PASSWORD, config->password, plen);
    }

    #undef DPB_APPEND

    if (out_dpb_len) {
        *out_dpb_len = used;
    }

    return buf;
}

/*
 * Backwards-compatible wrapper: returns the firebird:// URL as a text
 * connection string suitable for engine detection and config parsing.
 * The binary DPB is built separately by firebird_build_attach_string
 * at connect time (see connection.c).
 */
char* firebird_get_connection_string(const ConnectionConfig* config) {
    if (!config) return NULL;

    // The Database field in JSON configs may include the firebird:// prefix.
    // Strip it so we can rebuild the URL from host/port/path components below,
    // matching how PostgreSQL/MySQL/DB2 build their connection strings from
    // separate config fields. Leading slashes left after stripping (e.g.
    // firebird:///path → ///path) are also trimmed so the URL is well-formed.
    const char* db_field = config->database;
    char* stripped = NULL;
    if (db_field && strncmp(db_field, "firebird://", 11) == 0) {
        stripped = strdup(db_field + 11);
        // Trim leading slashes so firebird:///path becomes /path
        while (stripped && *stripped == '/') {
            memmove(stripped, stripped + 1, strlen(stripped));
        }
        db_field = stripped;
    }

    size_t bufsize = 4096;
    char* url = calloc(1, bufsize);
    if (!url) { free(stripped); return NULL; }

    size_t used = 0;
    int n = snprintf(url + used, bufsize - used, "firebird://");
    if (n < 0 || (size_t)n >= bufsize - used) { free(stripped); free(url); return NULL; }
    used += (size_t)n;

    if (config->host && *config->host) {
        n = snprintf(url + used, bufsize - used, "%s", config->host);
        if (n < 0 || (size_t)n >= bufsize - used) { free(stripped); free(url); return NULL; }
        used += (size_t)n;

        if (config->port > 0) {
            n = snprintf(url + used, bufsize - used, ":%d", config->port);
            if (n < 0 || (size_t)n >= bufsize - used) { free(stripped); free(url); return NULL; }
            used += (size_t)n;
        }

        // db_field may already start with '/' (absolute path), so don't prepend another
        const char* path_sep = (db_field && *db_field == '/') ? "" : "/";
        n = snprintf(url + used, bufsize - used, "%s%s", path_sep, db_field ? db_field : "");
        if (n < 0 || (size_t)n >= bufsize - used) { free(stripped); free(url); return NULL; }
        used += (size_t)n;

        if (config->username || config->password) {
            n = snprintf(url + used, bufsize - used, "?user=%s", config->username ? config->username : "");
            if (n < 0 || (size_t)n >= bufsize - used) { free(stripped); free(url); return NULL; }
            used += (size_t)n;
            if (config->password) {
                n = snprintf(url + used, bufsize - used, "&password=%s", config->password);
                if (n < 0 || (size_t)n >= bufsize - used) { free(stripped); free(url); return NULL; }
                used += (size_t)n;
            }
        }
    } else if (db_field) {
        n = snprintf(url + used, bufsize - used, "%s", db_field);
        if (n < 0 || (size_t)n >= bufsize - used) { free(stripped); free(url); return NULL; }
        used += (size_t)n;
    }

    url[used] = '\0';
    free(stripped);
    return url;
}

/*
 * Validate a Firebird connection string / URL.
 * Accepts:
 *   - firebird://HOST:PORT/PATH     (SuperServer)
 *   - firebird://HOST/PATH          (SuperServer, port 3050 default)
 *   - firebird://PATH               (embedded, path starts with / or bare filename)
 */
bool firebird_validate_connection_string(const char* connection_string) {
    if (!connection_string || *connection_string == '\0') {
        return false;
    }

    size_t len = strlen(connection_string);
    if (len > 4096) {
        return false;
    }

    if (strncmp(connection_string, "firebird://", 11) == 0) {
        const char* after_proto = connection_string + 11;
        return (*after_proto != '\0');
    }

    return false;
}

/*
 * Firebird string escaping: double up single quotes (SQL standard).
 * Firebird does not treat backslash as an escape character by default.
 */
char* firebird_escape_string(const DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return NULL;
    }

    size_t input_len = strlen(input);
    size_t escaped_len = input_len * 2 + 1;
    char* escaped = calloc(1, escaped_len);
    if (!escaped) {
        return NULL;
    }

    const char* src = input;
    char* dst = escaped;
    while (*src) {
        if (*src == '\'') {
            *dst++ = '\'';
            *dst++ = '\'';
        } else {
            *dst++ = *src;
        }
        src++;
    }

    return escaped;
}

/*
 * Parse a firebird:// connection-string URL into its components.
 *
 * URL formats:
 *   firebird://HOST:PORT/PATH        — SuperServer
 *   firebird://HOST/PATH             — SuperServer, no explicit port
 *   firebird://PATH                  — embedded (path starts with '/' or is a
 *                                     bare filename with no '/' after the host)
 *
 * Query parameters (?user=…&password=…&schema=…) are parsed into the
 * user_out / pass_out / schema_out buffers.
 *
 * All output buffers must be non-NULL with a positive size. They are
 * always null-terminated. Returns true on parse success.
 */
bool firebird_parse_connstring_url(const char* conn_str,
                                    char* host_out, int host_out_sz,
                                    char* port_out, int port_out_sz,
                                    char* path_out, int path_out_sz,
                                    char* user_out, int user_out_sz,
                                    char* pass_out, int pass_out_sz,
                                    char* schema_out, int schema_out_sz) {
    if (!conn_str || !host_out || !path_out ||
        host_out_sz <= 0 || path_out_sz <= 0) {
        return false;
    }

    // Initialize all outputs to empty
    if (port_out && port_out_sz > 0)  port_out[0]   = '\0';
    if (user_out && user_out_sz > 0)  user_out[0]   = '\0';
    if (pass_out && pass_out_sz > 0)  pass_out[0]   = '\0';
    if (schema_out && schema_out_sz > 0) schema_out[0] = '\0';
    host_out[0]  = '\0';
    path_out[0]  = '\0';

    if (strncmp(conn_str, "firebird://", 11) != 0) {
        return false;
    }

    const char* p = conn_str + 11;  // skip "firebird://"

    // Split path portion from query string
    const char* query_start = strchr(p, '?');
    const char* path_end = query_start ? query_start : p + strlen(p);
    size_t path_part_len = (size_t)(path_end - p);

    // Copy the authority+path portion
    if (path_part_len >= (size_t)path_out_sz) {
        path_part_len = (size_t)path_out_sz - 1;
    }
    memcpy(path_out, p, path_part_len);
    path_out[path_part_len] = '\0';

    // Parse query string for user/password/schema
    if (query_start) {
        const char* qp = query_start + 1;  // skip '?'
        while (qp && *qp) {
            const char* amp = strchr(qp, '&');
            const char* pair_end = amp ? amp : qp + strlen(qp);
            size_t pair_len = (size_t)(pair_end - qp);

            // Extract key and value
            const char* eq = memchr(qp, '=', pair_len);
            if (eq) {
                size_t key_len = (size_t)(eq - qp);
                size_t val_len = pair_len - key_len - 1;  // after '='
                const char* val_start = eq + 1;

                char key[64];
                if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
                memcpy(key, qp, key_len);
                key[key_len] = '\0';

                if (strcmp(key, "user") == 0 || strcmp(key, "username") == 0) {
                    if (user_out && user_out_sz > 0) {
                        val_len = val_len >= (size_t)user_out_sz ? (size_t)user_out_sz - 1 : val_len;
                        memcpy(user_out, val_start, val_len);
                        user_out[val_len] = '\0';
                    }
                } else if (strcmp(key, "password") == 0 || strcmp(key, "pass") == 0) {
                    if (pass_out && pass_out_sz > 0) {
                        val_len = val_len >= (size_t)pass_out_sz ? (size_t)pass_out_sz - 1 : val_len;
                        memcpy(pass_out, val_start, val_len);
                        pass_out[val_len] = '\0';
                    }
                } else if (strcmp(key, "schema") == 0) {
                    if (schema_out && schema_out_sz > 0) {
                        val_len = val_len >= (size_t)schema_out_sz ? (size_t)schema_out_sz - 1 : val_len;
                        memcpy(schema_out, val_start, val_len);
                        schema_out[val_len] = '\0';
                    }
                }
            }

            if (amp) {
                qp = amp + 1;
            } else {
                break;
            }
        }
    }

    // Now parse host and port from path_out (which holds the full authority+path)
    // Formats in path_out:
    //   /absolute/path                    — embedded, no host
    //   bare_filename                     — embedded alias
    //   host/path                         — SuperServer
    //   host:3050/path                    — SuperServer with port
    //   host:3050:dir/path                — SuperServer with embedded port (alternate separator)
    //
    // If the path starts with '/', there is no host — it's an embedded/local
    // database path, so the entire string is the database path.
    // Strip leading slashes that arise from firebird:///path (triple slash
    // after the protocol) so the path is /path not ///path.
    while (path_out[0] == '/' && path_out[1] == '/') {
        memmove(path_out, path_out + 1, strlen(path_out));
    }
    if (path_out[0] == '/') {
        // Embedded mode — path_out already holds the full database path
        return true;
    }

    char* saveptr = NULL;
    char* segment = strtok_r(path_out, "/", &saveptr);

    if (!segment) {
        // path_out is empty or starts with '/', segment is NULL → embedded
        // Rebuild path_out from the original conn_str portion for the embedded case
        return true;
    }

    // Check if segment contains ':' → could be host:port
    char* colon = strchr(segment, ':');
    if (colon) {
        // Could be "host:port" or "host:port:dir" or just a filename with ':'
        // In Firebird URL, host:port uses ':' as separator
        *colon = '\0';
        if (host_out_sz > 0) {
            strncpy(host_out, segment, (size_t)host_out_sz - 1);
            host_out[(size_t)host_out_sz - 1] = '\0';
        }
        // Port is after the first ':'
        const char* port_str = colon + 1;
        // Check for a second ':' (embedded-port variant) — just take first segment as port
        const char* second_colon = strchr(port_str, ':');
        size_t port_len = second_colon ? (size_t)(second_colon - port_str) : strlen(port_str);
        if (port_out && port_out_sz > 0) {
            port_len = port_len >= (size_t)port_out_sz ? (size_t)port_out_sz - 1 : port_len;
            memcpy(port_out, port_str, port_len);
            port_out[port_len] = '\0';
        }
    } else {
        // segment is a hostname
        strncpy(host_out, segment, (size_t)host_out_sz - 1);
        host_out[(size_t)host_out_sz - 1] = '\0';
    }

    // The remaining path (after the first '/') is the database path
    // It's still in saveptr — reconstruct it
    if (saveptr && *saveptr) {
        // saveptr points to the remainder after the first '/'
        // We need to prepend '/' to get the full path
        char full_path[4096];
        full_path[0] = '/';
        strncpy(full_path + 1, saveptr, sizeof(full_path) - 2);
        full_path[sizeof(full_path) - 1] = '\0';

        // Copy into path_out (overwrite the host:port: path we put there earlier)
        strncpy(path_out, full_path, (size_t)path_out_sz - 1);
        path_out[(size_t)path_out_sz - 1] = '\0';
    } else {
        // No path after host — path_out currently holds just the host segment
        // This means embedded mode with bare filename
        // path_out already has the filename from the initial copy
    }

    return true;
}

/*
 * Extract the first message from a Firebird isc_status vector and log it.
 * Firebird status vectors pack: [0] = GDS/FB code, [1] = SQL code,
 * [2] = engine-specific code, [3] = message arg, [4..] = more args.
 * On error, the first non-zero entry indicates the error.
 */
void firebird_status_to_error(const fb_status_t* status, const char* designator) {
    if (!status || !designator) {
        return;
    }

    if (status[0] == 0) {
        return;  // No error
    }

    fb_status_t fb_err = status[0];
    fb_status_t sql_code = status[1];

    log_this(designator, "Firebird error: isc_status=%d, sql_code=%d", LOG_LEVEL_ERROR, 2, fb_err, sql_code);
    // Note: real implementation would call isc_interpret / fb_sqlstate
    // to extract the human-readable message. Phase 5 skeleton logs the codes only.
}
