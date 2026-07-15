/*
 * Mail Relay worker pool.
 *
 * Spawns configurable worker threads that dequeue messages and send them
 * via mailrelay_send_raw. Workers register with the global service thread
 * tracking and unregister on exit.
 */

#ifndef MAILRELAY_WORKERS_H
#define MAILRELAY_WORKERS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Start the configured number of worker threads.
 *
 * @param count Number of worker threads to spawn (must be > 0).
 * @return true if workers were started, false if the runtime is not
 *         initialized or thread creation failed.
 */
bool mailrelay_workers_start(int count);

/*
 * Signal all worker threads to stop. Workers will finish their current
 * dequeue/send cycle and then exit. Call mailrelay_shutdown() to wait for
 * the drain to complete.
 */
void mailrelay_workers_stop(void);

/*
 * Worker thread entry point. Exposed (non-static) so the Unity test suite can
 * drive a worker directly. Not part of the stable public API.
 */
void* mailrelay_worker_thread(void* arg);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_WORKERS_H */
