/*
 * Log Fan-Out Consumer
 *
 * Drains the "SystemLog" queue (populated by log_this in logging.c) and fans
 * each entry out to the configured destinations: console, file, and mailrelay.
 * Replaces the orphaned log_queue_manager.c, which was intended to be this
 * consumer thread but was never spawned - leaving the SystemLog queue with no
 * reader (queued entries were silently dropped and console output was
 * suppressed inline in log_this).
 *
 * See log_fanout.h for the public API.
 */

#include <src/hydrogen.h>
#include <src/logging/log_fanout.h>

#include <src/queue/queue.h>
#include <src/config/config_logging.h>
#include <src/mailrelay/mailrelay.h>

#include <jansson.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// Shared logging state (declared in logging.c / state.c).
extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Local mirror of logging.c's label widths so this module stays self-contained.
#define FANOUT_PRIORITY_WIDTH 8
#define FANOUT_SUBSYSTEM_WIDTH 16

const char* fanout_priority_label(int priority) {
    switch (priority) {
        case LOG_LEVEL_TRACE:  return "TRACE";
        case LOG_LEVEL_DEBUG:  return "DEBUG";
        case LOG_LEVEL_STATE:  return "STATE";
        case LOG_LEVEL_ALERT:  return "ALERT";
        case LOG_LEVEL_ERROR:  return "ERROR";
        case LOG_LEVEL_FATAL:  return "FATAL";
        case LOG_LEVEL_QUIET:  return "QUIET";
        default:               return "STATE";
    }
}

/*
 * Destination gating, mirroring the legacy should_log_to_* helpers. Returns
 * true when the given destination is enabled and the message priority meets or
 * exceeds the destination's configured minimum level.
 */
bool dest_enabled(const LoggingDestConfig* dest, int priority) {
    if (!dest || !dest->enabled) {
        return false;
    }
    if (priority < dest->default_level) {
        return false;
    }
    return true;
}

// Format a single log entry the way the console/file sinks expect to see it.
void format_entry(char* out, size_t out_size, const char* subsystem,
                         const char* details, int priority) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm_info;
    gmtime_r(&tv.tv_sec, &tm_info);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_info);
    char ts_ms[48];
    snprintf(ts_ms, sizeof(ts_ms), "%s.%03dZ", timestamp, (int)(tv.tv_usec / 1000));

    char prio[FANOUT_PRIORITY_WIDTH + 5];
    snprintf(prio, sizeof(prio), "[ %-*s ]", FANOUT_PRIORITY_WIDTH, fanout_priority_label(priority));
    char sub[FANOUT_SUBSYSTEM_WIDTH + 5];
    snprintf(sub, sizeof(sub), "[ %-*s ]", FANOUT_SUBSYSTEM_WIDTH, subsystem ? subsystem : "?");

    snprintf(out, out_size, "%s  %s  %s  %s\n", ts_ms, prio, sub, details ? details : "");
}

// Fan a single parsed entry out to mailrelay (async, non-blocking).
void fanout_to_mailrelay(const char* subsystem, const char* details, int priority) {
    (void)subsystem;
    if (!app_config || !app_config->mail_relay.Enabled) {
        return;
    }
    if (app_config->mail_relay.AdminRecipientCount <= 0) {
        return;
    }

    MailRelaySendDirectRequest req;
    memset(&req, 0, sizeof(req));

    // Build the recipient array from the configured admin recipients.
    const char** recipients = calloc((size_t)app_config->mail_relay.AdminRecipientCount, sizeof(char*));
    if (!recipients) {
        return;
    }
    for (int i = 0; i < app_config->mail_relay.AdminRecipientCount; i++) {
        recipients[i] = app_config->mail_relay.AdminRecipients[i];
    }
    req.to = recipients;
    req.to_count = app_config->mail_relay.AdminRecipientCount;
    req.from = app_config->mail_relay.DefaultFrom; // NULL -> mailrelay uses its default
    req.subject = "Hydrogen log alert";
    req.text_body = details;
    req.priority = priority;

    char err[256];
    MailRelayStatus status = mailrelay_send_direct(&req, NULL, err, sizeof(err));
    free(recipients);

    // MAILRELAY_OK == 0. Anything else (DISABLED/SHUTDOWN/QUEUE_FULL/...) is
    // treated as drop-and-continue: the log queue must never block on email.
    (void)status;
}

// Fan a single parsed entry out to the configured destinations.
void fanout_entry(const char* raw) {
    if (!raw || !app_config) {
        return;
    }

    json_error_t error;
    json_t* json = json_loads(raw, 0, &error);
    if (!json) {
        return;
    }

    const char* subsystem = json_string_value(json_object_get(json, "subsystem"));
    const char* details = json_string_value(json_object_get(json, "details"));
    int priority = (int)json_integer_value(json_object_get(json, "priority"));

    bool log_console = json_boolean_value(json_object_get(json, "LogConsole"));
    bool log_file = json_boolean_value(json_object_get(json, "LogFile"));
    bool log_notify = json_boolean_value(json_object_get(json, "LogNotify"));

    char entry[1024];
    format_entry(entry, sizeof(entry), subsystem, details, priority);

    if (log_console && dest_enabled(&app_config->logging.console, priority)) {
        // During normal operation the queue consumer is THE console sink.
        fputs(entry, stdout);
        fflush(stdout);
    }

    if (log_file && dest_enabled(&app_config->logging.file, priority)) {
        const char* path = app_config->server.log_file;
        if (path) {
            FILE* fp = fopen(path, "a");
            if (fp) {
                fputs(entry, fp);
                fclose(fp);
            }
        }
    }

    if (log_notify && dest_enabled(&app_config->logging.notify, priority)) {
        fanout_to_mailrelay(subsystem, details, priority);
    }

    json_decref(json);
}

static Queue* g_log_queue = NULL;
static pthread_t g_log_fanout_thread;
static volatile sig_atomic_t g_log_fanout_running = 0;

void* log_fanout_thread(void* arg) {
    (void)arg;

    while (!log_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        while (queue_size(g_log_queue) == 0 && !log_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        // Drain everything currently available.
        size_t size;
        int priority;
        char* data = queue_dequeue(g_log_queue, &size, &priority);
        while (data) {
            fanout_entry(data);
            free(data);
            data = queue_dequeue_nonblocking(g_log_queue, &size, &priority);
        }
    }

    return NULL;
}

bool init_log_fanout(void) {
    QueueAttributes attrs = {0};
    g_log_queue = queue_create("SystemLog", &attrs);
    if (!g_log_queue) {
        return false;
    }

    g_log_fanout_running = 1;
    if (pthread_create(&g_log_fanout_thread, NULL, log_fanout_thread, NULL) != 0) {
        g_log_fanout_running = 0;
        return false;
    }

    return true;
}

bool shutdown_log_fanout(void) {
    if (!g_log_fanout_running) {
        return true;
    }
    g_log_fanout_running = 0;

    log_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);

    bool ok = true;
    if (g_log_fanout_thread) {
        if (pthread_join(g_log_fanout_thread, NULL) != 0) {
            ok = false;
        }
        g_log_fanout_thread = 0;
    }
    return ok;
}
