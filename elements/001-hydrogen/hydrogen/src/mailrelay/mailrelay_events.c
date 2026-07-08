/*
 * Mail Relay event emission implementation.
 *
 * Maps configured event keys to Lua handler scripts. Handlers receive an
 * event table and return a mail request table; the C layer enqueues the
 * rendered template through mailrelay_send_template().
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_events.h>
#include <src/mailrelay/mailrelay_internal.h>

#include <src/scripting/lua_context.h>
#include <src/utils/utils_time.h>

#include <stdlib.h>
#include <string.h>

/* Built-in default handler for system.server_started. */
static const char* MAILRELAY_EVENT_STARTED_SCRIPT =
    "function handle_event(event)\n"
    "    local recipients = event.admin_recipients or {}\n"
    "    if #recipients == 0 then\n"
    "        return nil\n"
    "    end\n"
    "    local params = {\n"
    "        SERVER_NAME = event.server_name or \"\",\n"
    "        APP_NAME = event.app_name or \"\",\n"
    "        TIMESTAMP = event.timestamp or \"\"\n"
    "    }\n"
    "    if event.params then\n"
    "        for k, v in pairs(event.params) do\n"
    "            params[k] = v\n"
    "        end\n"
    "    end\n"
    "    return {\n"
    "        template_key = \"system.server_started\",\n"
    "        to = recipients,\n"
    "        params = params\n"
    "    }\n"
    "end\n";

/* Built-in default handler for system.server_stopped. */
static const char* MAILRELAY_EVENT_STOPPED_SCRIPT =
    "function handle_event(event)\n"
    "    local recipients = event.admin_recipients or {}\n"
    "    if #recipients == 0 then\n"
    "        return nil\n"
    "    end\n"
    "    local params = {\n"
    "        SERVER_NAME = event.server_name or \"\",\n"
    "        APP_NAME = event.app_name or \"\",\n"
    "        TIMESTAMP = event.timestamp or \"\"\n"
    "    }\n"
    "    if event.params then\n"
    "        for k, v in pairs(event.params) do\n"
    "            params[k] = v\n"
    "        end\n"
    "    end\n"
    "    return {\n"
    "        template_key = \"system.server_stopped\",\n"
    "        to = recipients,\n"
    "        params = params\n"
    "    }\n"
    "end\n";

/*
 * Check and update the per-event-key rate limit.
 *
 * Returns true if the emit is allowed, false if rate limited.
 * Caller must hold mailrelay_runtime->mutex.
 */
static bool mailrelay_event_check_rate_limit(const char* event_key,
                                              int max_per_interval,
                                              int interval_seconds) {
    if (!mailrelay_runtime || !event_key || max_per_interval <= 0 || interval_seconds <= 0) {
        return true;
    }

    time_t now = time(NULL);
    MailRelayRateLimitEntry* entry = mailrelay_runtime->rate_limit_head;
    while (entry != NULL) {
        if (strcmp(entry->event_key, event_key) == 0) {
            if ((now - entry->window_start) >= interval_seconds) {
                entry->window_start = now;
                entry->count = 1;
                return true;
            }
            if (entry->count < max_per_interval) {
                entry->count++;
                return true;
            }
            return false;
        }
        entry = entry->next;
    }

    entry = calloc(1, sizeof(MailRelayRateLimitEntry));
    if (!entry) {
        return true; // Fail open on allocation error to avoid blocking critical events.
    }
    entry->event_key = strdup(event_key);
    entry->window_start = now;
    entry->count = 1;
    entry->next = mailrelay_runtime->rate_limit_head;
    mailrelay_runtime->rate_limit_head = entry;
    return true;
}

/*
 * Free all rate limit entries. Called during shutdown.
 * Caller must hold mailrelay_runtime->mutex.
 */
static void mailrelay_event_free_rate_limits(void) {
    if (!mailrelay_runtime) {
        return;
    }
    MailRelayRateLimitEntry* entry = mailrelay_runtime->rate_limit_head;
    while (entry != NULL) {
        MailRelayRateLimitEntry* next = entry->next;
        free(entry->event_key);
        free(entry);
        entry = next;
    }
    mailrelay_runtime->rate_limit_head = NULL;
}

/*
 * Push the event details as a Lua table on top of the stack.
 * The table contains: event_key, timestamp, server_name, app_name,
 * admin_recipients (array), params (table).
 */
