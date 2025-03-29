/*
 * Landing Review Header
 * 
 * This module defines interfaces for final landing (shutdown) status reporting.
 * It provides functions for:
 * - Reporting landing status for each subsystem
 * - Tracking shutdown time and thread cleanup
 * - Providing summary statistics
 * - Handling final landing checks
 */

#ifndef LANDING_REVIEW_H
#define LANDING_REVIEW_H

// System includes
#include <time.h>

// Local includes
#include "landing.h"

// Review and report final landing status
void handle_landing_review(const ReadinessResults* results, time_t start_time);

// Status reporting functions
void report_subsystem_landing_status(const char* subsystem, bool landed);
void report_thread_cleanup_status(void);
void report_final_landing_summary(const ReadinessResults* results);

#endif /* LANDING_REVIEW_H */