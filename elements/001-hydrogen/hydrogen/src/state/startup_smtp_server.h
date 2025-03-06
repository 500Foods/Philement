/*
 * SMTP Server Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the SMTP server subsystem.
 * The SMTP server provides email notification capabilities for system events.
 */

#ifndef HYDROGEN_STARTUP_SMTP_SERVER_H
#define HYDROGEN_STARTUP_SMTP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SMTP server subsystem
 * 
 * This function initializes the SMTP server for email notifications.
 * It provides email capabilities:
 * - Send print job notifications
 * - Alert on system events
 * - Deliver error reports
 * - Handle maintenance notifications
 * 
 * Requires network connectivity and proper SMTP configuration
 * for email delivery.
 * 
 * @return 1 on success, 0 on failure
 */
int init_smtp_server_subsystem(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_SMTP_SERVER_H */