/*
 * Mail Relay Subsystem Launch Interface
 */

#ifndef LAUNCH_MAIL_RELAY_H
#define LAUNCH_MAIL_RELAY_H

#include "launch.h"

/**
 * @brief Initialize the Mail Relay subsystem
 * @returns 1 on success, 0 on failure
 */
int launch_mail_relay_subsystem(void);

LaunchReadiness check_mail_relay_launch_readiness(void);

#endif /* LAUNCH_MAIL_RELAY_H */
