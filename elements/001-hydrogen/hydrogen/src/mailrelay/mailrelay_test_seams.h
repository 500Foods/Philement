/*
 * Mail Relay test seams.
 *
 * Provides injectable time() and Message-ID sources so render and send
 * paths are deterministic in unit tests without branching in production.
 */

#ifndef MAILRELAY_TEST_SEAMS_H
#define MAILRELAY_TEST_SEAMS_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef time_t (*mailrelay_seam_time_fn)(void);
typedef void (*mailrelay_seam_message_id_fn)(char* buffer, size_t buflen, const char* app_name);

extern mailrelay_seam_time_fn mailrelay_seam_time;
extern mailrelay_seam_message_id_fn mailrelay_seam_message_id;

void mailrelay_set_seam_time(mailrelay_seam_time_fn fn);
void mailrelay_set_seam_message_id(mailrelay_seam_message_id_fn fn);
void mailrelay_reset_seams(void);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_TEST_SEAMS_H */
