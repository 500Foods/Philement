#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <gio/gio.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <libnm/NetworkManager.h>
#pragma GCC diagnostic pop
#include <nm-dbus-interface.h>
#include <time.h>

#define MAX_DEVICES 100
#define MAX_SSIDS 100
#define WIFI_SCAN_TIMEOUT 30.0 // Wi-Fi scan timeout in seconds

// Structures to hold network information
typedef struct {
    int is_active;
    char interface[32];
    char connection_name[64];
    char ip_address[16];
    char device_path[256];
    guint32 device_type;
} NetworkConnection;

typedef struct {
    char ssid[33];
    int strength;
    unsigned int frequency;
} AccessPoint;

static GDBusConnection *dbus_connection = NULL;

static NetworkConnection network_connections[MAX_DEVICES];
static int num_connections = 0;

// Function prototypes
gboolean init_dbus_connection(void);
void* network_list_thread(void* arg);
void* wifi_scan_thread(void* arg);
void get_active_connections(void);
void scan_wifi_networks(const char *device_path);
void wait_for_wifi_scan(const char *device_path);
void print_wifi_device_info(const char *device_path);
int compare_access_points(const void *a, const void *b);

int compare_access_points(const void *a, const void *b) {
    return ((AccessPoint *)b)->strength - ((AccessPoint *)a)->strength;
}

