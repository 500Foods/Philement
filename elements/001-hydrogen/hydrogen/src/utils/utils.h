/*
 * Core Utilities for High-Precision 3D Printing
 * 
 * This header serves as a facade for the utility subsystem,
 * providing access to all utility functions through a single include.
 * 
 * The implementation has been split into focused modules:
 * - Thread Management (utils_threads)
 * - Queue Operations (utils_queue)
 * - System Status (utils_status)
 * - Time Operations (utils_time)
 * - Logging Functions (utils_logging)
 */

#ifndef UTILS_H
#define UTILS_H

// Include all utility module headers
#include "utils_threads.h"  // Local include since we're in utils/
#include "utils_queue.h"    // Local include since we're in utils/
#include "utils_status.h"   // Local include since we're in utils/
#include "utils_time.h"     // Local include since we're in utils/
#include "utils_logging.h"  // Local include since we're in utils/

#endif // UTILS_H