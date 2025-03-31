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

// Core landing functions
bool check_all_landing_readiness(void);

#endif /* LANDING_H */