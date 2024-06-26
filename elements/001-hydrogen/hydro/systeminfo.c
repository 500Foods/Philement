#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <pwd.h>
#include <utmp.h>
#include <time.h>
#include <jansson.h>
#include <glib.h>
#include <sys/sysinfo.h>
#include <utmp.h>
#include <sys/statvfs.h>
#include <sched.h>
#include <stdbool.h>
#include <mntent.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <glob.h>
#include <limits.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <NetworkManager.h>
#pragma GCC diagnostic pop

static int stats_counter = 0;

// Global variable to cache the NetworkManager client
static NMClient* nm_client = NULL;

// Previous Information
static json_t* previous_filesystems = NULL;
static json_t* previous_network = NULL;
static time_t last_filesystem_check_time = 0;
static time_t last_network_check_time = 0;

// Function prototypes
long get_memory_usage(const char* field);
long get_heap_size();
long get_stack_size();
void get_network_info(json_t* network_info);
void count_connections(const char* file_path, json_t* interfaces, const char* connection_type);
void get_filesystem_info(json_t* filesystems);
bool is_relevant_filesystem(const char* device, const char* mount_point);
void get_system_info(json_t* system_info);
void* systeminfo_thread();

long get_memory_usage(const char* field) {
    FILE* file = fopen("/proc/self/status", "r");
    if (file == NULL) {
        return -1;
    }

    char line[256];
    long value = -1;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, field, strlen(field)) == 0) {
            sscanf(line, "%*s %ld", &value);
            break;
        }
    }

    fclose(file);
    return value * 1024;  // Convert from kilobytes to bytes
}

long get_heap_size() {
    FILE* file = fopen("/proc/self/smaps", "r");
    if (file == NULL) {
        return -1;
    }

    char line[256];
    long heap_size = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "[heap]") != NULL) {
            sscanf(line, "%*x-%*x %ld", &heap_size);
            break;
        }
    }

    fclose(file);
    return heap_size;
}

long get_stack_size() {
    FILE* file = fopen("/proc/self/smaps", "r");
    if (file == NULL) {
        return -1;
    }

    char line[256];
    long stack_size = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "[stack]") != NULL) {
            sscanf(line, "%*x-%*x %ld", &stack_size);
            break;
        }
    }

    fclose(file);
    return stack_size;
}

