/*
 * File Descriptor Configuration
 *
 * Defines constants and types for file descriptor management.
 * These are used by the status reporting system to track open
 * file descriptors and their purposes.
 */

#ifndef HYDROGEN_CONFIG_FD_H
#define HYDROGEN_CONFIG_FD_H

// File descriptor type and description size limits
#define DEFAULT_FD_TYPE_SIZE 32        // Size for file descriptor type strings
#define DEFAULT_FD_DESCRIPTION_SIZE 128 // Size for file descriptor descriptions

// File descriptor types
#define FD_TYPE_SOCKET "socket"
#define FD_TYPE_FILE "file"
#define FD_TYPE_PIPE "pipe"
#define FD_TYPE_EVENTFD "eventfd"
#define FD_TYPE_TIMERFD "timerfd"
#define FD_TYPE_SIGNALFD "signalfd"
#define FD_TYPE_OTHER "other"

#endif /* HYDROGEN_CONFIG_FD_H */