#include "store.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static mv_store g_store;
static pthread_mutex_t g_store_lock;

void store_init(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_store_lock, &attr);
    pthread_mutexattr_destroy(&attr);
    memset(&g_store, 0, sizeof(g_store));
    snprintf(g_store.account_id, sizeof(g_store.account_id), "acct1");
    g_store.next_mid = 1;
    g_store.next_mbid = 1;
    /* Seed INBOX. */
    store_mailbox_get("INBOX", true);
}

void store_lock(void) { pthread_mutex_lock(&g_store_lock); }
void store_unlock(void) { pthread_mutex_unlock(&g_store_lock); }
mv_store* store_get(void) { return &g_store; }

mv_mailbox* store_mailbox_get(const char* name, bool create) {
    store_lock();
    for (int i = 0; i < g_store.mbox_count; i++) {
        if (strcmp(g_store.mboxes[i]->name, name) == 0) {
            mv_mailbox* mb = g_store.mboxes[i];
            store_unlock();
            return mb;
        }
    }
    if (!create) { store_unlock(); return NULL; }
    mv_mailbox* mb = mv_xcalloc(1, sizeof(*mb));
    mb->name = mv_xstrdup(name);
    mb->id = mv_xmalloc(16);
    snprintf(mb->id, 16, "MB%u", g_store.next_mbid++);
    mb->uidvalidity = 1;
    mb->uidnext = 1;
    if (g_store.mbox_count >= g_store.mbox_capacity) {
        g_store.mbox_capacity = g_store.mbox_capacity ? g_store.mbox_capacity * 2 : 8;
        g_store.mboxes = mv_xrealloc(g_store.mboxes, g_store.mbox_capacity * sizeof(*g_store.mboxes));
    }
    g_store.mboxes[g_store.mbox_count++] = mb;
    store_unlock();
    return mb;
}

char* store_next_msg_id(void) {
    store_lock();
    char* id = mv_xmalloc(16);
    snprintf(id, 16, "M%u", g_store.next_mid++);
    store_unlock();
    return id;
}

mv_message* store_find_message(const char* id) {
    for (int i = 0; i < g_store.mbox_count; i++) {
        mv_mailbox* mb = g_store.mboxes[i];
        for (int j = 0; j < mb->msg_count; j++) {
            if (strcmp(mb->msgs[j]->id, id) == 0) return mb->msgs[j];
        }
    }
    return NULL;
}

unsigned int store_append(mv_mailbox* mb, const char* from, const char* to,
                          const char* subject, const char* date,
                          const char* raw, long raw_len, int flags) {
    store_lock();
    mv_message* m = mv_xcalloc(1, sizeof(*m));
    m->id = store_next_msg_id();
    m->from = mv_xstrdup(from ? from : "");
    m->to = mv_xstrdup(to ? to : "");
    m->subject = mv_xstrdup(subject ? subject : "");
    m->date = mv_xstrdup(date ? date : "");
    m->raw = mv_xmalloc(raw_len > 0 ? (size_t)raw_len + 1 : 1);
    memcpy(m->raw, raw ? raw : "", (size_t)(raw_len > 0 ? raw_len : 0));
    m->raw[raw_len > 0 ? raw_len : 0] = '\0';
    m->raw_len = raw_len;
    m->flags = flags | MV_FLAG_RECENT;
    m->uid = mb->uidnext++;
    if (mb->msg_count >= mb->msg_capacity) {
        mb->msg_capacity = mb->msg_capacity ? mb->msg_capacity * 2 : 8;
        mb->msgs = mv_xrealloc(mb->msgs, mb->msg_capacity * sizeof(*mb->msgs));
    }
    mb->msgs[mb->msg_count++] = m;
    mb->exists++;
    unsigned int uid = m->uid;
    store_unlock();
    return uid;
}
