/*
 * Mail Relay template macro engine implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_template.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jansson.h>

#include <src/mailrelay/mailrelay_repository.h>

#define MAILRELAY_TEMPLATE_INITIAL_CAPACITY 8

/* Internal output buffer for incremental render construction. */
typedef struct MailRelayTemplateOutput {
    char* data;
    size_t len;
    size_t cap;
} MailRelayTemplateOutput;

/* Forward declarations for internal helpers. */
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

/* Context shared between mailrelay_template_preview and its callback. */
typedef struct PreviewRenderContext {
    const MailRelayTemplateParams* params;
    const char* app_name;
    const char* server_name;
    const char* timestamp;
    const char* request_id;
    const char* user_email;
    const char* otp_code;
    MailRelayMacroList* macros_used;
    char* subject;
    char* text_body;
    char* html_body;
    bool success;
    char error[256];
} PreviewRenderContext;

void preview_render_callback(MailRelayRepoResult* result, void* user_data);

/* Return true if the character is valid inside a macro name. */
bool is_macro_name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '.';
}

/* Append a string of a given length to the output buffer. */
bool output_append(MailRelayTemplateOutput* out,
                          const char* s,
                          size_t s_len,
                          char* err,
                          size_t err_cap) {
    if (s_len == 0) {
        return true;
    }

    size_t new_len = out->len + s_len;
    if (new_len >= MAILRELAY_TEMPLATE_MAX_OUTPUT_LEN) {
        snprintf(err, err_cap, "rendered output exceeds maximum size");
        return false;
    }

    if (new_len + 1 > out->cap) {
        size_t new_cap = out->cap ? out->cap * 2 : 256;
        while (new_cap < new_len + 1) {
            new_cap *= 2;
        }
        if (new_cap > MAILRELAY_TEMPLATE_MAX_OUTPUT_LEN) {
            new_cap = MAILRELAY_TEMPLATE_MAX_OUTPUT_LEN;
        }
        char* new_data = realloc(out->data, new_cap);
        if (!new_data) {
            snprintf(err, err_cap, "memory allocation failed");
            return false;
        }
        out->data = new_data;
        out->cap = new_cap;
    }

    memcpy(out->data + out->len, s, s_len);
    out->len += s_len;
    out->data[out->len] = '\0';
    return true;
}

/* Append a single character to the output buffer. */
bool output_append_char(MailRelayTemplateOutput* out,
                               char c,
                               char* err,
                               size_t err_cap) {
    return output_append(out, &c, 1, err, err_cap);
}

/* Free the output buffer's owned data. */
void output_free(MailRelayTemplateOutput* out) {
    if (!out) {
        return;
    }
    if (out->data) {
        free(out->data);
        out->data = NULL;
    }
    out->len = 0;
    out->cap = 0;
}

/* Resolve a built-in macro name to its caller-provided value. */
const char* resolve_builtin(const char* name,
                                   const char* app_name,
                                   const char* server_name,
                                   const char* timestamp,
                                   const char* request_id,
                                   const char* user_email,
                                   const char* otp_code) {
    if (strcmp(name, "APP_NAME") == 0) {
        return app_name;
    }
    if (strcmp(name, "SERVER_NAME") == 0) {
        return server_name;
    }
    if (strcmp(name, "TIMESTAMP") == 0) {
        return timestamp;
    }
    if (strcmp(name, "REQUEST_ID") == 0) {
        return request_id;
    }
    if (strcmp(name, "USER_EMAIL") == 0) {
        return user_email;
    }
    if (strcmp(name, "OTP_CODE") == 0) {
        return otp_code;
    }
    return NULL;
}

/*
 * Parse a macro reference at the current position. The opening '%' at *pos
 * has already been verified. On success, name and default_value are filled,
 * *pos is advanced past the closing '%', and has_default is set.
 */
