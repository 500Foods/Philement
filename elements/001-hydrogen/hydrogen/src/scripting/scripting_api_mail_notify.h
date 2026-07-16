/*
 * Scripting Subsystem - Host API: H.mail, H.notify
 *
 * Internal declarations for scripting_api_mail_notify.c, exposed for
 * Unity unit tests (NOT part of the stable public API). The parse
 * helpers operate on a Lua stack and the module-private MailLuaParse
 * struct; they are exposed so tests can drive Lua parsing directly.
 */

#ifndef HYDROGEN_SCRIPTING_API_MAIL_NOTIFY_H
#define HYDROGEN_SCRIPTING_API_MAIL_NOTIFY_H

// Forward-declare Lua types so we don't drag the Lua headers into
// every translation unit that includes this header.
typedef struct lua_State lua_State;

// MailRelay types used by the parse helpers.
#include <src/utils/utils_uuid.h>             // UUID_STR_LEN
#include <src/mailrelay/mailrelay.h>          // MailRelayTemplateParams, MailRelayStatus
#include <src/mailrelay/mailrelay_template.h> // mailrelay_template_params_*

/*
 * Owned parse buffers freed by free_mail_parse. Defined here so tests
 * can stack-allocate a MailLuaParse and call the parse helpers.
 */
typedef struct MailLuaParse {
    char* template_key;
    char* subject;
    char* text_body;
    char* html_body;
    char* from;
    char* reply_to;
    char* idempotency_key;
    char idempotency_owned[UUID_STR_LEN];
    bool idempotency_generated;
    char** to;
    int to_count;
    char** cc;
    int cc_count;
    char** bcc;
    int bcc_count;
    MailRelayTemplateParams params;
    int priority;
    char err[512];
} MailLuaParse;

void free_mail_parse(MailLuaParse* p);
void mail_parse_init(MailLuaParse* p);
bool parse_recipient_field(lua_State* L, int table_idx, const char* field,
                           char*** out_items, int* out_count,
                           char* err, size_t err_cap);
bool parse_params_table(lua_State* L, int table_idx, MailRelayTemplateParams* params,
                        char* err, size_t err_cap);
bool parse_optional_string_field(lua_State* L, int table_idx, const char* field,
                                 char** out, size_t max_len,
                                 char* err, size_t err_cap);
bool parse_mail_message(lua_State* L, MailLuaParse* p);
void status_to_mail_error(MailRelayStatus status, const char* producer_err,
                          char* out, size_t out_cap);

#endif /* HYDROGEN_SCRIPTING_API_MAIL_NOTIFY_H */
