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
    free(m->idempotency_key);
    for (int i = 0; i < m->to_count; i++) free(m->to[i]);
    for (int i = 0; i < m->cc_count; i++) free(m->cc[i]);
    for (int i = 0; i < m->bcc_count; i++) free(m->bcc[i]);
    memset(m, 0, sizeof(*m));
}

bool mailrelay_is_valid_email(const char* email) {
    if (!email || strlen(email) == 0 || strlen(email) > 254) return false;
    const char* at = strchr(email, '@');
    if (!at) return false;
    const char* dot = strchr(at + 1, '.');
    if (!dot) return false;
    if (at == email) return false;                /* no local part */
    if (dot == at + 1) return false;             /* no domain part */
    if (strlen(dot + 1) == 0) return false;      /* no TLD */
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
    arr[*count] = mv_xstrdup(addr);
    (*count)++;
    return true;
}

bool mailrelay_message_set_from(MailRelayMessage* m, const char* from) {
    if (!m || !from || !mailrelay_is_valid_email(from)) return false;
    free(m->from);
    m->from = mv_xstrdup(from);
    return true;
}

bool mailrelay_message_set_reply_to(MailRelayMessage* m, const char* reply_to) {
    if (!m || !reply_to || !mailrelay_is_valid_email(reply_to)) return false;
    free(m->reply_to);
    m->reply_to = mv_xstrdup(reply_to);
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