bool parse_macro(const char* template_text,
                        size_t* pos,
                        char* name,
                        size_t name_cap,
                        char* default_value,
                        size_t default_cap,
                        bool* has_default,
                        char* err,
                        size_t err_cap) {
    size_t i = *pos + 1; /* skip opening '%' */
    size_t name_start = i;
    size_t name_len = 0;
    size_t default_start = 0;
    size_t default_len = 0;

    *has_default = false;

    while (template_text[i] != '\0' && template_text[i] != '%') {
        if (!*has_default && template_text[i] == '|') {
            name_len = i - name_start;
            *has_default = true;
            default_start = i + 1;
        } else if (*has_default) {
            default_len++;
        }
        i++;
    }

    if (!*has_default) {
        name_len = i - name_start;
    }

    if (template_text[i] != '%') {
        snprintf(err, err_cap, "unclosed macro starting at offset %zu", *pos);
        return false;
    }

    if (name_len == 0) {
        snprintf(err, err_cap, "empty macro name at offset %zu", *pos);
        return false;
    }

    if (name_len >= name_cap) {
        snprintf(err, err_cap, "macro name too long at offset %zu", *pos);
        return false;
    }

    memcpy(name, template_text + name_start, name_len);
    name[name_len] = '\0';

    if (*has_default) {
        if (default_len >= default_cap) {
            snprintf(err, err_cap, "macro default value too long at offset %zu", *pos);
            return false;
        }
        memcpy(default_value, template_text + default_start, default_len);
        default_value[default_len] = '\0';
    }

    *pos = i + 1; /* advance past closing '%' */
    return true;
}

/* Validate that a macro name contains only allowed characters. */
bool is_valid_macro_name(const char* name, char* err, size_t err_cap) {
    for (size_t i = 0; name[i] != '\0'; i++) {
        if (!is_macro_name_char(name[i])) {
            snprintf(err, err_cap, "invalid macro name '%s'", name);
            return false;
        }
    }
    return true;
}

void mailrelay_template_params_init(MailRelayTemplateParams* params) {
    if (!params) {
        return;
    }
    params->items = NULL;
    params->count = 0;
    params->capacity = 0;
}

bool mailrelay_template_params_add(MailRelayTemplateParams* params,
                                   const char* key,
                                   const char* value) {
    if (!params || !key || !value) {
        return false;
    }

    size_t key_len = strlen(key);
    size_t value_len = strlen(value);
    if (key_len == 0 || key_len >= MAILRELAY_TEMPLATE_MAX_KEY_LEN ||
        value_len >= MAILRELAY_TEMPLATE_MAX_VALUE_LEN) {
        return false;
    }

    if (params->count >= MAILRELAY_TEMPLATE_MAX_PARAMS) {
        return false;
    }

    if (params->count >= params->capacity) {
        int new_capacity = params->capacity ? params->capacity * 2 : MAILRELAY_TEMPLATE_INITIAL_CAPACITY;
        if (new_capacity > MAILRELAY_TEMPLATE_MAX_PARAMS) {
            new_capacity = MAILRELAY_TEMPLATE_MAX_PARAMS;
        }
        MailRelayTemplateParam* new_items = realloc(params->items,
                                                      (size_t)new_capacity * sizeof(MailRelayTemplateParam));
        if (!new_items) {
            return false;
        }
        params->items = new_items;
        params->capacity = new_capacity;
    }

    char* key_copy = strdup(key);
    char* value_copy = strdup(value);
    if (!key_copy || !value_copy) {
        free(key_copy);
        free(value_copy);
        return false;
    }

    params->items[params->count].key = key_copy;
    params->items[params->count].value = value_copy;
    params->count++;
    return true;
}

void mailrelay_template_params_free(MailRelayTemplateParams* params) {
    if (!params) {
        return;
    }
    for (int i = 0; i < params->count; i++) {
        free(params->items[i].key);
        free(params->items[i].value);
    }
    free(params->items);
    params->items = NULL;
    params->count = 0;
    params->capacity = 0;
}

const char* mailrelay_template_params_get(const MailRelayTemplateParams* params,
                                          const char* key) {
    if (!params || !key) {
        return NULL;
    }

    /* Latest entry wins for duplicate keys. */
    for (int i = params->count - 1; i >= 0; i--) {
        if (strcmp(params->items[i].key, key) == 0) {
            return params->items[i].value;
        }
    }
    return NULL;
}

void mailrelay_macro_list_init(MailRelayMacroList* list) {
    if (!list) {
        return;
    }
    list->names = NULL;
    list->count = 0;
    list->capacity = 0;
}

