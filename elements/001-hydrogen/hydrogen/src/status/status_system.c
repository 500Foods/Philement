/*
 * System Metrics Collection Implementation
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "status_system.h"
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

// Helper function to format percentage with consistent precision (3 decimal places)
void format_percentage(double value, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "%.3f", value);
}

// Collect CPU metrics from /proc/stat
bool collect_cpu_metrics(CpuMetrics *cpu) {
    FILE *stat = fopen("/proc/stat", "r");
    if (!stat) {
        log_this(SR_STATUS, "Failed to open /proc/stat", LOG_LEVEL_ERROR, 0);
        return false;
    }

    char line[256];  // Fixed size for /proc/stat entries
    int core_count = 0;

    // First pass to count cores
    while (fgets(line, sizeof(line), stat)) {
        if (strncmp(line, "cpu", 3) == 0) {
            if (line[3] >= '0' && line[3] <= '9') {
                core_count++;
            }
        }
    }

    if (core_count == 0) {
        log_this(SR_STATUS, "No CPU cores found", LOG_LEVEL_ERROR, 0);
        fclose(stat);
        return false;
    }

    // Allocate array for per-core usage
    char **core_usage = calloc((size_t)core_count, sizeof(char*));
    if (!core_usage) {
        log_this(SR_STATUS, "Failed to allocate core usage array", LOG_LEVEL_ERROR, 0);
        return false;
    }

    for (int i = 0; i < core_count; i++) {
        core_usage[i] = calloc(MAX_PERCENTAGE_STRING, sizeof(char));
        if (!core_usage[i]) {
            log_this(SR_STATUS, "Failed to allocate core usage string", LOG_LEVEL_ERROR, 0);
            // Clean up previously allocated strings
            for (int j = 0; j < i; j++) {
                free(core_usage[j]);
            }
            free(core_usage);
            return false;
        }
    }

    // Reset file position
    rewind(stat);

    // Second pass to collect metrics
    while (fgets(line, sizeof(line), stat)) {
        if (strncmp(line, "cpu", 3) == 0) {
            char cpu_id[32];  // CPU identifier buffer
            long long user, nice, system, idle, iowait, irq, softirq, steal;
            if (sscanf(line, "%31s %lld %lld %lld %lld %lld %lld %lld %lld",
                      cpu_id, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) == 9) {
                
                long long total = user + nice + system + idle + iowait + irq + softirq + steal;
                if (total > 0) {  // Avoid division by zero
                    double usage = 100.0 * (double)(total - idle) / (double)total;

                    if (strcmp(cpu_id, "cpu") == 0) {
                        format_percentage(usage, cpu->total_usage, MAX_PERCENTAGE_STRING);
                    } else {
                        int core = atoi(cpu_id + 3);
                        if (core >= 0 && core < core_count) {
                            format_percentage(usage, core_usage[core], MAX_PERCENTAGE_STRING);
                        }
                    }
                }
            }
        }
    }

    fclose(stat);

    // Store results
    cpu->per_core_usage = core_usage;
    cpu->core_count = core_count;

    // Get load averages
    double loadavg[3];
    if (getloadavg(loadavg, 3) != -1) {
        format_percentage(loadavg[0], cpu->load_1min, MAX_PERCENTAGE_STRING);
        format_percentage(loadavg[1], cpu->load_5min, MAX_PERCENTAGE_STRING);
        format_percentage(loadavg[2], cpu->load_15min, MAX_PERCENTAGE_STRING);
    }

    return true;
}

// Collect memory metrics using sysinfo
bool collect_memory_metrics(SystemMemoryMetrics *memory) {
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        log_this(SR_STATUS, "Failed to get system memory info", LOG_LEVEL_ERROR, 0);
        return false;
    }

    memory->total_ram = si.totalram * si.mem_unit;
    memory->free_ram = si.freeram * si.mem_unit;
    memory->used_ram = memory->total_ram - memory->free_ram;

    double ram_percent = (double)memory->used_ram / (double)memory->total_ram * 100.0;
    format_percentage(ram_percent, memory->ram_used_percent, MAX_PERCENTAGE_STRING);

    memory->total_swap = si.totalswap * si.mem_unit;
    if (memory->total_swap > 0) {
        memory->free_swap = si.freeswap * si.mem_unit;
        memory->used_swap = memory->total_swap - memory->free_swap;
        double swap_percent = (double)memory->used_swap / (double)memory->total_swap * 100.0;
        format_percentage(swap_percent, memory->swap_used_percent, MAX_PERCENTAGE_STRING);
    } else {
        memory->free_swap = 0;
        memory->used_swap = 0;
        memory->swap_used_percent[0] = '0';
        memory->swap_used_percent[1] = '\0';
    }

    return true;
}

// Collect network interface metrics
bool collect_network_metrics(NetworkMetrics *network) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != 0) {
        log_this(SR_STATUS, "Failed to get network interfaces", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // First pass to count interfaces
    int interface_count = 0;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || strcmp(ifa->ifa_name, "lo") == 0)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6) {
            interface_count++;
        }
    }

    // Allocate array for interfaces
    network->interfaces = calloc((size_t)interface_count, sizeof(NetworkInterfaceMetrics));
    network->interface_count = interface_count;

    // Second pass to collect metrics
    int current_interface = 0;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || strcmp(ifa->ifa_name, "lo") == 0)
            continue;

        int family = ifa->ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6)
            continue;

        NetworkInterfaceMetrics *iface = &network->interfaces[current_interface];
        strncpy(iface->name, ifa->ifa_name, MAX_TYPE_STRING - 1);
        iface->name[MAX_TYPE_STRING - 1] = '\0';

        // Get interface statistics
        char path[256];
        snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_bytes", ifa->ifa_name);
        FILE *f = fopen(path, "r");
        if (f) {
            if (fscanf(f, "%llu", &iface->rx_bytes) != 1) {
                log_this(SR_STATUS, "Failed to read rx_bytes", LOG_LEVEL_ERROR, 0);
                iface->rx_bytes = 0; // Default value
            }
            fclose(f);
        }

        snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_bytes", ifa->ifa_name);
        f = fopen(path, "r");
        if (f) {
            if (fscanf(f, "%llu", &iface->tx_bytes) != 1) {
                log_this(SR_STATUS, "Failed to read tx_bytes", LOG_LEVEL_ERROR, 0);
                iface->tx_bytes = 0; // Default value
            }
            fclose(f);
        }

        current_interface++;
    }

    freeifaddrs(ifaddr);
    return true;
}

// Collect filesystem metrics
bool collect_filesystem_metrics(FilesystemMetrics **filesystems, int *count) {
    FILE *mtab = setmntent("/etc/mtab", "r");
    if (!mtab) {
        log_this(SR_STATUS, "Failed to open /etc/mtab", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // First pass to count filesystems
    struct mntent *entry;
    *count = 0;
    while ((entry = getmntent(mtab))) {
        if (strcmp(entry->mnt_type, "tmpfs") == 0 ||
            strcmp(entry->mnt_type, "devtmpfs") == 0 ||
            strcmp(entry->mnt_type, "sysfs") == 0 ||
            strcmp(entry->mnt_type, "proc") == 0)
            continue;
        (*count)++;
    }

    // Allocate array for filesystem metrics
    *filesystems = calloc((size_t)*count, sizeof(FilesystemMetrics));
    if (!*filesystems) {
        endmntent(mtab);
        return false;
    }

    // Reset and second pass to collect metrics
    rewind(mtab);
    int current_fs = 0;

    while ((entry = getmntent(mtab))) {
        if (strcmp(entry->mnt_type, "tmpfs") == 0 ||
            strcmp(entry->mnt_type, "devtmpfs") == 0 ||
            strcmp(entry->mnt_type, "sysfs") == 0 ||
            strcmp(entry->mnt_type, "proc") == 0)
            continue;

        struct statvfs vfs;
        if (statvfs(entry->mnt_dir, &vfs) != 0)
            continue;

        FilesystemMetrics *fs = &(*filesystems)[current_fs];
        strncpy(fs->device, entry->mnt_fsname, MAX_PATH_STRING - 1);
        strncpy(fs->mount_point, entry->mnt_dir, MAX_PATH_STRING - 1);
        strncpy(fs->type, entry->mnt_type, MAX_TYPE_STRING - 1);

        fs->total_space = (unsigned long long)vfs.f_frsize * vfs.f_blocks;
        fs->available_space = (unsigned long long)vfs.f_frsize * vfs.f_bavail;
        unsigned long long free_space = (unsigned long long)vfs.f_frsize * vfs.f_bfree;
        fs->used_space = fs->total_space - free_space;

        double used_percent = (double)fs->used_space / (double)fs->total_space * 100.0;
        format_percentage(used_percent, fs->used_percent, MAX_PERCENTAGE_STRING);

        current_fs++;
    }

    endmntent(mtab);
    return true;
}

// Collect system information using uname
bool collect_system_info(SystemMetrics *metrics) {
    struct utsname system_info;
    if (uname(&system_info) != 0) {
        log_this(SR_STATUS, "Failed to get system information", LOG_LEVEL_ERROR, 0);
        return false;
    }

    strncpy(metrics->sysname, system_info.sysname, MAX_SYSINFO_STRING - 1);
    strncpy(metrics->nodename, system_info.nodename, MAX_SYSINFO_STRING - 1);
    strncpy(metrics->release_info, system_info.release, MAX_SYSINFO_STRING - 1);
    strncpy(metrics->version_info, system_info.version, MAX_SYSINFO_STRING - 1);
    strncpy(metrics->machine, system_info.machine, MAX_SYSINFO_STRING - 1);

    return true;
}
