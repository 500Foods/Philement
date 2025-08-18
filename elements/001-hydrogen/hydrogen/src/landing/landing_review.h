/*
 * Landing Review Header
 * 
 * DESIGN PRINCIPLES:
 * - Minimal public interface - only expose orchestration functions
 * - Implementation details remain private
 * - Status reporting is handled internally
 * - Follows launch architecture patterns
 *
 * This module provides the main landing review orchestration function
 * which coordinates the final phase of the landing sequence.
 */

#ifndef LANDING_REVIEW_H
#define LANDING_REVIEW_H

// System includes
#include <time.h>

// Local includes
#include "landing.h"

/*
 * Main landing review orchestration function
 * Coordinates the final phase of landing, including:
 * - Timing assessment
 * - Thread cleanup verification
 * - Status reporting
 * - Final system state review
 */
void handle_landing_review(const ReadinessResults* results, time_t start_time);

#endif /* LANDING_REVIEW_H */
