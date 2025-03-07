/*
 * Logging Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the logging subsystem.
 */

#ifndef HYDROGEN_STARTUP_LOGGING_H
#define HYDROGEN_STARTUP_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize logging subsystem
 * 
 * Requires app_config to be loaded and available before calling.
 * @return 1 on success, 0 on failure
 */
int init_logging_subsystem(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_LOGGING_H */