void get_filesystem_info(json_t* filesystems) {
    time_t current_time = time(NULL);
    double elapsed = (last_filesystem_check_time == 0) ? 0.0 : difftime(current_time, last_filesystem_check_time);

    FILE* mtab = setmntent("/etc/mtab", "r");
    if (mtab == NULL) {
        perror("setmntent");
        return;
    }

    struct mntent* entry;
    while ((entry = getmntent(mtab)) != NULL) {
        struct stat st;
        if (stat(entry->mnt_fsname, &st) != 0) {
            continue;
        }

        if (strncmp(entry->mnt_fsname, "/dev/loop", 9) == 0) {
            continue;
        }

        if (!S_ISBLK(st.st_mode)) {
            continue;
        }

        struct statvfs vfs;
        if (statvfs(entry->mnt_dir, &vfs) != 0) {
            continue;
        }

        json_t* fs = json_object();

        unsigned long long total_space = (unsigned long long)vfs.f_frsize * vfs.f_blocks;
        unsigned long long available_space = (unsigned long long)vfs.f_frsize * vfs.f_bavail;
        unsigned long long used_space = total_space - available_space;

        double used_percent = (double)used_space / total_space * 100.0;
        double available_percent = (double)available_space / total_space * 100.0;

        json_object_set_new(fs, "device", json_string(entry->mnt_fsname));
        json_object_set_new(fs, "mount_point", json_string(entry->mnt_dir));
        json_object_set_new(fs, "total_space", json_integer(total_space));
        json_object_set_new(fs, "used_space", json_integer(used_space));
        json_object_set_new(fs, "available_space", json_integer(available_space));
        json_object_set_new(fs, "used_percent", json_real(used_percent));
        json_object_set_new(fs, "available_percent", json_real(available_percent));

        char* sysfs_path = NULL;

        if (strncmp(entry->mnt_fsname, "/dev/mapper/", 12) == 0) {
            // For device mapper devices
            char dm_name[256];
            snprintf(dm_name, sizeof(dm_name), "/dev/mapper/%s", entry->mnt_fsname + 12);

            char* lsblk_command = NULL;
            asprintf(&lsblk_command, "lsblk -n -o KNAME '%s'", dm_name);

            FILE* lsblk_pipe = popen(lsblk_command, "r");
            free(lsblk_command);

            if (lsblk_pipe) {
                char kname[256];
                if (fgets(kname, sizeof(kname), lsblk_pipe) != NULL) {
                    kname[strcspn(kname, "\n")] = 0;
                    asprintf(&sysfs_path, "/sys/block/%s/stat", kname);
                } else {
                    char error_msg[512];
                    snprintf(error_msg, sizeof(error_msg), "Failed to retrieve KNAME for device mapper device '%s' using lsblk", dm_name);
                    json_object_set_new(fs, "io_stat_error", json_string(error_msg));
                }
                pclose(lsblk_pipe);
            } else {
                char error_msg[512];
                snprintf(error_msg, sizeof(error_msg), "Failed to execute lsblk command for device mapper device '%s'", dm_name);
                json_object_set_new(fs, "io_stat_error", json_string(error_msg));
            }
        } else {
            // For other devices
            char* device_name = strdup(entry->mnt_fsname + strlen("/dev/"));
            char* partition = strchr(device_name, '/');
            if (partition != NULL) {
                *partition = '\0';
            }
            asprintf(&sysfs_path, "/sys/class/block/%s/stat", device_name);
            free(device_name);
        }

        if (sysfs_path) {
            FILE* stat_file = fopen(sysfs_path, "r");
            if (stat_file) {
                unsigned long long reads, read_sectors, writes, written_sectors;
                if (fscanf(stat_file, "%*d %*d %llu %*d %*d %*d %llu %*d %*d %llu %llu",
                           &reads, &read_sectors, &writes, &written_sectors) == 4) {
                    json_object_set_new(fs, "read_operations", json_integer(reads));
                    json_object_set_new(fs, "write_operations", json_integer(writes));
                    json_object_set_new(fs, "read_bytes", json_integer(read_sectors * 512));
                    json_object_set_new(fs, "written_bytes", json_integer(written_sectors * 512));

                    if (previous_filesystems && elapsed > 0) {
                        json_t* prev_fs = json_object_get(previous_filesystems, entry->mnt_fsname);
                        if (prev_fs) {
                            unsigned long long prev_reads = json_integer_value(json_object_get(prev_fs, "read_operations"));
                            unsigned long long prev_writes = json_integer_value(json_object_get(prev_fs, "write_operations"));
                            unsigned long long prev_read_bytes = json_integer_value(json_object_get(prev_fs, "read_bytes"));
                            unsigned long long prev_written_bytes = json_integer_value(json_object_get(prev_fs, "written_bytes"));

                            json_object_set_new(fs, "read_operations_per_second", json_real((reads - prev_reads) / elapsed));
                            json_object_set_new(fs, "write_operations_per_second", json_real((writes - prev_writes) / elapsed));
                            json_object_set_new(fs, "read_bytes_per_second", json_real((read_sectors * 512 - prev_read_bytes) / elapsed));
                            json_object_set_new(fs, "written_bytes_per_second", json_real((written_sectors * 512 - prev_written_bytes) / elapsed));
                        }
                    }
                }
                fclose(stat_file);
            } else {
                char error_msg[512];
                snprintf(error_msg, sizeof(error_msg), "Failed to open sysfs path '%s' for reading: %s", sysfs_path, strerror(errno));
                json_object_set_new(fs, "io_stat_error", json_string(error_msg));
            }
            free(sysfs_path);
        }

        json_object_set_new(filesystems, entry->mnt_fsname, fs);
    }

    endmntent(mtab);

    if (previous_filesystems) {
        json_decref(previous_filesystems);
    }
    previous_filesystems = json_deep_copy(filesystems);

    json_object_del(previous_filesystems, "_debug_previous_values");
    json_object_del(previous_filesystems, "_debug_elapsed");
    json_object_del(previous_filesystems, "_debug_last_filesystem_check_time");

    last_filesystem_check_time = current_time;

    json_object_set_new(filesystems, "_debug_elapsed", json_real(elapsed));
    json_object_set_new(filesystems, "_debug_previous_values", json_deep_copy(previous_filesystems));
    json_object_set_new(filesystems, "_debug_last_filesystem_check_time", json_integer(last_filesystem_check_time));
}

