/*
 * Log Fan-Out Consumer
 *
 * The logging subsystem enqueues formatted log entries onto the "SystemLog"
 * queue (see logging.c). This module provides the long-missing consumer that
 * drains that queue and fans each entry out to the configured destinations:
 *
 *   - console : stdout/stderr (the steady-state console sink; logging.c
 *               suppresses its inline console write when the enqueue succeeds,
 *               so the consumer is the one that actually prints)
 *   - file    : append to app_config->server.log_file
 *   - mailrelay : async email via mailrelay_send_direct() when the notify
 *               logging destination is enabled
 *
 * Historically log_queue_manager.c was intended to fill this role but was
 * never spawned, so the SystemLog queue had no consumer and queued entries
 * were silently dropped. This module replaces it with a properly wired,
 * thread-safe consumer.
 */

#ifndef LOG_FANOUT_H
#define LOG_FANOUT_H

#include <stdbool.h>

#include <src/config/config_logging.h>

/*
 * Create the SystemLog queue (if not already present) and spawn the fan-out
 * consumer thread. Must be called after queue_system_init() and after the
 * configuration has been loaded (the consumer reads app_config for destination
 * routing). Returns true on success.
 */
bool init_log_fanout(void);

/*
 * Signal the consumer to stop, then join it. Safe to call even if
 * init_log_fanout() was never called. Returns true on success.
 */
bool shutdown_log_fanout(void);

/*
 * The following helpers are exposed (non-static) primarily so the Unity test
 * suite can exercise priority labeling, destination gating, entry formatting,
 * mailrelay fan-out, and per-entry routing directly. They are not part of the
 * stable public API but must remain callable for unit testing (per the
 * project's no-static-callable-functions rule).
 */

/* Map a numeric LOG_LEVEL_* to its short label (default "STATE"). */
const char* fanout_priority_label(int priority);

/* True when dest is enabled and priority meets/exceeds dest->default_level. */
bool dest_enabled(const LoggingDestConfig* dest, int priority);

/* Format one log entry the way the console/file sinks expect to see it. */
void format_entry(char* out, size_t out_size,
                  const char* subsystem, const char* details, int priority);

/* Fan a single parsed entry out to mailrelay (async, non-blocking). */
void fanout_to_mailrelay(const char* subsystem, const char* details, int priority);

/* Fan a single raw JSON queue entry out to the configured destinations. */
void fanout_entry(const char* raw);

/* Consumer thread entry point: drains SystemLog and fans out each entry. */
void* log_fanout_thread(void* arg);

#endif // LOG_FANOUT_H
