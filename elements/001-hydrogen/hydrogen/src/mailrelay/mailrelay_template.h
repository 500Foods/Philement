/*
 * Mail Relay template macro engine.
 *
 * Syntax:
 *   %MACRO%              - Replace with the value of user or built-in macro.
 *   %MACRO|default%      - Replace with macro value, or "default" if missing.
 *   %%                   - A literal percent sign.
 *
 * Built-in macros:
 *   %APP_NAME%           - Application name from configuration.
 *   %SERVER_NAME%        - Server name from configuration.
 *   %TIMESTAMP%          - ISO 8601 timestamp (caller-formatted string).
 *   %USER_EMAIL%         - Authenticated user email, if available.
 *   %REQUEST_ID%         - Request/correlation ID, if available.
 *   %OTP_CODE%           - One-time password code, if available.
 *
 * User macros are supplied via MailRelayTemplateParams.
 *
 * NO AUTOMATIC HTML OR TEXT ESCAPING is performed. Callers must sanitize all
 * values before adding them to the parameter map. This keeps the engine
 * deterministic and avoids double-encoding surprises.
 *
 * A missing required macro (one with no default) causes the render to fail
 * with a MAIL_PARAM_MISSING error written to the provided error buffer.
 */

#ifndef MAILRELAY_TEMPLATE_H
#define MAILRELAY_TEMPLATE_H

// System includes
#include <stdbool.h>
#include <stddef.h>

// Project includes
#include <src/mailrelay/mailrelay_repository.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of a macro name (user or built-in). */
#define MAILRELAY_TEMPLATE_MAX_NAME_LEN 128

/* Maximum number of user parameters in a single render call. */
#define MAILRELAY_TEMPLATE_MAX_PARAMS 256

/* Maximum length of a user parameter key. */
#define MAILRELAY_TEMPLATE_MAX_KEY_LEN 128

/* Maximum length of a user parameter value. */
#define MAILRELAY_TEMPLATE_MAX_VALUE_LEN 16384

/* Maximum rendered output size in bytes. */
#define MAILRELAY_TEMPLATE_MAX_OUTPUT_LEN 1048576

/*
 * A single user-supplied macro parameter.
 */
typedef struct MailRelayTemplateParam {
    char* key;    /**< Parameter name (heap-allocated). */
    char* value;  /**< Parameter value (heap-allocated). */
} MailRelayTemplateParam;

/*
 * User-supplied macro parameter map.
 *
 * The map is append-only. Adding a duplicate key replaces the previous value
 * so the latest addition wins on lookup.
 */
typedef struct MailRelayTemplateParams {
    MailRelayTemplateParam* items;  /**< Dynamic array of parameters. */
    int count;                     /**< Number of stored parameters. */
    int capacity;                  /**< Allocated capacity of items. */
} MailRelayTemplateParams;

/* Initialize an empty parameter map. */
void mailrelay_template_params_init(MailRelayTemplateParams* params);

/*
 * Add a key/value pair to the parameter map. Both key and value are copied.
 *
 * @param params Parameter map.
 * @param key    Parameter name.
 * @param value  Parameter value.
 * @return true on success, false on allocation failure or invalid input.
 */
bool mailrelay_template_params_add(MailRelayTemplateParams* params,
                                     const char* key,
                                     const char* value);

/* Free all owned strings and reset the parameter map. */
void mailrelay_template_params_free(MailRelayTemplateParams* params);

/*
 * Look up a value by key. The latest added entry wins for duplicate keys.
 *
 * @param params Parameter map.
 * @param key    Parameter name.
 * @return The value, or NULL if the key is not present.
 */
const char* mailrelay_template_params_get(const MailRelayTemplateParams* params,
                                          const char* key);

/*
 * A list of macro names used during template rendering.
 *
 * The list deduplicates names as they are added and is owned by the caller.
 */
typedef struct MailRelayMacroList {
    char** names;  /**< Macro names (heap-allocated). */
    int count;     /**< Number of names in the list. */
    int capacity;  /**< Allocated capacity. */
} MailRelayMacroList;

/* Initialize an empty macro list. */
void mailrelay_macro_list_init(MailRelayMacroList* list);

/* Free all owned names and reset the macro list. */
void mailrelay_macro_list_free(MailRelayMacroList* list);

/*
 * Add a macro name to the list if it is not already present.
 *
 * @param list Macro list.
 * @param name Macro name.
 * @return true on success, false on allocation failure or invalid input.
 */
bool mailrelay_macro_list_add(MailRelayMacroList* list, const char* name);

/*
 * Render a template using the provided parameters and built-in macros.
 *
 * @param template    Input template string.
 * @param params      User macro parameters (may be NULL).
 * @param app_name    Value for %APP_NAME% (may be NULL).
 * @param server_name Value for %SERVER_NAME% (may be NULL).
 * @param timestamp   Value for %TIMESTAMP% (may be NULL).
 * @param request_id  Value for %REQUEST_ID% (may be NULL).
 * @param user_email  Value for %USER_EMAIL% (may be NULL).
 * @param otp_code    Value for %OTP_CODE% (may be NULL).
 * @param out         Out: caller-owned rendered string. Must be freed by caller.
 * @param err         Buffer for error message on failure.
 * @param err_cap     Capacity of err buffer.
 * @return true on success, false on error (missing macro, allocation failure,
 *         output too large, unclosed macro).
 */