void get_network_info(json_t* network_info) {
    time_t current_time = time(NULL);
    double elapsed = (last_network_check_time == 0) ? 0.0 : difftime(current_time, last_network_check_time);

    struct ifaddrs* ifaddr, * ifa;
    json_t* interfaces = json_object();

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    if (!nm_client) {
        nm_client = nm_client_new(NULL, NULL);
        if (!nm_client) {
            fprintf(stderr, "Failed to create NetworkManager client\n");
            freeifaddrs(ifaddr);
            return;
        }
    }

    const GPtrArray* devices = nm_client_get_devices(nm_client);

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6)
            continue;

        if (strcmp(ifa->ifa_name, "lo") == 0 || !(ifa->ifa_flags & IFF_UP))
            continue;

        json_t* iface = json_object_get(interfaces, ifa->ifa_name);
        if (iface == NULL) {
            iface = json_object();
            json_object_set_new(iface, "name", json_string(ifa->ifa_name));
            json_object_set_new(iface, "addresses", json_array());
            json_object_set_new(iface, "tcp_connections", json_integer(0));
            json_object_set_new(iface, "udp_connections", json_integer(0));
            json_object_set_new(interfaces, ifa->ifa_name, iface);
        }

        char addr[INET6_ADDRSTRLEN];
        void* sin_addr = (family == AF_INET) ? (void*)&((struct sockaddr_in*)ifa->ifa_addr)->sin_addr
                                             : (void*)&((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
        inet_ntop(family, sin_addr, addr, sizeof(addr));
        json_array_append_new(json_object_get(iface, "addresses"), json_string(addr));

        for (guint i = 0; i < devices->len; i++) {
            NMDevice* device = g_ptr_array_index(devices, i);
            const char* device_iface = nm_device_get_iface(device);
            if (strcmp(device_iface, ifa->ifa_name) == 0) {
                const char* mac = nm_device_get_hw_address(device);
                if (mac) {
                    json_object_set_new(iface, "mac_address", json_string(mac));
                }

                if (NM_IS_DEVICE_WIFI(device)) {
                    NMDeviceWifi* wifi_device = NM_DEVICE_WIFI(device);
                    NMAccessPoint* active_ap = nm_device_wifi_get_active_access_point(wifi_device);
                    if (active_ap) {
                        GBytes* ssid_bytes = nm_access_point_get_ssid(active_ap);
                        if (ssid_bytes) {
                            gsize ssid_len;
                            const char* ssid = g_bytes_get_data(ssid_bytes, &ssid_len);
                            if (ssid_len > 0 && g_utf8_validate(ssid, ssid_len, NULL)) {
                                char* ssid_str = g_strndup(ssid, ssid_len);
                                json_object_set_new(iface, "ssid", json_string(ssid_str));
                                g_free(ssid_str);
                            }
                        }
                    }
                }
                break;
            }
        }

        char path[512];
        snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_bytes", ifa->ifa_name);
        FILE* f = fopen(path, "r");
        if (f) {
            unsigned long long rx_bytes;
            fscanf(f, "%llu", &rx_bytes);
            fclose(f);
            json_object_set_new(iface, "rx_bytes", json_integer(rx_bytes));

            if (previous_network && elapsed > 0) {
                json_t* prev_interfaces = json_object_get(previous_network, "interfaces");
                if (prev_interfaces && json_is_array(prev_interfaces)) {
                    size_t index;
                    json_t* prev_iface;
                    json_array_foreach(prev_interfaces, index, prev_iface) {
                        if (strcmp(json_string_value(json_object_get(prev_iface, "name")), ifa->ifa_name) == 0) {
                            unsigned long long prev_rx_bytes = json_integer_value(json_object_get(prev_iface, "rx_bytes"));
                            double rx_bytes_diff = (double)(rx_bytes - prev_rx_bytes);
                            double rx_bytes_per_second = (elapsed > 0) ? rx_bytes_diff / elapsed : 0.0;
                            json_object_set_new(iface, "rx_bytes_per_second", json_real(rx_bytes_per_second));
                            break;
                        }
                    }
                } 
            } else {
                json_object_set_new(iface, "rx_bytes_per_second", json_real(0));
            }
        }

        snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_bytes", ifa->ifa_name);
        f = fopen(path, "r");
        if (f) {
            unsigned long long tx_bytes;
            fscanf(f, "%llu", &tx_bytes);
            fclose(f);
            json_object_set_new(iface, "tx_bytes", json_integer(tx_bytes));

            if (previous_network && elapsed > 0) {
                json_t* prev_interfaces = json_object_get(previous_network, "interfaces");
                if (prev_interfaces && json_is_array(prev_interfaces)) {
                    size_t index;
                    json_t* prev_iface;
                    json_array_foreach(prev_interfaces, index, prev_iface) {
                        if (strcmp(json_string_value(json_object_get(prev_iface, "name")), ifa->ifa_name) == 0) {
                            unsigned long long prev_tx_bytes = json_integer_value(json_object_get(prev_iface, "tx_bytes"));
                            double tx_bytes_diff = (double)(tx_bytes - prev_tx_bytes);
                            double tx_bytes_per_second = (elapsed > 0) ? tx_bytes_diff / elapsed : 0.0;
                            json_object_set_new(iface, "tx_bytes_per_second", json_real(tx_bytes_per_second));
                            break;
                        }
                    }
                } 
            } else {
                json_object_set_new(iface, "tx_bytes_per_second", json_real(0));
            }
        }

        snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_packets", ifa->ifa_name);
        f = fopen(path, "r");
        if (f) {
            unsigned long long rx_packets;
            fscanf(f, "%llu", &rx_packets);
            fclose(f);
            json_object_set_new(iface, "rx_packets", json_integer(rx_packets));

            if (previous_network && elapsed > 0) {
                json_t* prev_interfaces = json_object_get(previous_network, "interfaces");
                if (prev_interfaces && json_is_array(prev_interfaces)) {
                    size_t index;
                    json_t* prev_iface;
                    json_array_foreach(prev_interfaces, index, prev_iface) {
                        if (strcmp(json_string_value(json_object_get(prev_iface, "name")), ifa->ifa_name) == 0) {
                            unsigned long long prev_rx_packets = json_integer_value(json_object_get(prev_iface, "rx_packets"));
                            double rx_packets_diff = (double)(rx_packets - prev_rx_packets);
                            double rx_packets_per_second = (elapsed > 0) ? rx_packets_diff / elapsed : 0.0;
                            json_object_set_new(iface, "rx_packets_per_second", json_real(rx_packets_per_second));
                            break;
                        }
                    }
                }
            } else {
                json_object_set_new(iface, "rx_packets_per_second", json_real(0));
            }
        }

        snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_packets", ifa->ifa_name);
        f = fopen(path, "r");
        if (f) {
            unsigned long long tx_packets;
            fscanf(f, "%llu", &tx_packets);
            fclose(f);
            json_object_set_new(iface, "tx_packets", json_integer(tx_packets));

            if (previous_network && elapsed > 0) {
                json_t* prev_interfaces = json_object_get(previous_network, "interfaces");
                if (prev_interfaces && json_is_array(prev_interfaces)) {
                    size_t index;
                    json_t* prev_iface;
                    json_array_foreach(prev_interfaces, index, prev_iface) {
                        if (strcmp(json_string_value(json_object_get(prev_iface, "name")), ifa->ifa_name) == 0) {
                            unsigned long long prev_tx_packets = json_integer_value(json_object_get(prev_iface, "tx_packets"));
                            double tx_packets_diff = (double)(tx_packets - prev_tx_packets);
                            double tx_packets_per_second = (elapsed > 0) ? tx_packets_diff / elapsed : 0.0;
                            json_object_set_new(iface, "tx_packets_per_second", json_real(tx_packets_per_second));
                            break;
                        }
                    }
                }
            } else {
                json_object_set_new(iface, "tx_packets_per_second", json_real(0));
            }
        }
    }

    freeifaddrs(ifaddr);

    count_connections("/proc/net/tcp", interfaces, "tcp_connections");
    count_connections("/proc/net/udp", interfaces, "udp_connections");

    json_t* filtered_interfaces = json_array();
    const char* iface_name;
    json_t* iface_value;
    json_object_foreach(interfaces, iface_name, iface_value) {
        if (json_integer_value(json_object_get(iface_value, "tcp_connections")) > 0 ||
            json_integer_value(json_object_get(iface_value, "udp_connections")) > 0) {
            json_array_append(filtered_interfaces, iface_value);
        }
    }

    json_object_set_new(network_info, "interfaces", filtered_interfaces);
    json_decref(interfaces);

    // Ensure previous_network is copied correctly
    if (previous_network) {
        json_decref(previous_network);
    }
    previous_network = json_deep_copy(network_info);

    json_object_del(previous_network, "_debug_previous_values");
    json_object_del(previous_network, "_debug_elapsed");
    json_object_del(previous_network, "_debug_last_network_check_time");

    last_network_check_time = current_time;

    json_object_set_new(network_info, "_debug_elapsed", json_real(elapsed));
    json_object_set_new(network_info, "_debug_previous_values", json_deep_copy(previous_network));
    json_object_set_new(network_info, "_debug_last_network_check_time", json_integer(last_network_check_time));
}

