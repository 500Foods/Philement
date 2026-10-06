/*
 * NATS subject builder.
 *
 * Config stores a suffix. The wire name is cluster.<ClusterId>.<suffix>.
 */

#ifndef NATS_SUBJECT_H
#define NATS_SUBJECT_H

#include <stdbool.h>

/* Suffix rules for cluster.<ClusterId>.<suffix>. Rejects empty text,
 * a cluster. prefix, and space, *, or >. */
bool nats_event_suffix_ok(const char *suffix);
char *nats_subject_build(const char *cluster_id, const char *suffix);

#endif /* NATS_SUBJECT_H */
