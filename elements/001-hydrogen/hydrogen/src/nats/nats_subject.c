/*
 * Build a cluster subject from a config suffix.
 */

#include <src/hydrogen.h>

#include <src/nats/nats_subject.h>

char *nats_subject_build(const char *cluster_id, const char *suffix) {
    size_t i;
    size_t id_len;
    size_t suffix_len;
    size_t total;
    char *out;

    if (!cluster_id || cluster_id[0] == '\0' || !suffix || suffix[0] == '\0') {
        return NULL;
    }
    if (strncmp(suffix, "cluster.", 8) == 0) {
        return NULL;
    }

    for (i = 0; cluster_id[i] != '\0'; i++) {
        unsigned char c = (unsigned char)cluster_id[i];
        if (c <= ' ' || c == '.' || c == '*' || c == '>') {
            return NULL;
        }
    }
    for (i = 0; suffix[i] != '\0'; i++) {
        unsigned char c = (unsigned char)suffix[i];
        if (c <= ' ' || c == '*' || c == '>') {
            return NULL;
        }
    }

    id_len = strlen(cluster_id);
    suffix_len = strlen(suffix);
    total = strlen("cluster.") + id_len + 1 + suffix_len + 1;
    out = malloc(total);
    if (!out) {
        return NULL;
    }
    snprintf(out, total, "cluster.%s.%s", cluster_id, suffix);
    return out;
}