void count_connections(const char* file_path, json_t* interfaces, const char* connection_type) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    char line[256];
    int unidentified_connections = 0;

    // Skip header line
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        char local_addr[128];
        unsigned int local_port;
        sscanf(line, "%*d: %64[0-9A-Fa-f]:%X", local_addr, &local_port);

        struct in_addr addr;
        addr.s_addr = strtoul(local_addr, NULL, 16);

        char local_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, local_ip, sizeof(local_ip));

        int matched = 0;
        const char* iface_name;
        json_t* iface_value;
        json_object_foreach(interfaces, iface_name, iface_value) {
            size_t index;
            json_t* addr_value;
            json_array_foreach(json_object_get(iface_value, "addresses"), index, addr_value) {
                const char* iface_addr = json_string_value(addr_value);
                if (strcmp(iface_addr, local_ip) == 0) {
                    json_integer_set(json_object_get(iface_value, connection_type),
                                     json_integer_value(json_object_get(iface_value, connection_type)) + 1);
                    matched = 1;
                    break;
                }
            }
            if (matched)
                break;
        }

        if (!matched)
            unidentified_connections++;
    }

    fclose(file);

    if (unidentified_connections > 0) {
        json_t* unidentified = json_object_get(interfaces, "Unidentified Adapter");
        if (!unidentified) {
            unidentified = json_object();
            json_object_set_new(unidentified, "name", json_string("Unidentified Adapter"));
            json_object_set_new(unidentified, "tcp_connections", json_integer(0));
            json_object_set_new(unidentified, "udp_connections", json_integer(0));
            json_object_set_new(interfaces, "Unidentified Adapter", unidentified);
        }
        json_integer_set(json_object_get(unidentified, connection_type), unidentified_connections);
    }
}


