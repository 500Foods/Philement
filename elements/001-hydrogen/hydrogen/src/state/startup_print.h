/*
 * Print Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the print subsystem.
 */

#ifndef HYDROGEN_STARTUP_PRINT_H
#define HYDROGEN_STARTUP_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize print subsystem
 * 
 * This function initializes the print queue system and starts
 * the print queue manager thread.
 * 
 * @return 1 on success, 0 on failure
 */
int init_print_subsystem(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_PRINT_H */