static void mailrelay_event_push_event_table(lua_State* L,
                                              const char* event_key,
                                              const MailRelayTemplateParams* params) {
    lua_newtable(L);

    lua_pushstring(L, event_key);
    lua_setfield(L, -2, "event_key");

    char timestamp[32];
    format_iso_time(time(NULL), timestamp, sizeof(timestamp));
    lua_pushstring(L, timestamp);
    lua_setfield(L, -2, "timestamp");

    const char* server_name = (app_config && app_config->server.server_name)
                                  ? app_config->server.server_name
                                  : "Hydrogen";
    lua_pushstring(L, server_name);
    lua_setfield(L, -2, "server_name");
    lua_pushstring(L, server_name);
    lua_setfield(L, -2, "app_name");

    lua_newtable(L);
    if (app_config) {
        int admin_count = app_config->mail_relay.AdminRecipientCount;
        for (int i = 0; i < admin_count; i++) {
            const char* addr = app_config->mail_relay.AdminRecipients[i];
            if (addr && addr[0] != '\0') {
                lua_pushinteger(L, (lua_Integer)(lua_rawlen(L, -1)) + 1);
                lua_pushstring(L, addr);
                lua_settable(L, -3);
            }
        }
    }
    lua_setfield(L, -2, "admin_recipients");

    lua_newtable(L);
    if (params) {
        for (int i = 0; i < params->count; i++) {
            if (params->items[i].key && params->items[i].value) {
                lua_pushstring(L, params->items[i].key);
                lua_pushstring(L, params->items[i].value);
                lua_settable(L, -3);
            }
        }
    }
    lua_setfield(L, -2, "params");
}

/*
 * Read a string array from a Lua table field. Returns a heap-allocated
 * array of heap-allocated strings and sets *out_count. Returns NULL if
 * the field is missing, empty, or not a table.
 */
