/*
 * Mail Relay message model and validation implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_result.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void mailrelay_message_init(MailRelayMessage* m) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
}

void mailrelay_message_free(MailRelayMessage* m) {
    if (!m) return;
    free(m->from);
    free(m->reply_to);
    free(m->subject);
    free(m->text_body);
    free(m->html_body);
    free(m->template_key);
    free(m->headers);
    free(m->idempotency_key);
    free(m->debounce_key);
    for (int i = 0; i < m->to_count; i++) free(m->to[i]);
    for (int i = 0; i < m->cc_count; i++) free(m->cc[i]);
    for (int i = 0; i < m->bcc_count; i++) free(m->bcc[i]);
    memset(m, 0, sizeof(*m));
}

bool mailrelay_message_copy(MailRelayMessage* dst, const MailRelayMessage* src) {
    if (!dst || !src) return false;
    mailrelay_message_init(dst);

    if (src->from) {
        if (!mailrelay_message_set_from(dst, src->from)) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    if (src->reply_to) {
        if (!mailrelay_message_set_reply_to(dst, src->reply_to)) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    for (int i = 0; i < src->to_count; i++) {
        if (!mailrelay_message_add_to(dst, src->to[i])) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    for (int i = 0; i < src->cc_count; i++) {
        if (!mailrelay_message_add_cc(dst, src->cc[i])) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    for (int i = 0; i < src->bcc_count; i++) {
        if (!mailrelay_message_add_bcc(dst, src->bcc[i])) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    if (src->subject) {
        dst->subject = strdup(src->subject);
        if (!dst->subject) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    if (src->text_body) {
        dst->text_body = strdup(src->text_body);
        if (!dst->text_body) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    if (src->html_body) {
        dst->html_body = strdup(src->html_body);
        if (!dst->html_body) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    if (src->template_key) {
        dst->template_key = strdup(src->template_key);
        if (!dst->template_key) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    if (src->headers) {
        dst->headers = strdup(src->headers);
        if (!dst->headers) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    if (src->idempotency_key) {
        dst->idempotency_key = strdup(src->idempotency_key);
        if (!dst->idempotency_key) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    if (src->debounce_key) {
        dst->debounce_key = strdup(src->debounce_key);
        if (!dst->debounce_key) {
            mailrelay_message_free(dst);
            return false;
        }
    }
    dst->priority = src->priority;
    return true;
}

bool mailrelay_is_valid_email(const char* email) {
    if (!email || strlen(email) == 0) return false;
    const char* at = strchr(email, '@');
    if (!at) return false;
    const char* dot = strchr(at + 1, '.');
    if (!dot) return false;
    if (at == email) return false;
    if (dot == at + 1) return false;
    if (strlen(dot + 1) == 0) return false;
    for (size_t i = 0; i < strlen(email); i++) {
        char c = email[i];
        if (!(isalnum((unsigned char)c) || c == '@' || c == '.' ||
              c == '_' || c == '-' || c == '+')) {
            return false;
        }
    }
    return true;
}

static bool add_recipient(char* arr[], int* count, const char* addr) {
    if (!arr || !count || !addr || !*addr) return false;
    if (*count >= MV_MAX_RECIPIENTS) return false;
    if (!mailrelay_is_valid_email(addr)) return false;
    arr[*count] = strdup(addr);
    (*count)++;
    return true;
}

bool mailrelay_message_set_from(MailRelayMessage* m, const char* from) {
    if (!m || !from || !mailrelay_is_valid_email(from)) return false;
    free(m->from);
    m->from = strdup(from);
    return true;
}

bool mailrelay_message_set_reply_to(MailRelayMessage* m, const char* reply_to) {
    if (!m || !reply_to || !mailrelay_is_valid_email(reply_to)) return false;
    free(m->reply_to);
    m->reply_to = strdup(reply_to);
    return true;
}

bool mailrelay_message_add_to(MailRelayMessage* m, const char* addr) {
    return add_recipient(m ? m->to : NULL, m ? &m->to_count : NULL, addr);
}

bool mailrelay_message_add_cc(MailRelayMessage* m, const char* addr) {
    return add_recipient(m ? m->cc : NULL, m ? &m->cc_count : NULL, addr);
}

bool mailrelay_message_add_bcc(MailRelayMessage* m, const char* addr) {
    return add_recipient(m ? m->bcc : NULL, m ? &m->bcc_count : NULL, addr);
}

int mailrelay_message_recipient_count(const MailRelayMessage* m) {
    if (!m) return 0;
    return m->to_count + m->cc_count + m->bcc_count;
}

char* mailrelay_message_recipients_to_json(const MailRelayMessage* m) {
    if (!m) return NULL;
    json_t* arr = json_array();
    if (!arr) return NULL;

    for (int i = 0; i < m->to_count; i++) {
        if (m->to[i]) json_array_append_new(arr, json_string(m->to[i]));
    }
    for (int i = 0; i < m->cc_count; i++) {
        if (m->cc[i]) json_array_append_new(arr, json_string(m->cc[i]));
    }
    for (int i = 0; i < m->bcc_count; i++) {
        if (m->bcc[i]) json_array_append_new(arr, json_string(m->bcc[i]));
    }

    char* result = json_dumps(arr, JSON_COMPACT);
    json_decref(arr);
    return result;
}

bool mailrelay_validate_message(const MailRelayMessage* m, char* err, size_t err_cap) {
    if (err && err_cap) err[0] = '\0';
    if (!m) {
        if (err) snprintf(err, err_cap, "null message");
        return false;
    }
    if (!m->from || !mailrelay_is_valid_email(m->from)) {
        if (err) snprintf(err, err_cap, "invalid or missing From address");
        return false;
    }
    if (mailrelay_message_recipient_count(m) == 0) {
        if (err) snprintf(err, err_cap, "no recipients specified");
        return false;
    }
    if (!m->subject) {
        if (err) snprintf(err, err_cap, "missing Subject");
        return false;
    }
    if (!m->text_body && !m->html_body) {
        if (err) snprintf(err, err_cap, "message has no body");
        return false;
    }
    if (strlen(m->subject) > 998) {
        if (err) snprintf(err, err_cap, "subject exceeds maximum length");
        return false;
    }
    return true;
}