void mailrelay_macro_list_free(MailRelayMacroList* list) {
    if (!list) {
        return;
    }
    for (int i = 0; i < list->count; i++) {
        free(list->names[i]);
    }
    free(list->names);
    list->names = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool mailrelay_macro_list_add(MailRelayMacroList* list, const char* name) {
    if (!list || !name || name[0] == '\0') {
        return false;
    }

    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->names[i], name) == 0) {
            return true;
        }
    }

    if (list->count >= list->capacity) {
        int new_capacity = list->capacity ? list->capacity * 2 : MAILRELAY_TEMPLATE_INITIAL_CAPACITY;
        if (new_capacity > MAILRELAY_TEMPLATE_MAX_PARAMS) {
            new_capacity = MAILRELAY_TEMPLATE_MAX_PARAMS;
        }
        char** new_names = realloc(list->names, (size_t)new_capacity * sizeof(char*));
        if (!new_names) {
            return false;
        }
        list->names = new_names;
        list->capacity = new_capacity;
    }

    list->names[list->count] = strdup(name);
    if (!list->names[list->count]) {
        return false;
    }
    list->count++;
    return true;
}

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
                                           size_t err_cap) {
    if (!template_text || !out || !err || err_cap == 0) {
        if (err && err_cap > 0) {
            snprintf(err, err_cap, "invalid arguments");
        }
        return false;
    }

    *out = NULL;
    err[0] = '\0';

    MailRelayTemplateOutput output = { NULL, 0, 0 };
    size_t pos = 0;
    char name[MAILRELAY_TEMPLATE_MAX_NAME_LEN];
    char default_value[MAILRELAY_TEMPLATE_MAX_VALUE_LEN];

    while (template_text[pos] != '\0') {
        if (template_text[pos] == '%') {
            if (template_text[pos + 1] == '%') {
                if (!output_append_char(&output, '%', err, err_cap)) {
                    output_free(&output);
                    return false;
                }
                pos += 2;
                continue;
            }

            bool has_default = false;
            if (!parse_macro(template_text, &pos, name, sizeof(name),
                             default_value, sizeof(default_value),
                             &has_default, err, err_cap)) {
                output_free(&output);
                return false;
            }

            if (!is_valid_macro_name(name, err, err_cap)) {
                output_free(&output);
                return false;
            }

            const char* value = resolve_builtin(name, app_name, server_name, timestamp,
                                                request_id, user_email, otp_code);
            if (!value) {
                value = mailrelay_template_params_get(params, name);
            }

            if (!value) {
                if (has_default) {
                    value = default_value;
                } else {
                    snprintf(err, err_cap, "MAIL_PARAM_MISSING: macro '%s' not found", name);
                    output_free(&output);
                    return false;
                }
            }

            if (macros_used) {
                if (!mailrelay_macro_list_add(macros_used, name)) {
                    snprintf(err, err_cap, "memory allocation failed");
                    output_free(&output);
                    return false;
                }
            }

            if (!output_append(&output, value, strlen(value), err, err_cap)) {
                output_free(&output);
                return false;
            }
        } else {
            if (!output_append_char(&output, template_text[pos], err, err_cap)) {
                output_free(&output);
                return false;
            }
            pos++;
        }
    }

    if (!output.data) {
        output.data = strdup("");
        if (!output.data) {
            snprintf(err, err_cap, "memory allocation failed");
            return false;
        }
        output.cap = 1;
    }

    *out = output.data;
    return true;
}

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
                               size_t err_cap) {
    return mailrelay_template_render_with_macros(template_text, params, app_name,
                                                  server_name, timestamp, request_id,
                                                  user_email, otp_code, NULL,
                                                  out, err, err_cap);
}

/*
 * Repository callback for mailrelay_template_preview. Renders the subject and
 * body templates retrieved from the database into the preview context.
 */