static char** mailrelay_event_read_string_array(lua_State* L,
                                                 const char* field_name,
                                                 int* out_count) {
    *out_count = 0;
    lua_getfield(L, -1, field_name);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return NULL;
    }

    int n = (int)lua_rawlen(L, -1);
    if (n <= 0) {
        lua_pop(L, 1);
        return NULL;
    }

    char** arr = calloc((size_t)n, sizeof(char*));
    if (!arr) {
        lua_pop(L, 1);
        return NULL;
    }

    int valid_count = 0;
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, -1, i);
        const char* s = lua_tostring(L, -1);
        if (s && s[0] != '\0') {
            arr[valid_count] = strdup(s);
            if (arr[valid_count]) {
                valid_count++;
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (valid_count == 0) {
        free(arr);
        return NULL;
    }
    *out_count = valid_count;
    return arr;
}

/*
 * Read an optional string field from a Lua table.
 * Returns a newly allocated string or NULL.
 */
static char* mailrelay_event_read_string(lua_State* L, const char* field_name) {
    lua_getfield(L, -1, field_name);
    const char* s = lua_tostring(L, -1);
    char* result = (s && s[0] != '\0') ? strdup(s) : NULL;
    lua_pop(L, 1);
    return result;
}

/*
 * Read an optional integer field from a Lua table.
 * Returns the integer if present and valid, otherwise default_value.
 */
static int mailrelay_event_read_int(lua_State* L,
                                     const char* field_name,
                                     int default_value) {
    lua_getfield(L, -1, field_name);
    int result = default_value;
    if (lua_isinteger(L, -1)) {
        result = (int)lua_tointeger(L, -1);
    } else if (lua_isnumber(L, -1)) {
        result = (int)lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    return result;
}

/*
 * Read the returned params table from the handler into a
 * MailRelayTemplateParams structure. Caller must free with
 * mailrelay_template_params_free().
 */
static bool mailrelay_event_read_params(lua_State* L,
                                         const char* field_name,
                                         MailRelayTemplateParams* out_params) {
    mailrelay_template_params_init(out_params);
    lua_getfield(L, -1, field_name);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return true; // Empty params is OK.
    }

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        const char* key = lua_tostring(L, -2);
        const char* value = lua_tostring(L, -1);
        if (key && value) {
            if (!mailrelay_template_params_add(out_params, key, value)) {
                lua_pop(L, 2);
                lua_pop(L, 1);
                return false;
            }
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return true;
}

/*
 * Free a string array returned by mailrelay_event_read_string_array.
 */
static void mailrelay_event_free_string_array(char** arr, int count) {
    if (!arr) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(arr[i]);
    }
    free(arr);
}

/* Test seam: dispatcher used for handler mail requests. */
static mailrelay_event_dispatcher_fn event_dispatcher = mailrelay_send_template;

void mailrelay_event_set_dispatcher(mailrelay_event_dispatcher_fn fn) {
    event_dispatcher = fn ? fn : mailrelay_send_template;
}

/*
 * Convert the handler's returned mail-request table into a
 * MailRelaySendTemplateRequest and enqueue it.
 */
static bool mailrelay_event_dispatch_request(lua_State* L, char* err, size_t err_cap) {
    if (!lua_istable(L, -1)) {
        snprintf(err, err_cap, "handler did not return a table");
        return false;
    }

    char* template_key = mailrelay_event_read_string(L, "template_key");
    if (!template_key) {
        snprintf(err, err_cap, "handler returned no template_key");
        return false;
    }

    int to_count = 0;
    char** to = mailrelay_event_read_string_array(L, "to", &to_count);
    if (!to || to_count == 0) {
        free(template_key);
        snprintf(err, err_cap, "handler returned no recipients");
        return false;
    }

    int cc_count = 0;
    char** cc = mailrelay_event_read_string_array(L, "cc", &cc_count);
    int bcc_count = 0;
    char** bcc = mailrelay_event_read_string_array(L, "bcc", &bcc_count);

    char* from = mailrelay_event_read_string(L, "from");
    char* reply_to = mailrelay_event_read_string(L, "reply_to");
    char* idempotency_key = mailrelay_event_read_string(L, "idempotency_key");
    char* debounce_key = mailrelay_event_read_string(L, "debounce_key");
    int priority = mailrelay_event_read_int(L, "priority", 0);

    MailRelayTemplateParams params;
    if (!mailrelay_event_read_params(L, "params", &params)) {
        free(template_key);
        mailrelay_event_free_string_array(to, to_count);
        mailrelay_event_free_string_array(cc, cc_count);
        mailrelay_event_free_string_array(bcc, bcc_count);
        free(from);
        free(reply_to);
        free(idempotency_key);
        free(debounce_key);
        snprintf(err, err_cap, "failed to read params table");
        return false;
    }

    MailRelaySendTemplateRequest req = {0};
    req.template_key = template_key;
    req.to = (const char* const*)to;
    req.to_count = to_count;
    req.cc = (const char* const*)cc;
    req.cc_count = cc_count;
    req.bcc = (const char* const*)bcc;
    req.bcc_count = bcc_count;
    req.from = from;
    req.reply_to = reply_to;
    req.params = &params;
    req.idempotency_key = idempotency_key;
    req.priority = priority;
    req.app_name = (app_config && app_config->server.server_name) ? app_config->server.server_name : "Hydrogen";
    req.server_name = req.app_name;

    char timestamp[32];
    format_iso_time(time(NULL), timestamp, sizeof(timestamp));
    req.timestamp = timestamp;

    MailRelaySendTemplateResponse resp;
    mailrelay_send_template_response_init(&resp);
    MailRelayStatus status = event_dispatcher(&req, &resp, err, err_cap);

    mailrelay_send_template_response_free(&resp);
    mailrelay_template_params_free(&params);
    free(template_key);
    mailrelay_event_free_string_array(to, to_count);
    mailrelay_event_free_string_array(cc, cc_count);
    mailrelay_event_free_string_array(bcc, bcc_count);
    free(from);
    free(reply_to);
    free(idempotency_key);
    free(debounce_key);

    return status == MAILRELAY_OK;
}

/*
 * Execute a Lua handler source for the given event.
 * Returns true if a mail was successfully dispatched or the handler chose
 * not to send (returned nil/empty). Returns false on Lua/runtime/error.
 */
static bool mailrelay_event_run_handler(const char* source,
                                         const char* chunk_name,
                                         const char* event_key,
                                         const MailRelayTemplateParams* params,
                                         char* err,
                                         size_t err_cap) {
    if (!source || !event_key) {
        snprintf(err, err_cap, "missing handler source or event key");
        return false;
    }

    lua_State* L = H_lua_create_context();
    if (!L) {
        snprintf(err, err_cap, "failed to create Lua context");
        return false;
    }

    int rc = H_lua_run_string(L, source, chunk_name);
    if (rc != LUA_OK) {
        const char* lua_err = lua_tostring(L, -1);
        snprintf(err, err_cap, "handler load failed: %s",
                 lua_err ? lua_err : "unknown");
        H_lua_destroy_context(L);
        return false;
    }

    lua_getglobal(L, "handle_event");
    if (!lua_isfunction(L, -1)) {
        snprintf(err, err_cap, "handler does not define handle_event");
        lua_pop(L, 1);
        H_lua_destroy_context(L);
        return false;
    }

    mailrelay_event_push_event_table(L, event_key, params);

    rc = lua_pcall(L, 1, 1, 0);
    if (rc != LUA_OK) {
        const char* lua_err = lua_tostring(L, -1);
        snprintf(err, err_cap, "handler execution failed: %s",
                 lua_err ? lua_err : "unknown");
        H_lua_destroy_context(L);
        return false;
    }

    bool dispatched = true;
    if (lua_istable(L, -1)) {
        dispatched = mailrelay_event_dispatch_request(L, err, err_cap);
    } else if (lua_isnil(L, -1)) {
        // Handler chose not to send.
        err[0] = '\0';
    } else {
        snprintf(err, err_cap, "handler returned unexpected type");
        dispatched = false;
    }

    H_lua_destroy_context(L);
    return dispatched;
}

/*
 * Find the configured event rule for event_key.
 * Returns a pointer to the rule or NULL if not found.
 */
static const MailEventRule* mailrelay_event_find_rule(const char* event_key) {
    if (!app_config || !event_key) {
        return NULL;
    }
    const MailRelayConfig* config = &app_config->mail_relay;
    for (int i = 0; i < config->Events.RuleCount; i++) {
        if (config->Events.Rules[i].event_key &&
            strcmp(config->Events.Rules[i].event_key, event_key) == 0) {
            return &config->Events.Rules[i];
        }
    }
    return NULL;
}

/*
 * Resolve the Lua handler source for an event.
 * Phase 6.1a only supports built-in default handlers for the well-known
 * system events. Phase 6.1b will add DB-loaded custom scripts.
 */
static const char* mailrelay_event_resolve_source(const char* event_key,
                                                   const MailEventRule* rule) {
    (void)rule; // Reserved for DB script lookup in Phase 6.1b.

    if (strcmp(event_key, "system.server_started") == 0) {
        return MAILRELAY_EVENT_STARTED_SCRIPT;
    }
    if (strcmp(event_key, "system.server_stopped") == 0) {
        return MAILRELAY_EVENT_STOPPED_SCRIPT;
    }
    return NULL;
}

bool mailrelay_event_emit(const char* event_key,
                          const MailRelayTemplateParams* params) {
    if (!event_key || event_key[0] == '\0') {
        return false;
    }

    if (!app_config || !app_config->mail_relay.Enabled) {
        return false;
    }

    if (!mailrelay_runtime_is_initialized() || mailrelay_runtime->shutdown_requested) {
        return false;
    }

    const MailRelayConfig* config = &app_config->mail_relay;
    if (!config->Events.Enabled) {
        return false;
    }

    const MailEventRule* rule = mailrelay_event_find_rule(event_key);
    const char* source = mailrelay_event_resolve_source(event_key, rule);
    if (!source) {
        log_this(SR_MAIL_RELAY,
                 "No handler available for event '%s'",
                 LOG_LEVEL_DEBUG, 1, event_key);
        return false;
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    bool allowed = mailrelay_event_check_rate_limit(
        event_key,
        config->Events.MaxEventsPerInterval,
        config->Events.EventIntervalSeconds);
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    if (!allowed) {
        log_this(SR_MAIL_RELAY,
                 "Event '%s' rate limited",
                 LOG_LEVEL_ALERT, 1, event_key);
        return false;
    }

    char err[256];
    bool ok = mailrelay_event_run_handler(source, event_key, event_key, params, err, sizeof(err));
    if (!ok) {
        log_this(SR_MAIL_RELAY,
                 "Event '%s' handler failed: %s",
                 LOG_LEVEL_ERROR, 2, event_key, err);
    } else {
        log_this(SR_MAIL_RELAY,
                 "Event '%s' dispatched",
                 LOG_LEVEL_DEBUG, 1, event_key);
    }
    return ok;
}

/*
 * Free all rate-limit state. Public so mailrelay_shutdown can call it.
 */
void mailrelay_event_free_all_rate_limits(void) {
    if (!mailrelay_runtime) {
        return;
    }
    pthread_mutex_lock(&mailrelay_runtime->mutex);
    mailrelay_event_free_rate_limits();
    pthread_mutex_unlock(&mailrelay_runtime->mutex);
}
