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
#include "utils_threads.h"
#include "utils_queue.h"
#include "utils_status.h"
#include "utils_time.h"
#include "utils_logging.h"

#endif // UTILS_H