void retrieve_access_points(const char *device_path) {
    GError *error = NULL;
    GDBusProxy *device_proxy = g_dbus_proxy_new_sync(
        dbus_connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        NM_DBUS_SERVICE,
        device_path,
        NM_DBUS_INTERFACE_DEVICE_WIRELESS,
        NULL,
        &error
    );

    if (error != NULL) {
        fprintf(stderr, "Failed to create device proxy: %s\n", error->message);
        g_error_free(error);
        return;
    }

    GVariant *aps = g_dbus_proxy_call_sync(
        device_proxy,
        "GetAccessPoints",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (aps != NULL) {
        GVariantIter *iter;
        const char *ap_path;
        g_variant_get(aps, "(ao)", &iter);

        AccessPoint *access_points = NULL;
        int num_aps = 0;
        int max_aps = 10;
        access_points = malloc(max_aps * sizeof(AccessPoint));

        while (g_variant_iter_next(iter, "&o", &ap_path)) {
            GVariant *ap_props = g_dbus_connection_call_sync(
                dbus_connection,
                NM_DBUS_SERVICE,
                ap_path,
                "org.freedesktop.DBus.Properties",
                "GetAll",
                g_variant_new("(s)", NM_DBUS_INTERFACE_ACCESS_POINT),
                G_VARIANT_TYPE("(a{sv})"),
                G_DBUS_CALL_FLAGS_NONE,
                -1,
                NULL,
                &error
            );

            if (ap_props != NULL) {
                GVariantIter *props_iter;
                const char *key;
                GVariant *value;

                g_variant_get(ap_props, "(a{sv})", &props_iter);

                AccessPoint ap = {0};

                while (g_variant_iter_next(props_iter, "{&sv}", &key, &value)) {
                    if (strcmp(key, "Ssid") == 0) {
                        const guchar *ssid_data;
                        gsize ssid_len;
                        ssid_data = g_variant_get_fixed_array(value, &ssid_len, sizeof(guchar));
                        memcpy(ap.ssid, ssid_data, ssid_len < sizeof(ap.ssid) - 1 ? ssid_len : sizeof(ap.ssid) - 1);
                    } else if (strcmp(key, "Strength") == 0) {
                        ap.strength = g_variant_get_byte(value);
                    } else if (strcmp(key, "Frequency") == 0) {
                        ap.frequency = g_variant_get_uint32(value);
                    }
                    g_variant_unref(value);
                }
                g_variant_iter_free(props_iter);

                if (num_aps >= max_aps) {
                    max_aps *= 2;
                    access_points = realloc(access_points, max_aps * sizeof(AccessPoint));
                }
                access_points[num_aps++] = ap;

                g_variant_unref(ap_props);
            }
        }
        g_variant_iter_free(iter);
        g_variant_unref(aps);

        qsort(access_points, num_aps, sizeof(AccessPoint), compare_access_points);

        char seen_ssids[MAX_SSIDS][33] = {0};
        int num_seen_ssids = 0;
        int num_unique_aps = 0;

        for (int i = 0; i < num_aps; i++) {
            //const char *band = access_points[i].frequency > 4000 ? "5G" : "2.4G";

            int seen = 0;
            for (int j = 0; j < num_seen_ssids; j++) {
                if (strcmp(seen_ssids[j], access_points[i].ssid) == 0) {
                    seen = 1;
                    break;
                }
            }

            if (!seen) {
                strncpy(seen_ssids[num_seen_ssids], access_points[i].ssid, 32);
                seen_ssids[num_seen_ssids][32] = '\0';
                num_seen_ssids++;
                num_unique_aps++;
            }
        }

        printf("\n%d Access Points:\n", num_unique_aps);
        for (int i = 0; i < num_seen_ssids; i++) {
            for (int j = 0; j < num_aps; j++) {
                if (strcmp(seen_ssids[i], access_points[j].ssid) == 0) {
                    const char *band = access_points[j].frequency > 4000 ? "5G" : "2.4G";
                    printf(" [%4d%% | %s ] %s\n",
                           access_points[j].strength,
                           band,
                           access_points[j].ssid);
                    break;
                }
            }
        }

        free(access_points);
    } else {
        fprintf(stderr, "Failed to retrieve access points: %s\n", error->message);
        g_error_free(error);
    }

    g_object_unref(device_proxy);
}

void print_wireless_capabilities(guint32 capabilities) {
    const char *cap_names[] = {
        "WEP40", "WEP104", "TKIP", "CCMP",
        "WPA", "RSN", "AP", "AD-HOC",
        "FREQ_VALID", "FREQ_2GHZ", "FREQ_5GHZ"
    };
    int num_caps = sizeof(cap_names) / sizeof(cap_names[0]);

    printf("  Wireless Capabilities:");
    for (int i = 0; i < num_caps; i++) {
        if (capabilities & (1 << i)) {
            printf(" %s", cap_names[i]);
        }
    }
    printf("\n");
}

void print_wifi_device_info(const char *device_path) {
    GError *error = NULL;
    GVariant *props = g_dbus_connection_call_sync(
        dbus_connection,
        NM_DBUS_SERVICE,
        device_path,
        "org.freedesktop.DBus.Properties",
        "GetAll",
        g_variant_new("(s)", NM_DBUS_INTERFACE_DEVICE_WIRELESS),
        G_VARIANT_TYPE("(a{sv})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (props != NULL) {
	          GVariantIter *iter;
        const char *key;
        GVariant *value;

        g_variant_get(props, "(a{sv})", &iter);
        printf("\nWi-Fi Device Information:\n");
        guint32 capabilities = 0;
        // gint64 last_scan = 0;

        while (g_variant_iter_next(iter, "{&sv}", &key, &value)) {
            if (strcmp(key, "AccessPoints") != 0) {
                if (strcmp(key, "WirelessCapabilities") == 0) {
                    capabilities = g_variant_get_uint32(value);
                } else if (strcmp(key, "LastScan") == 0) {
         //           last_scan = g_variant_get_int64(value);
                } else if (strcmp(key, "ActiveAccessPoint") == 0) {
                    const char *active_ap = g_variant_get_string(value, NULL);
                    printf("  Active Access Point: %s\n", active_ap);
                } else if (strcmp(key, "Bitrate") == 0) {
                    guint32 bitrate = g_variant_get_uint32(value);
                    printf("  Bitrate: %u Kbit/s\n", bitrate);
                } else {
                    char *value_str = g_variant_print(value, FALSE);
                    printf("  %s: %s\n", key, value_str);
                    g_free(value_str);
                }
            }
            g_variant_unref(value);
        }

        print_wireless_capabilities(capabilities);
        //printf("  Last Scan: %.1f seconds ago\n", last_scan / 1000000.0);

        g_variant_iter_free(iter);
        g_variant_unref(props);
    } else {
        fprintf(stderr, "Failed to retrieve Wi-Fi device information: %s\n", error->message);
        g_error_free(error);
    }
}

/**
 * @brief Initializes the D-Bus connection to the system bus.
 * @return TRUE if the connection was successfully established, FALSE otherwise.
 */
gboolean init_dbus_connection(void) {
    GError *error = NULL;

    dbus_connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error != NULL) {
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Thread function to list active network connections.
 * @param arg Unused argument.
 * @return NULL.
 */
void* network_list_thread(void* arg) {
    (void)arg; // Suppress unused parameter warning
    get_active_connections();
    return NULL;
}

/**
 * @brief Thread function to scan for available Wi-Fi networks.
 * @param arg Device path of the Wi-Fi device.
 * @return NULL.
 */
void* wifi_scan_thread(void* arg) {
    char *device_path = (char *)arg;
    scan_wifi_networks(device_path);
    return NULL;
}

/**
 * @brief Retrieves the list of active network connections.
 */

void get_active_connections(void) {
    GVariant *ret;
    GError *error = NULL;

    ret = g_dbus_connection_call_sync(
        dbus_connection,
        NM_DBUS_SERVICE,
        NM_DBUS_PATH,
        NM_DBUS_INTERFACE,
        "GetDevices",
        NULL,
        G_VARIANT_TYPE("(ao)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error != NULL) {
        g_error_free(error);
        return;
    }

    GVariantIter *iter;
    const gchar *device_path;
    g_variant_get(ret, "(ao)", &iter);

    while (g_variant_iter_next(iter, "&o", &device_path)) {
        GVariant *device_props = g_dbus_connection_call_sync(
            dbus_connection,
            NM_DBUS_SERVICE,
            device_path,
            "org.freedesktop.DBus.Properties",
            "GetAll",
            g_variant_new("(s)", NM_DBUS_INTERFACE_DEVICE),
            G_VARIANT_TYPE("(a{sv})"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            NULL);

        if (device_props != NULL) {
            GVariant *props_dict;
            g_variant_get(device_props, "(@a{sv})", &props_dict);

            const gchar *iface;
            guint32 device_type;
            gboolean managed, is_active;
            gchar *active_connection_path = NULL;

            g_variant_lookup(props_dict, "Interface", "&s", &iface);
            g_variant_lookup(props_dict, "DeviceType", "u", &device_type);
            g_variant_lookup(props_dict, "Managed", "b", &managed);
            g_variant_lookup(props_dict, "ActiveConnection", "o", &active_connection_path);

            is_active = (active_connection_path != NULL && strcmp(active_connection_path, "/") != 0);

            if (managed && num_connections < MAX_DEVICES) {
                strncpy(network_connections[num_connections].interface, iface, sizeof(network_connections[num_connections].interface) - 1);
                strncpy(network_connections[num_connections].device_path, device_path, sizeof(network_connections[num_connections].device_path) - 1);
                network_connections[num_connections].device_type = device_type;
                network_connections[num_connections].is_active = is_active;

                // Get IP address (simplified, assumes IPv4)
                if (is_active) {
                    GVariant *ip4_config = g_variant_lookup_value(props_dict, "Ip4Config", G_VARIANT_TYPE_OBJECT_PATH);
                    if (ip4_config != NULL) {
                        const gchar *ip4_path = g_variant_get_string(ip4_config, NULL);
                        GVariant *ip4_props = g_dbus_connection_call_sync(
                            dbus_connection,
                            NM_DBUS_SERVICE,
                            ip4_path,
                            "org.freedesktop.DBus.Properties",
                            "Get",
                            g_variant_new("(ss)", NM_DBUS_INTERFACE_IP4_CONFIG, "AddressData"),
                            G_VARIANT_TYPE("(v)"),
                            G_DBUS_CALL_FLAGS_NONE,
                            -1,
                            NULL,
                            NULL);

                        if (ip4_props != NULL) {
                            GVariant *address_variant;
                            g_variant_get(ip4_props, "(v)", &address_variant);
                            GVariantIter *addr_iter;
                            GVariant *addr_dict;
                            g_variant_get(address_variant, "aa{sv}", &addr_iter);
                            if (g_variant_iter_next(addr_iter, "@a{sv}", &addr_dict)) {
                                const gchar *ip4_address;
                                g_variant_lookup(addr_dict, "address", "&s", &ip4_address);
                                strncpy(network_connections[num_connections].ip_address, ip4_address, sizeof(network_connections[num_connections].ip_address) - 1);
                                g_variant_unref(addr_dict);
                            }
                            g_variant_iter_free(addr_iter);
                            g_variant_unref(address_variant);
                            g_variant_unref(ip4_props);
                        }
                        g_variant_unref(ip4_config);
                    }
                }

                // Get connection name
                if (is_active) {
                    GVariant *active_connection_props = g_dbus_connection_call_sync(
                        dbus_connection,
                        NM_DBUS_SERVICE,
                        active_connection_path,
                        "org.freedesktop.DBus.Properties",
                        "Get",
                        g_variant_new("(ss)", NM_DBUS_INTERFACE_ACTIVE_CONNECTION, "Id"),
                        G_VARIANT_TYPE("(v)"),
                        G_DBUS_CALL_FLAGS_NONE,
                        -1,
                        NULL,
                        NULL);

                    if (active_connection_props != NULL) {
                        GVariant *id_variant;
                        const gchar *id;
                        g_variant_get(active_connection_props, "(v)", &id_variant);
                        id = g_variant_get_string(id_variant, NULL);
                        strncpy(network_connections[num_connections].connection_name, id, sizeof(network_connections[num_connections].connection_name) - 1);
                        g_variant_unref(id_variant);
                        g_variant_unref(active_connection_props);
                    }
                }

                num_connections++;
            }

            g_variant_unref(props_dict);
            g_variant_unref(device_props);
            g_free(active_connection_path);
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(ret);
}

/**
 * @brief Scans for available Wi-Fi networks and retrieves the results.
 */

void scan_wifi_networks(const char *device_path) {
    GError *error = NULL;
    GDBusProxy *device_proxy;
    GVariant *ret;

    printf("\nScanning Wi-Fi networks on device: %s\n", device_path);
    device_proxy = g_dbus_proxy_new_sync(dbus_connection,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         NM_DBUS_SERVICE,
                                         device_path,
                                         NM_DBUS_INTERFACE_DEVICE_WIRELESS,
                                         NULL,
                                         &error);
    if (error != NULL) {
        fprintf(stderr, "Error creating device proxy: %s\n", error->message);
        g_error_free(error);
        return;
    }

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    GVariant *empty_options = g_variant_builder_end(&builder);

    printf("Requesting Wi-Fi scan...\n");
    ret = g_dbus_proxy_call_sync(
        device_proxy,
        "RequestScan",
        g_variant_new("(@a{sv})", empty_options),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (ret != NULL) {
        g_variant_unref(ret);
        printf("Scan requested successfully.\n");
	print_wifi_device_info(device_path);  // Move this line here
        sleep(5); // Wait for scan to complete
        retrieve_access_points(device_path);
    } else {
        fprintf(stderr, "Failed to request Wi-Fi scan: %s\n", error->message);
        g_error_free(error);
    }

    g_object_unref(device_proxy);
}

/**
 * @brief Waits for the Wi-Fi scan to complete, with a 30-second timeout.
 */
void wait_for_wifi_scan(const char *device_path) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_nsec - start.tv_nsec) / 1e9;

        if (elapsed >= WIFI_SCAN_TIMEOUT) {
            break;
        }

        GVariant *last_scan_variant = g_dbus_connection_call_sync(
            dbus_connection,
            NM_DBUS_SERVICE,
            device_path,
            "org.freedesktop.DBus.Properties",
            "Get",
            g_variant_new("(ss)", NM_DBUS_INTERFACE_DEVICE_WIRELESS, "LastScan"),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            NULL
        );

        if (last_scan_variant != NULL) {
            printf("LastScan property retrieved successfully\n");
            g_variant_unref(last_scan_variant);
        } else {
            printf("Failed to retrieve LastScan property\n");
        }

        usleep(1000000); // Sleep for 1 second to avoid busy waiting and reduce output
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    double total_time = (now.tv_sec - start.tv_sec) + (now.tv_nsec - start.tv_nsec) / 1e9;

    printf("Wi-Fi scan timed out after %.1f seconds.\n", total_time);
    printf("Wi-Fi scan did not complete successfully.\n");
}

int main(void) {
    pthread_t net_thread;
    pthread_t wifi_threads[MAX_DEVICES];
    int num_wifi_devices = 0;

    if (!init_dbus_connection()) {
        fprintf(stderr, "Failed to connect to D-Bus.\n");
        return 1;
    }
    printf("netscan v.11\n");

    // Start network listing thread
    pthread_create(&net_thread, NULL, network_list_thread, NULL);
    pthread_join(net_thread, NULL);

    printf("\n%d Network Interfaces:\n", num_connections);  // Move this line before the loop
    for (int i = 0; i < num_connections; i++) {
        printf("[%-8s | %-8s ] %-15s (%-15s): %s\n",
               network_connections[i].is_active ? "Active" : "Inactive",
               network_connections[i].device_type == NM_DEVICE_TYPE_ETHERNET ? "Wired" : (network_connections[i].device_type == NM_DEVICE_TYPE_WIFI ? "Wireless" : "Other"),
               network_connections[i].interface,
               network_connections[i].connection_name,
               network_connections[i].ip_address);
    }
    for (int i = 0; i < num_connections; i++) {
        if (network_connections[i].device_type == NM_DEVICE_TYPE_WIFI) {
            pthread_create(&wifi_threads[num_wifi_devices], NULL, wifi_scan_thread, network_connections[i].device_path);
            num_wifi_devices++;
        }
    }

    // Wait for all Wi-Fi scanning threads to finish
    for (int i = 0; i < num_wifi_devices; i++) {
        pthread_join(wifi_threads[i], NULL);
    }

    if (num_wifi_devices == 0) {
        printf("\nNo Wi-Fi networks found.\n");
    }

    // Cleanup
    g_dbus_connection_close_sync(dbus_connection, NULL, NULL);
    g_object_unref(dbus_connection);

    return 0;
}

