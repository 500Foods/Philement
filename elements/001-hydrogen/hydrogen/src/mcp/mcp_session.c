#include <src/hydrogen.h>
#include <src/mcp/mcp_session.h>
#include <src/mcp/mcp_stats.h>
#include <src/utils/utils_uuid.h>

#include <string.h>

pthread_mutex_t mcp_session_lock;
bool mcp_session_ready = false;
bool mcp_session_use_override = false;
time_t mcp_session_override_now = 0;
McpSessionEntry *mcp_session_head = NULL;

void mcp_session_clear_now(void) {
    mcp_session_use_override = false;
    mcp_session_override_now = 0;
}

void mcp_session_set_now(time_t now) {
    mcp_session_use_override = true;
    mcp_session_override_now = now;
}

time_t mcp_session_now(void) {
    if (mcp_session_use_override) {
        return mcp_session_override_now;
    }
    return time(NULL);
}

void mcp_session_free_entry(McpSessionEntry *entry) {
    if (!entry) {
        return;
    }
    free(entry->id);
    free(entry->sub);
    free(entry);
}

void mcp_session_init(void) {
    if (mcp_session_ready) {
        return;
    }
    pthread_mutex_init(&mcp_session_lock, NULL);
    mcp_session_head = NULL;
    mcp_session_ready = true;
}

void mcp_session_shutdown(void) {
    McpSessionEntry *cur;
    McpSessionEntry *next;

    if (!mcp_session_ready) {
        return;
    }
    pthread_mutex_lock(&mcp_session_lock);
    cur = mcp_session_head;
    mcp_session_head = NULL;
    pthread_mutex_unlock(&mcp_session_lock);
    while (cur) {
        next = cur->next;
        mcp_session_free_entry(cur);
        cur = next;
    }
    pthread_mutex_destroy(&mcp_session_lock);
    mcp_session_ready = false;
    mcp_session_clear_now();
}

void mcp_session_ensure(void) {
    if (!mcp_session_ready) {
        mcp_session_init();
    }
}

int mcp_session_count(void) {
    McpSessionEntry *cur;
    int n = 0;

    if (!mcp_session_ready) {
        return 0;
    }
    pthread_mutex_lock(&mcp_session_lock);
    for (cur = mcp_session_head; cur; cur = cur->next) {
        n++;
    }
    pthread_mutex_unlock(&mcp_session_lock);
    return n;
}

int mcp_session_reap(int idle_timeout_seconds) {
    McpSessionEntry *cur;
    McpSessionEntry *prev;
    McpSessionEntry *gone;
    time_t now;
    int expired = 0;

    if (!mcp_session_ready || idle_timeout_seconds <= 0) {
        return 0;
    }
    now = mcp_session_now();
    pthread_mutex_lock(&mcp_session_lock);
    prev = NULL;
    cur = mcp_session_head;
    while (cur) {
        if ((now - cur->last_seen) >= (time_t)idle_timeout_seconds) {
            gone = cur;
            cur = cur->next;
            if (prev) {
                prev->next = cur;
            } else {
                mcp_session_head = cur;
            }
            mcp_session_free_entry(gone);
            expired++;
            mcp_stats_inc_sessions_expired();
            mcp_stats_add_sessions_active(-1);
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
    pthread_mutex_unlock(&mcp_session_lock);
    return expired;
}

McpSessionEntry *mcp_session_find_locked(const char *id) {
    McpSessionEntry *cur;

    if (!id) {
        return NULL;
    }
    for (cur = mcp_session_head; cur; cur = cur->next) {
        if (cur->id && strcmp(cur->id, id) == 0) {
            return cur;
        }
    }
    return NULL;
}

McpSessionResult mcp_session_create_locked(const char *sub, int max_sessions, char **out_id) {
    McpSessionEntry *entry;
    McpSessionEntry *cur;
    char uuid[UUID_STR_LEN];
    int n = 0;
    int cap;

    cap = max_sessions > 0 ? max_sessions : 256;
    for (cur = mcp_session_head; cur; cur = cur->next) {
        n++;
    }
    if (n >= cap) {
        return MCP_SESSION_LIMIT;
    }
    generate_uuid(uuid);
    entry = calloc(1, sizeof(*entry));
    if (!entry) {
        return MCP_SESSION_LIMIT;
    }
    entry->id = strdup(uuid);
    entry->sub = strdup(sub ? sub : "");
    entry->last_seen = mcp_session_now();
    if (!entry->id || !entry->sub) {
        mcp_session_free_entry(entry);
        return MCP_SESSION_LIMIT;
    }
    entry->next = mcp_session_head;
    mcp_session_head = entry;
    mcp_stats_inc_sessions_total();
    mcp_stats_add_sessions_active(1);
    if (out_id) {
        *out_id = strdup(entry->id);
    }
    return MCP_SESSION_CREATED;
}

McpSessionResult mcp_session_resolve(const char *incoming_id, const char *sub,
                                     bool allow_create, int max_sessions,
                                     int idle_timeout_seconds, char **out_id) {
    McpSessionEntry *found;
    McpSessionResult created;
    const char *want;

    if (out_id) {
        *out_id = NULL;
    }
    mcp_session_ensure();
    mcp_session_reap(idle_timeout_seconds);
    want = sub ? sub : "";

    pthread_mutex_lock(&mcp_session_lock);
    if (incoming_id && incoming_id[0] != '\0') {
        found = mcp_session_find_locked(incoming_id);
        if (found) {
            if (strcmp(found->sub ? found->sub : "", want) != 0) {
                pthread_mutex_unlock(&mcp_session_lock);
                return MCP_SESSION_HIJACK;
            }
            found->last_seen = mcp_session_now();
            if (out_id) {
                *out_id = strdup(found->id);
            }
            pthread_mutex_unlock(&mcp_session_lock);
            return MCP_SESSION_OK;
        }
        if (!allow_create) {
            pthread_mutex_unlock(&mcp_session_lock);
            return MCP_SESSION_UNKNOWN;
        }
    } else if (!allow_create) {
        pthread_mutex_unlock(&mcp_session_lock);
        return MCP_SESSION_UNKNOWN;
    }
    created = mcp_session_create_locked(want, max_sessions, out_id);
    pthread_mutex_unlock(&mcp_session_lock);
    return created;
}

McpSessionResult mcp_session_delete(const char *id, const char *sub) {
    McpSessionEntry *cur;
    McpSessionEntry *prev;
    const char *want;

    mcp_session_ensure();
    if (!id || id[0] == '\0') {
        return MCP_SESSION_UNKNOWN;
    }
    want = sub ? sub : "";
    pthread_mutex_lock(&mcp_session_lock);
    prev = NULL;
    cur = mcp_session_head;
    while (cur) {
        if (cur->id && strcmp(cur->id, id) == 0) {
            if (strcmp(cur->sub ? cur->sub : "", want) != 0) {
                pthread_mutex_unlock(&mcp_session_lock);
                return MCP_SESSION_HIJACK;
            }
            if (prev) {
                prev->next = cur->next;
            } else {
                mcp_session_head = cur->next;
            }
            mcp_session_free_entry(cur);
            mcp_stats_add_sessions_active(-1);
            pthread_mutex_unlock(&mcp_session_lock);
            return MCP_SESSION_DELETED;
        }
        prev = cur;
        cur = cur->next;
    }
    pthread_mutex_unlock(&mcp_session_lock);
    return MCP_SESSION_UNKNOWN;
}
