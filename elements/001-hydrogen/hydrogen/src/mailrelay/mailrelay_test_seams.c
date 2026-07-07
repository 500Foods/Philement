/*
 * Mail Relay test seams implementation.
 */

#include <src/mailrelay/mailrelay_test_seams.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

static time_t default_seam_time(void) {
    return time(NULL);
}

static void default_seam_message_id(char* buffer, size_t buflen, const char* app_name) {
    (void)snprintf(buffer, buflen, "%lX.%lX@%s",
                   (unsigned long)time(NULL),
                   (unsigned long)pthread_self(),
                   app_name && app_name[0] ? app_name : "hydrogen");
}

mailrelay_seam_time_fn mailrelay_seam_time = default_seam_time;
mailrelay_seam_message_id_fn mailrelay_seam_message_id = default_seam_message_id;

void mailrelay_set_seam_time(mailrelay_seam_time_fn fn) {
    if (fn) {
        mailrelay_seam_time = fn;
    }
}

void mailrelay_set_seam_message_id(mailrelay_seam_message_id_fn fn) {
    if (fn) {
        mailrelay_seam_message_id = fn;
    }
}

void mailrelay_reset_seams(void) {
    mailrelay_seam_time = default_seam_time;
    mailrelay_seam_message_id = default_seam_message_id;
}