bool is_relevant_filesystem(const char* device, const char* mount_point) {
    // Exclude loop devices
    if (strncmp(device, "/dev/loop", 9) == 0) {
        return false;
    }

    // Include all other devices starting with "/dev/"
    if (strncmp(device, "/dev/", 5) == 0) {
        return true;
    }

    // Include specific mount points that might be relevant
    const char* relevant_mounts[] = {"/", "/home", "/boot", "/var", "/usr", "/tmp", NULL};
    for (int i = 0; relevant_mounts[i] != NULL; i++) {
        if (strcmp(mount_point, relevant_mounts[i]) == 0) {
            return true;
        }
    }

    // Exclude specific filesystem types
    const char* exclude_fs[] = {"sysfs", "proc", "devtmpfs", "securityfs", "tmpfs", "devpts", "cgroup2", 
                                "pstore", "binfmt_misc", "debugfs", "fusectl", "configfs", "fuse", 
                                "gvfsd-fuse", "efivarfs", NULL};
    for (int i = 0; exclude_fs[i] != NULL; i++) {
        if (strcmp(device, exclude_fs[i]) == 0) {
            return false;
        }
    }

    return false;  // Exclude anything else
}

void get_system_info(json_t* system_info) {

    // Add the stats generation count
    json_object_set_new(system_info, "stats_counter", json_integer(stats_counter));
    stats_counter++;

    // Load averages
    double loadavg[3];
    if (getloadavg(loadavg, 3) != -1) {
        json_object_set_new(system_info, "load_1min", json_real(loadavg[0]));
        json_object_set_new(system_info, "load_5min", json_real(loadavg[1]));
        json_object_set_new(system_info, "load_15min", json_real(loadavg[2]));
    }

    // CPU cores and usage
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    json_object_set_new(system_info, "cpu_cores", json_integer(num_cores));

    FILE* stat = fopen("/proc/stat", "r");
    if (stat) {
        char line[256];
        json_t* cpu_usage = json_object();
        json_t* cpu_usage_per_core = json_object();

        while (fgets(line, sizeof(line), stat)) {
            if (strncmp(line, "cpu", 3) == 0) {
                char cpu[10];
                long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
                sscanf(line, "%s %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
                       cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);
                
                long long total = user + nice + system + idle + iowait + irq + softirq + steal;
                double usage = 100.0 * (total - idle) / total;

                if (strcmp(cpu, "cpu") == 0) {
                    json_object_set_new(cpu_usage, "total", json_real(usage));
                } else {
                    json_object_set_new(cpu_usage_per_core, cpu, json_real(usage));
                }
            }
        }
        fclose(stat);

        json_object_set_new(system_info, "cpu_usage", cpu_usage);
        json_object_set_new(system_info, "cpu_usage_per_core", cpu_usage_per_core);
    }

    // Temperatures
    DIR* dir = opendir("/sys/class/thermal");
    if (dir) {
        struct dirent* entry;
        json_t* temperatures = json_object();
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "thermal_zone", 12) == 0) {
                char path[512];
                snprintf(path, sizeof(path), "/sys/class/thermal/%s/temp", entry->d_name);
                FILE* temp_file = fopen(path, "r");
                if (temp_file) {
                    int temp;
                    fscanf(temp_file, "%d", &temp);
                    fclose(temp_file);
                    json_object_set_new(temperatures, entry->d_name, json_real(temp / 1000.0));
                }
            }
        }
        closedir(dir);
        json_object_set_new(system_info, "temperatures", temperatures);
    }

    // System uptime and boot time
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        json_object_set_new(system_info, "uptime_seconds", json_integer(si.uptime));
        time_t boot_time = time(NULL) - si.uptime;
        json_object_set_new(system_info, "boot_time", json_integer(boot_time));

        // Physical RAM
        unsigned long long total_ram = si.totalram * si.mem_unit;
        json_object_set_new(system_info, "total_physical_ram", json_integer(total_ram));

        // Swap information
        unsigned long long total_swap = si.totalswap * si.mem_unit;
        unsigned long long free_swap = si.freeswap * si.mem_unit;
        unsigned long long used_swap = total_swap - free_swap;
        double swap_used_percent = (double)used_swap / total_swap * 100.0;
        double swap_free_percent = 100.0 - swap_used_percent;

        json_object_set_new(system_info, "total_swap", json_integer(total_swap));
        json_object_set_new(system_info, "used_swap", json_integer(used_swap));
        json_object_set_new(system_info, "free_swap", json_integer(free_swap));
        json_object_set_new(system_info, "swap_used_percent", json_real(swap_used_percent));
        json_object_set_new(system_info, "swap_free_percent", json_real(swap_free_percent));
    }

    // OS and kernel info
    struct utsname un;
    if (uname(&un) == 0) {
        json_object_set_new(system_info, "os_name", json_string(un.sysname));
        json_object_set_new(system_info, "os_version", json_string(un.version));
        json_object_set_new(system_info, "kernel_version", json_string(un.release));
    }

    // Get a nicer OS name
    FILE* os_release = fopen("/etc/os-release", "r");
    if (os_release) {
        char line[256];
        char os_name[256] = "";
        char os_version[256] = "";
        while (fgets(line, sizeof(line), os_release)) {
            if (strncmp(line, "NAME=", 5) == 0) {
		sscanf(line, "NAME=\"%[^\"]\"", os_name);
            } else if (strncmp(line, "VERSION=", 8) == 0) {
	        sscanf(line, "VERSION=\"%[^\"]\"", os_version);
            }
        }
        fclose(os_release);
        //if (os_name[0] && os_version[0]) {
            char full_os_name[512];
            snprintf(full_os_name, sizeof(full_os_name), "%s %s", os_name, os_version);
            json_object_set_new(system_info, "os_full_name", json_string(full_os_name));
        //}
    }

    // Open file descriptors - system
    FILE* file = fopen("/proc/sys/fs/file-nr", "r");
    if (file) {
        unsigned long allocated, unused, max;
        fscanf(file, "%lu %lu %lu", &allocated, &unused, &max);
        fclose(file);
        json_object_set_new(system_info, "open_file_descriptors_system", json_integer(allocated - unused));
        json_object_set_new(system_info, "max_file_descriptors_system", json_integer(max));
    }

    // Open file descriptors - process
    dir = opendir("/proc/self/fd");
    if (dir) {
        int count = 0;
        json_t* open_files = json_array();
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.') {
                count++;
                char fd_path[512];
                char file_path[512];
                ssize_t len;
                snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%s", entry->d_name);
                len = readlink(fd_path, file_path, sizeof(file_path) - 1);
                if (len != -1) {
                    file_path[len] = '\0';
                    json_array_append_new(open_files, json_string(file_path));
                }
            }
        }
        closedir(dir);
        json_object_set_new(system_info, "open_file_descriptors_process", json_integer(count));
        json_object_set_new(system_info, "open_files", open_files);
    }

    // Logged in users
    json_t* users = json_array();
    setutent();
    struct utmp* entry;
    while ((entry = getutent()) != NULL) {
        if (entry->ut_type == USER_PROCESS) {
            json_t* user = json_object();
            json_object_set_new(user, "username", json_string(entry->ut_user));
            json_object_set_new(user, "tty", json_string(entry->ut_line));
            json_object_set_new(user, "host", json_string(entry->ut_host));
            json_object_set_new(user, "login_time", json_integer(entry->ut_tv.tv_sec));
            json_array_append_new(users, user);
        }
    }
    endutent();
    json_object_set_new(system_info, "logged_in_users", users);

    // Total processes
    int process_count = 0;
    DIR* proc_dir = opendir("/proc");
    if (proc_dir) {
        struct dirent* proc_entry;
        while ((proc_entry = readdir(proc_dir)) != NULL) {
            if (proc_entry->d_type == DT_DIR) {
                char* endptr;
                strtol(proc_entry->d_name, &endptr, 10);
                if (*endptr == '\0') {
                    process_count++;
                }
            }
        }
        closedir(proc_dir);
    }
    json_object_set_new(system_info, "total_processes", json_integer(process_count));

    // Thread count for current process
    int thread_count = 0;
    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/task", getpid());
    
    DIR* task_dir = opendir(proc_path);
    if (task_dir) {
        struct dirent* task_entry;
        while ((task_entry = readdir(task_dir)) != NULL) {
            if (task_entry->d_type == DT_DIR && strcmp(task_entry->d_name, ".") != 0 && strcmp(task_entry->d_name, "..") != 0) {
                thread_count++;
            }
        }
        closedir(task_dir);
    }
    json_object_set_new(system_info, "current_process_threads", json_integer(thread_count));
}

