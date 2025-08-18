/*
 * Landing System Header
 * 
 * This module defines the interfaces for the landing (shutdown) system.
 * It coordinates the shutdown sequence through specialized modules:
 * - landing_readiness.c - Handles readiness checks
 * - landing_plan.c - Coordinates landing sequence
 * - landing_review.c - Reports landing status
 */

#ifndef LANDING_H
#define LANDING_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Project includes
#include "../state/state_types.h"  // For shared types
#include <signal.h>  // For sig_atomic_t

// Restart control
extern volatile sig_atomic_t restart_requested;
extern volatile int restart_count;

// Core landing functions
bool check_all_landing_readiness(void);
void handle_sighup(void);  // SIGHUP signal handler for restart
void handle_sigint(void);  // SIGINT signal handler for shutdown

#endif /* LANDING_H */
