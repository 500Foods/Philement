/*
 * Mail Relay event emission API.
 *
 * System and subsystem events are mapped to configured Lua handler scripts.
 * Each handler receives an event table and returns a mail request table that
 * the C layer converts into a templated enqueue via mailrelay_send_template().
 *
 * Phase 6.1a uses built-in default Lua handlers for the well-known events
 * `system.server_started` and `system.server_stopped`. Later sub-chunks add
 * loading of custom event handler scripts from the scripts table.
 */

#ifndef MAILRELAY_EVENTS_H
#define MAILRELAY_EVENTS_H

// System includes
#include <stdbool.h>
#include <stddef.h>

// Project includes
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_template.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Emit a Mail Relay event.
 *
 * The event key is matched against MailRelay.Events.Rules. If a rule exists,
 * its Lua handler is executed in a fresh Lua state and the returned mail
 * request table is enqueued. If no rule exists, well-known system events
 * (`system.server_started`, `system.server_stopped`) use a built-in default
 * handler that sends to MailRelay.AdminRecipients using the matching seeded
 * template.
 *
 * Rate limiting is applied per event key using a token bucket configured by
 * MailRelay.Events.MaxEventsPerInterval and EventIntervalSeconds.
 *
 * @param event_key Required event key (e.g. "system.server_started").
 * @param params    Optional user macro parameters for the event.
 * @return true if the event was accepted (not necessarily queued; a suppressed
 *         or no-op handler still returns true), false if the event could not
 *         be emitted due to disabled subsystem, rate limit, or invalid args.
 */
bool mailrelay_event_emit(const char* event_key,
                          const MailRelayTemplateParams* params);

/*
 * Test seam: override the function used to dispatch a handler's mail request.
 * The default is mailrelay_send_template. Pass NULL to restore the default.
 */
typedef MailRelayStatus (*mailrelay_event_dispatcher_fn)(const MailRelaySendTemplateRequest* req,
                                                          MailRelaySendTemplateResponse* resp,
                                                          char* err,
                                                          size_t err_cap);
void mailrelay_event_set_dispatcher(mailrelay_event_dispatcher_fn fn);

/* Forward declaration so the test-only helper prototypes below need not pull
 * the Lua headers into this public header. */
typedef struct lua_State lua_State;

/*
 * The following helpers are exposed (non-static) primarily so the Unity test
 * suite can exercise rate limiting, the Lua table/param marshalling, handler
 * dispatch, and rule resolution directly. They are not part of the stable
 * public API.
 */
bool mailrelay_event_check_rate_limit(const char* event_key,
                                       int max_per_interval,
                                       int interval_seconds);
void mailrelay_event_free_rate_limits(void);
void mailrelay_event_push_event_table(lua_State* L,
                                      const char* event_key,
                                      const MailRelayTemplateParams* params);
char** mailrelay_event_read_string_array(lua_State* L,
                                         const char* field_name,
                                         int* out_count);
char* mailrelay_event_read_string(lua_State* L, const char* field_name);
int mailrelay_event_read_int(lua_State* L,
                             const char* field_name,
                             int default_value);
bool mailrelay_event_read_params(lua_State* L,
                                 const char* field_name,
                                 MailRelayTemplateParams* out_params);
void mailrelay_event_free_string_array(char** arr, int count);
bool mailrelay_event_dispatch_request(lua_State* L, char* err, size_t err_cap);
bool mailrelay_event_run_handler(const char* source,
                                 const char* chunk_name,
                                 const char* event_key,
                                 const MailRelayTemplateParams* params,
                                 char* err,
                                 size_t err_cap);
const MailEventRule* mailrelay_event_find_rule(const char* event_key);
const char* mailrelay_event_resolve_source(const char* event_key,
                                           const MailEventRule* rule);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_EVENTS_H */
