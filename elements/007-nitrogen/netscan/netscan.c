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

#define MAX_DEVICES 10
#define MAX_SSIDS 50

static GDBusConnection *dbus_connection = NULL;

// Structures to hold network information
typedef struct {
    char interface[32];
    char connection_name[64];
    char ip_address[16];
} NetworkConnection;

typedef struct {
    char ssid[33];  // 32 chars + null terminator
    int strength;
} WifiNetwork;

// Global data
static NetworkConnection network_connections[MAX_DEVICES];
static int num_connections = 0;
static WifiNetwork wifi_networks[MAX_SSIDS];
static int num_wifi_networks = 0;
static char wifi_device_path[256] = {0};

// Function prototypes
gboolean init_dbus_connection();
void* network_list_thread(void* arg);
void* wifi_scan_thread(void* arg);
void get_active_connections();
void scan_wifi_networks();

int main() {
    pthread_t net_thread, wifi_thread;

    if (!init_dbus_connection()) {
        fprintf(stderr, "Failed to connect to D-Bus.\n");
        return 1;
    }

    // Start network listing thread
    pthread_create(&net_thread, NULL, network_list_thread, NULL);

    // Start Wi-Fi scanning thread (if Wi-Fi is available)
    if (strlen(wifi_device_path) > 0) {
        pthread_create(&wifi_thread, NULL, wifi_scan_thread, NULL);
    }

    // Wait for 10 seconds
    sleep(10);

    // Print network connections
    printf("Active Network Connections:\n");
    for (int i = 0; i < num_connections; i++) {
        printf("%s (%s): %s\n", 
               network_connections[i].interface,
               network_connections[i].connection_name,
               network_connections[i].ip_address);
    }

    // Print Wi-Fi networks
    if (strlen(wifi_device_path) > 0) {
        printf("\nAvailable Wi-Fi Networks:\n");
        for (int i = 0; i < num_wifi_networks; i++) {
            printf("%s (Strength: %d%%)\n", wifi_networks[i].ssid, wifi_networks[i].strength);
        }
    } else {
        printf("\nNo Wi-Fi interface available.\n");
    }

    // Cleanup
    g_dbus_connection_close_sync(dbus_connection, NULL, NULL);
    g_object_unref(dbus_connection);

    return 0;
}

gboolean init_dbus_connection() {
    GError *error = NULL;

    dbus_connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error != NULL) {
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

void* network_list_thread(void* arg) {
    (void)arg; // Suppress unused parameter warning
    get_active_connections();
    return NULL;
}

void* wifi_scan_thread(void* arg) {
    (void)arg; // Suppress unused parameter warning
    scan_wifi_networks();
    return NULL;
}

void get_active_connections() {
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

            const gchar *iface, *ip4_address, *driver;
            guint32 device_type;
            gboolean managed;

            g_variant_lookup(props_dict, "Interface", "s", &iface);
            g_variant_lookup(props_dict, "DeviceType", "u", &device_type);
            g_variant_lookup(props_dict, "Driver", "s", &driver);
            g_variant_lookup(props_dict, "Managed", "b", &managed);

            if (managed && num_connections < MAX_DEVICES) {
                strncpy(network_connections[num_connections].interface, iface, sizeof(network_connections[num_connections].interface) - 1);
                
                // Get IP address (simplified, assumes IPv4)
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
                            g_variant_lookup(addr_dict, "address", "s", &ip4_address);
                            strncpy(network_connections[num_connections].ip_address, ip4_address, sizeof(network_connections[num_connections].ip_address) - 1);
                            g_variant_unref(addr_dict);
                        }
                        g_variant_iter_free(addr_iter);
                        g_variant_unref(address_variant);
                        g_variant_unref(ip4_props);
                    }
                    g_variant_unref(ip4_config);
                }

                // Get connection name
                GVariant *active_connection = g_variant_lookup_value(props_dict, "ActiveConnection", G_VARIANT_TYPE_OBJECT_PATH);
                if (active_connection != NULL) {
                    const gchar *active_path = g_variant_get_string(active_connection, NULL);
                    GVariant *active_props = g_dbus_connection_call_sync(
                        dbus_connection,
                        NM_DBUS_SERVICE,
                        active_path,
                        "org.freedesktop.DBus.Properties",
                        "Get",
                        g_variant_new("(ss)", NM_DBUS_INTERFACE_ACTIVE_CONNECTION, "Id"),
                        G_VARIANT_TYPE("(v)"),
                        G_DBUS_CALL_FLAGS_NONE,
                        -1,
                        NULL,
                        NULL);

                    if (active_props != NULL) {
                        GVariant *id_variant;
                        const gchar *id;
                        g_variant_get(active_props, "(v)", &id_variant);
                        id = g_variant_get_string(id_variant, NULL);
                        strncpy(network_connections[num_connections].connection_name, id, sizeof(network_connections[num_connections].connection_name) - 1);
                        g_variant_unref(id_variant);
                        g_variant_unref(active_props);
                    }
                    g_variant_unref(active_connection);
                }

                num_connections++;

                // Store Wi-Fi device path if this is a Wi-Fi device
                if (device_type == NM_DEVICE_TYPE_WIFI) {
                    strncpy(wifi_device_path, device_path, sizeof(wifi_device_path) - 1);
                }
            }

            g_variant_unref(props_dict);
            g_variant_unref(device_props);
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(ret);
}

void scan_wifi_networks() {
    if (strlen(wifi_device_path) == 0) return;

    GVariant *ret;
    GError *error = NULL;

    // Request a scan
    ret = g_dbus_connection_call_sync(
        dbus_connection,
        NM_DBUS_SERVICE,
        wifi_device_path,
        NM_DBUS_INTERFACE_DEVICE_WIRELESS,
        "RequestScan",
        g_variant_new("(a{sv})", NULL),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error != NULL) {
        g_error_free(error);
        return;
    }
    g_variant_unref(ret);

    // Wait for scan to complete
    sleep(5);

    // Get access points
    ret = g_dbus_connection_call_sync(
        dbus_connection,
        NM_DBUS_SERVICE,
        wifi_device_path,
        NM_DBUS_INTERFACE_DEVICE_WIRELESS,
        "GetAccessPoints",
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
    const gchar *ap_path;
    g_variant_get(ret, "(ao)", &iter);

    while (g_variant_iter_next(iter, "&o", &ap_path) && num_wifi_networks < MAX_SSIDS) {
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
            NULL);

        if (ap_props != NULL) {
            GVariant *dict;
            g_variant_get(ap_props, "(@a{sv})", &dict);

            GVariant *v_ssid = g_variant_lookup_value(dict, "Ssid", G_VARIANT_TYPE_BYTESTRING);
            GVariant *v_strength = g_variant_lookup_value(dict, "Strength", G_VARIANT_TYPE_BYTE);

            if (v_ssid && v_strength) {
                gsize ssid_len;
                const char *ssid_data = g_variant_get_fixed_array(v_ssid, &ssid_len, 1);
                strncpy(wifi_networks[num_wifi_networks].ssid, ssid_data, ssid_len);
                wifi_networks[num_wifi_networks].ssid[ssid_len] = '\0';
                wifi_networks[num_wifi_networks].strength = g_variant_get_byte(v_strength);
                num_wifi_networks++;
            }

            if (v_ssid) g_variant_unref(v_ssid);
            if (v_strength) g_variant_unref(v_strength);
            g_variant_unref(dict);
            g_variant_unref(ap_props);
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(ret);
}
