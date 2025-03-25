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

#endif /* LAUNCH_PAYLOAD_H */