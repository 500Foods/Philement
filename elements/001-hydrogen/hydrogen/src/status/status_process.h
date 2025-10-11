/*
 * Process Metrics Collection
 * 
 * Defines functions for collecting process-level metrics including:
 * - File descriptor information
 * - Thread statistics
 * - Process memory usage
 * - Service-specific metrics
 */

#ifndef STATUS_PROCESS_H
#define STATUS_PROCESS_H

#include "status_core.h"
#include <sys/types.h>
#include <src/threads/threads.h>

// Helper functions
char* safe_truncate(char* dest, size_t dest_size, const char* src);

// File descriptor functions
bool collect_file_descriptors(FileDescriptorInfo **descriptors, int *count);
void get_fd_info(int fd, FileDescriptorInfo *info);
void get_socket_info(ino_t inode, char *proto, int *port);

// Memory metrics functions
bool get_process_memory(size_t *vmsize, size_t *vmrss, size_t *vmswap);

// Thread metrics functions
void convert_thread_metrics(const ServiceThreads *src, ServiceThreadMetrics *dest);

// Service metrics functions
bool collect_service_metrics(SystemMetrics *metrics, const WebSocketMetrics *ws_metrics);

#endif /* STATUS_PROCESS_H */
