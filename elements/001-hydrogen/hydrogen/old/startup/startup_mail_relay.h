/*
 * Mail Relay Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the Mail relay subsystem.
 * The Mail relay provides email notification capabilities for system events.
 */

#ifndef HYDROGEN_STARTUP_MAIL_RELAY_H
#define HYDROGEN_STARTUP_MAIL_RELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize Mail relay subsystem
 * 
 * This function initializes the Mail relay for email notifications.
 * It provides email capabilities:
 * - Send print job notifications
 * - Alert on system events
 * - Deliver error reports
 * - Handle maintenance notifications
 * 
 * Requires network connectivity and proper mail configuration
 * for email delivery.
 * 
 * @return 1 on success, 0 on failure
 */
int init_mail_relay_subsystem(void);

/**
 * Shut down the Mail relay subsystem
 * 
 * This function performs cleanup and shutdown of the Mail relay system.
 * It ensures proper resource release and termination of mail operations.
 * 
 * Actions performed:
 * - Close active connections
 * - Flush mail queue
 * - Free allocated resources
 * - Clean up temporary files
 */
void shutdown_mail_relay(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_MAIL_RELAY_H */