void* systeminfo_thread() {

    // Initializations...
    stats_counter = 1;
    last_filesystem_check_time = 0;
    last_network_check_time = 0;

    while (1) {
        struct timespec start, end, memory_start, memory_end, network_start, network_end, filesystem_start, filesystem_end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        json_t* root = json_object();
        json_t* memory = json_object();
        json_t* time_obj = json_object();
        json_t* network_info = json_object();
        json_t* filesystems = json_object();
        json_t* system_info = json_object();

        // Memory statistics
        clock_gettime(CLOCK_MONOTONIC, &memory_start);

        long hwm = get_memory_usage("VmHWM:");
        if (hwm != -1) {
            json_t* hwm_obj = json_object();
            json_object_set_new(hwm_obj, "value", json_integer(hwm));
            json_object_set_new(hwm_obj, "units", json_string("bytes"));
            json_object_set_new(memory, "high_watermark", hwm_obj);
        }

        long current_usage = get_memory_usage("VmRSS:");
        if (current_usage != -1) {
            json_t* current = json_object();
            json_object_set_new(current, "value", json_integer(current_usage));
            json_object_set_new(current, "units", json_string("bytes"));
            json_object_set_new(memory, "current", current);
        }

        long heap_size = get_heap_size();
        if (heap_size != -1) {
            json_t* heap = json_object();
            json_object_set_new(heap, "value", json_integer(heap_size));
            json_object_set_new(heap, "units", json_string("bytes"));
            json_object_set_new(memory, "heap", heap);
        }

        long stack_size = get_stack_size();
        if (stack_size != -1) {
            json_t* stack = json_object();
            json_object_set_new(stack, "value", json_integer(stack_size));
            json_object_set_new(stack, "units", json_string("bytes"));
            json_object_set_new(memory, "stack", stack);
        }

        clock_gettime(CLOCK_MONOTONIC, &memory_end);

        json_object_set_new(root, "memory", memory);

        // Retrieve current time
        time_t now = time(NULL);
        json_object_set_new(time_obj, "timestamp", json_integer(now));

        char local_time[64];
        struct tm* local_tm = localtime(&now);
        strftime(local_time, sizeof(local_time), "%Y-%m-%dT%H:%M:%S%z", local_tm);
        json_object_set_new(time_obj, "local", json_string(local_time));

        char utc_time[64];
        struct tm* utc_tm = gmtime(&now);
        strftime(utc_time, sizeof(utc_time), "%Y-%m-%dT%H:%M:%SZ", utc_tm);
        json_object_set_new(time_obj, "UTC", json_string(utc_time));

        json_object_set_new(root, "time", time_obj);

        // Network statistics
        clock_gettime(CLOCK_MONOTONIC, &network_start);

        get_network_info(network_info);
        json_object_set_new(root, "network", network_info);

        clock_gettime(CLOCK_MONOTONIC, &network_end);

        // Filesystem statistics
        clock_gettime(CLOCK_MONOTONIC, &filesystem_start);
        get_filesystem_info(filesystems);
        json_object_set_new(root, "filesystems", filesystems);
        clock_gettime(CLOCK_MONOTONIC, &filesystem_end);

        // System information
        get_system_info(system_info);
        json_object_set_new(root, "system", system_info);

        // Calculate elapsed times
        clock_gettime(CLOCK_MONOTONIC, &end);

        double total_elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
        double memory_elapsed = (memory_end.tv_sec - memory_start.tv_sec) * 1000.0 + (memory_end.tv_nsec - memory_start.tv_nsec) / 1000000.0;
        double network_elapsed = (network_end.tv_sec - network_start.tv_sec) * 1000.0 + (network_end.tv_nsec - network_start.tv_nsec) / 1000000.0;
        double filesystem_elapsed = (filesystem_end.tv_sec - filesystem_start.tv_sec) * 1000.0 + (filesystem_end.tv_nsec - filesystem_start.tv_nsec) / 1000000.0;

        json_object_set_new(time_obj, "Elapsed Total (ms)", json_real(total_elapsed));
        json_object_set_new(time_obj, "Elapsed Memory (ms)", json_real(memory_elapsed));
        json_object_set_new(time_obj, "Elapsed Network (ms)", json_real(network_elapsed));
        json_object_set_new(time_obj, "Elapsed Filesystem (ms)", json_real(filesystem_elapsed));

        // Print the JSON object
        char* json_str = json_dumps(root, JSON_INDENT(4));
        puts(json_str);
        fflush(stdout);
        free(json_str);

        json_decref(root);

        // Sleep for 60 seconds
        sleep(10);
    }

    return NULL;
}

//void cleanup_systeminfo() {
    //if (previous_filesystems) {
        //json_decref(previous_filesystems);
        //previous_filesystems = NULL;
    //}
    //if (previous_network) {
        //json_decref(previous_network);
        //previous_network = NULL;
    //}
    //if (nm_client) {
        //g_object_unref(nm_client);
        //nm_client = NULL;
    //}
//}
