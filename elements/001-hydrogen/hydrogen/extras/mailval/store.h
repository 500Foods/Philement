/*
 * Shared in-memory mailbox store. SMTP delivery appends messages here; IMAP
 * and JMAP serve from here. Single global instance, mutex-protected.
 */

#ifndef MV_STORE_H
#define MV_STORE_H

#include <stdbool.h>

#define MV_FLAG_SEEN     (1 << 0)
#define MV_FLAG_ANSWERED (1 << 1)
#define MV_FLAG_FLAGGED  (1 << 2)
#define MV_FLAG_DELETED  (1 << 3)
#define MV_FLAG_DRAFT    (1 << 4)
#define MV_FLAG_RECENT   (1 << 5)

typedef struct {
    char* id;          /* message/blob id, e.g. "M1" */
    char* from;
    char* to;
    char* subject;
    char* date;        /* internaldate (RFC3339 UTC) */
    char* raw;         /* full RFC822 message */
    long raw_len;
    int flags;
    unsigned int uid;  /* IMAP uid within its mailbox */
} mv_message;

typedef struct {
    char* name;            /* "INBOX" */
    char* id;              /* mailbox id, e.g. "MB1" */
    unsigned int uidvalidity;
    unsigned int uidnext;
    int exists;            /* messages not \Deleted */
    mv_message** msgs;
    int msg_count;
    int msg_capacity;
} mv_mailbox;

typedef struct {
    mv_mailbox** mboxes;
    int mbox_count;
    int mbox_capacity;
    char account_id[64];
    unsigned int next_mid;   /* message id counter */
    unsigned int next_mbid;  /* mailbox id counter */
} mv_store;

void store_init(void);
void store_lock(void);
void store_unlock(void);
mv_store* store_get(void);

/* Get mailbox by name; create (INBOX or any) if missing when create=true. */
mv_mailbox* store_mailbox_get(const char* name, bool create);

/* Append a message to a mailbox (copies inputs). Returns assigned uid (>0). */
unsigned int store_append(mv_mailbox* mb, const char* from, const char* to,
                          const char* subject, const char* date,
                          const char* raw, long raw_len, int flags);

/* Allocate the next message id string (caller frees). */
char* store_next_msg_id(void);

/* Find message by id across all mailboxes (caller holds lock). */
mv_message* store_find_message(const char* id);

#endif /* MV_STORE_H */