void preview_render_callback(MailRelayRepoResult* result, void* user_data) {
    PreviewRenderContext* ctx = (PreviewRenderContext*)user_data;
    ctx->success = false;
    ctx->subject = NULL;
    ctx->text_body = NULL;
    ctx->html_body = NULL;
    ctx->error[0] = '\0';

    if (!result || result->status != MAILRELAY_REPO_OK) {
        const char* msg = (result && result->error_message) ? result->error_message : "repository lookup failed";
        snprintf(ctx->error, sizeof(ctx->error), "%s", msg);
        return;
    }

    if (!result->data || !json_is_array(result->data) || json_array_size(result->data) == 0) {
        snprintf(ctx->error, sizeof(ctx->error), "MAIL_TEMPLATE_NOT_FOUND: template not found");
        return;
    }

    json_t* row = json_array_get(result->data, 0);
    if (!json_is_object(row)) {
        snprintf(ctx->error, sizeof(ctx->error), "repository returned unexpected data");
        return;
    }

    json_t* subject_json = json_object_get(row, "subject_template");
    json_t* text_json = json_object_get(row, "text_template");
    json_t* html_json = json_object_get(row, "html_template");

    const char* subject_template = (json_is_string(subject_json) && json_string_value(subject_json)) ?
                                      json_string_value(subject_json) : NULL;
    const char* text_template = (json_is_string(text_json) && json_string_value(text_json)) ?
                                 json_string_value(text_json) : NULL;
    const char* html_template = (json_is_string(html_json) && json_string_value(html_json)) ?
                                 json_string_value(html_json) : NULL;

    if (!subject_template) {
        snprintf(ctx->error, sizeof(ctx->error), "MAIL_TEMPLATE_NOT_FOUND: template missing subject");
        return;
    }

    if (!mailrelay_template_render_with_macros(subject_template, ctx->params, ctx->app_name, ctx->server_name,
                                                ctx->timestamp, ctx->request_id, ctx->user_email, ctx->otp_code,
                                                ctx->macros_used,
                                                &ctx->subject, ctx->error, sizeof(ctx->error))) {
        return;
    }

    if (text_template) {
        if (!mailrelay_template_render_with_macros(text_template, ctx->params, ctx->app_name, ctx->server_name,
                                                    ctx->timestamp, ctx->request_id, ctx->user_email, ctx->otp_code,
                                                    ctx->macros_used,
                                                    &ctx->text_body, ctx->error, sizeof(ctx->error))) {
            free(ctx->subject);
            ctx->subject = NULL;
            return;
        }
    }

    if (html_template) {
        if (!mailrelay_template_render_with_macros(html_template, ctx->params, ctx->app_name, ctx->server_name,
                                                    ctx->timestamp, ctx->request_id, ctx->user_email, ctx->otp_code,
                                                    ctx->macros_used,
                                                    &ctx->html_body, ctx->error, sizeof(ctx->error))) {
            free(ctx->subject);
            free(ctx->text_body);
            ctx->subject = NULL;
            ctx->text_body = NULL;
            return;
        }
    }

    ctx->success = true;
    ctx->error[0] = '\0';
}

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
                                            size_t err_cap) {
    if (!template_key || !subject_out || !text_body_out || !html_body_out || !err || err_cap == 0) {
        if (err && err_cap > 0) {
            snprintf(err, err_cap, "invalid arguments");
        }
        return false;
    }

    *subject_out = NULL;
    *text_body_out = NULL;
    *html_body_out = NULL;
    err[0] = '\0';

    MailRelayRepoTemplateGetByKey req = { template_key };
    PreviewRenderContext ctx = { 0 };
    ctx.params = params;
    ctx.app_name = app_name;
    ctx.server_name = server_name;
    ctx.timestamp = timestamp;
    ctx.request_id = request_id;
    ctx.user_email = user_email;
    ctx.otp_code = otp_code;
    ctx.macros_used = macros_used;

    if (!mailrelay_repo_template_get_by_key(&req, preview_render_callback, &ctx)) {
        snprintf(err, err_cap, "repository lookup failed");
        return false;
    }

    if (!ctx.success) {
        snprintf(err, err_cap, "%s", ctx.error);
        return false;
    }

    *subject_out = ctx.subject;
    *text_body_out = ctx.text_body;
    *html_body_out = ctx.html_body;
    return true;
}

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
                                size_t err_cap) {
    return mailrelay_template_preview_with_macros(template_key, params, app_name, server_name,
                                                   timestamp, request_id, user_email, otp_code,
                                                   subject_out, text_body_out, html_body_out,
                                                   NULL, err, err_cap);
}
