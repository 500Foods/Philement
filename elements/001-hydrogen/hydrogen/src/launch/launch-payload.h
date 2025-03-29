/*
 * Payload Subsystem Launch Readiness Check Header
 * 
 * This header defines the interface for the payload subsystem 
 * launch readiness check.
 */

#ifndef LAUNCH_PAYLOAD_H
#define LAUNCH_PAYLOAD_H

#include "launch.h"

/**
 * Check if the payload subsystem is ready to launch
 * 
 * This function performs various checks to determine if all the
 * prerequisites for the payload subsystem are satisfied, including:
 * - Configuration loaded
 * - Payload attached to executable
 * - Key availability
 * - Payload accessibility
 * - Payload size determination
 * 
 * @return LaunchReadiness struct with readiness flags and messages
 */
LaunchReadiness check_payload_launch_readiness(void);

/**
 * Launch the payload subsystem
 * 
 * This function launches the payload subsystem by extracting and
 * processing the payload from the executable.
 * 
 * @return 1 if payload was successfully launched, 0 on failure
 */
int launch_payload_subsystem(void);

/**
 * Free resources allocated during payload launch
 * 
 * This function frees any resources allocated during the payload launch phase.
 * It should be called during the LANDING: PAYLOAD phase of the application.
 */
void free_payload_resources(void);

#endif /* LAUNCH_PAYLOAD_H */