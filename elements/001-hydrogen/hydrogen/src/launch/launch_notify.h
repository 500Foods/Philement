/*
 * Launch Notify Subsystem Header
 * 
 * This header defines the interface for the notify subsystem launch functionality.
 */

#ifndef LAUNCH_NOTIFY_H
#define LAUNCH_NOTIFY_H

#include <stdbool.h>
#include "launch.h"

// Notification configuration limits
#define MIN_SMTP_PORT 1
#define MAX_SMTP_PORT 65535
#define MIN_SMTP_TIMEOUT 1
#define MAX_SMTP_TIMEOUT 300
#define MIN_SMTP_RETRIES 0
#define MAX_SMTP_RETRIES 10

// Function declarations
LaunchReadiness check_notify_launch_readiness(void);
int launch_notify_subsystem(void);

#endif /* LAUNCH_NOTIFY_H */