bool mailrelay_template_render(const char* template_text,
                               const MailRelayTemplateParams* params,
                               const char* app_name,
                               const char* server_name,
                               const char* timestamp,
                               const char* request_id,
                               const char* user_email,
                               const char* otp_code,
                               char** out,
                               char* err,
                               size_t err_cap);

/*
 * Render a template while recording the macros that are resolved.
 *
 * This is identical to mailrelay_template_render except that any built-in or
 * user macro that is successfully resolved is added to @a macros_used. Pass
 * NULL for @a macros_used if the list is not needed.
 */
bool mailrelay_template_render_with_macros(const char* template_text,
                                           const MailRelayTemplateParams* params,
                                           const char* app_name,
                                           const char* server_name,
                                           const char* timestamp,
                                           const char* request_id,
                                           const char* user_email,
                                           const char* otp_code,
                                           MailRelayMacroList* macros_used,
                                           char** out,
                                           char* err,
                                           size_t err_cap);

/*
 * Preview a configured template by key.
 *
 * Looks up the template from the configured Mail Relay database, renders the
 * subject, text body, and HTML body using the provided parameters and built-in
 * macros, and returns the rendered strings. This function performs no queue
 * write, no enqueue, and no SMTP send; it is side-effect-free.
 *
 * @param template_key  Template key to look up (e.g. "mail.test").
 * @param params        User macro parameters (may be NULL).
 * @param app_name      Value for %APP_NAME% (may be NULL).
 * @param server_name   Value for %SERVER_NAME% (may be NULL).
 * @param timestamp     Value for %TIMESTAMP% (may be NULL).
 * @param request_id    Value for %REQUEST_ID% (may be NULL).
 * @param user_email    Value for %USER_EMAIL% (may be NULL).
 * @param otp_code      Value for %OTP_CODE% (may be NULL).
 * @param subject_out   Out: caller-owned rendered subject. Must be freed by caller.
 * @param text_body_out Out: caller-owned rendered text body, or NULL if the
 *                      template has no text_template. Must be freed by caller.
 * @param html_body_out Out: caller-owned rendered HTML body, or NULL if the
 *                      template has no html_template. Must be freed by caller.
 * @param err           Buffer for error message on failure.
 * @param err_cap       Capacity of err buffer.
 * @return true on success, false on error (template not found, missing macro,
 *         database/repository failure, allocation failure).
 */
bool mailrelay_template_preview(const char* template_key,
                                const MailRelayTemplateParams* params,
                                const char* app_name,
                                const char* server_name,
                                const char* timestamp,
                                const char* request_id,
                                const char* user_email,
                                const char* otp_code,
                                char** subject_out,
                                char** text_body_out,
                                char** html_body_out,
                                char* err,
                                size_t err_cap);

/*
 * Preview a configured template by key while recording the macros used.
 *
 * This is identical to mailrelay_template_preview except that any built-in or
 * user macro resolved during rendering is added to @a macros_used. Pass NULL
 * for @a macros_used if the list is not needed.
 */
bool mailrelay_template_preview_with_macros(const char* template_key,
                                            const MailRelayTemplateParams* params,
                                            const char* app_name,
                                            const char* server_name,
                                            const char* timestamp,
                                            const char* request_id,
                                            const char* user_email,
                                            const char* otp_code,
                                            char** subject_out,
                                            char** text_body_out,
                                            char** html_body_out,
                                            MailRelayMacroList* macros_used,
                                            char* err,
                                            size_t err_cap);

#ifdef __cplusplus
}
#endif

/*
 * The following helpers are exposed (non-static) primarily so the Unity test
 * suite can exercise the template macro engine (name validation, output buffer
 * growth, macro parsing, and built-in resolution) and the preview repository
 * callback directly. They are not part of the stable public API.
 */
typedef struct MailRelayTemplateOutput MailRelayTemplateOutput;

bool is_macro_name_char(char c);
bool output_append(MailRelayTemplateOutput* out,
                   const char* s,
                   size_t s_len,
                   char* err,
                   size_t err_cap);
bool output_append_char(MailRelayTemplateOutput* out,
                        char c,
                        char* err,
                        size_t err_cap);
void output_free(MailRelayTemplateOutput* out);
const char* resolve_builtin(const char* name,
                             const char* app_name,
                             const char* server_name,
                             const char* timestamp,
                             const char* request_id,
                             const char* user_email,
                             const char* otp_code);
bool parse_macro(const char* template_text,
                 size_t* pos,
                 char* name,
                 size_t name_cap,
                 char* default_value,
                 size_t default_cap,
                 bool* has_default,
                 char* err,
                 size_t err_cap);
bool is_valid_macro_name(const char* name, char* err, size_t err_cap);
void preview_render_callback(MailRelayRepoResult* result, void* user_data);

#endif /* MAILRELAY_TEMPLATE_H */
