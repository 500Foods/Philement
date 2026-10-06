/*
 * NATS subject builder.
 *
 * Config stores a suffix. The wire name is cluster.<ClusterId>.<suffix>.
 */

#ifndef NATS_SUBJECT_H
#define NATS_SUBJECT_H

char *nats_subject_build(const char *cluster_id, const char *suffix);

#endif /* NATS_SUBJECT_H */
