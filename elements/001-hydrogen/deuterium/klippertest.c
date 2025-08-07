#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <jansson.h>
#include <time.h>

#include "klipperconn.h"
#include "queue.h"

int main() {
    clock_t start = clock();
    printf("Starting Klipper Proxy\n");

    // Initialize Klipper connection(s)
    klipper_connection* conn = klipper_init();
    if (!conn) {
        fprintf(stderr, "Failed to initialize Klipper connection\n");
        return 1;
    }

    printf("Klipper connection initialized successfully\n");

    // Start Klipper communication threads
    if (klipper_start_threads(conn) != 0) {
        fprintf(stderr, "Failed to start Klipper threads\n");
        klipper_cleanup(conn);
        return 1;
    }

    // Send the "objects/list" request
    json_t *request = json_pack("{s:s, s:i}", "method", "objects/list", "id", 1);
    klipper_send_command(conn, request);
    json_decref(request);

    // Wait for and process the response
    int timeout = 5; // 5 seconds timeout
    while (timeout > 0) {
        json_t *response = klipper_get_message(conn);
        if (response) {
            char *json_str = json_dumps(response, JSON_INDENT(2));
            if (json_str) {
                printf("Received response:\n%s\n", json_str);
                free(json_str);
            }
            json_decref(response);
            clock_t end = clock();
            double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
            printf("Time taken: %f seconds\n", time_taken);
            break;
        }
        sleep(1);
        timeout--;
    }

    if (timeout == 0) {
        printf("Timed out waiting for response\n");
    }

    // Now let's implement the subscription functionality
    json_t *subscribe_request = json_pack(
        "{s:s, s:{s:{s:n}}, s:i}",
        "method", "objects/subscribe",
        "params",
            "objects",
                "toolhead", json_null(),
        "id", 2
    );
    klipper_send_command(conn, subscribe_request);
    json_decref(subscribe_request);

    // Wait for and process the subscription response
    timeout = 5;
    while (timeout > 0) {
        json_t *response = klipper_get_message(conn);
        if (response) {
            char *json_str = json_dumps(response, JSON_INDENT(2));
            if (json_str) {
                printf("Received subscription response:\n%s\n", json_str);
                free(json_str);
            }
            json_decref(response);
            break;
        }
        sleep(1);
        timeout--;
    }

    if (timeout == 0) {
        printf("Timed out waiting for subscription response\n");
    }

    // Main loop to continuously receive updates
    printf("Waiting for updates (press Ctrl+C to stop)...\n");
    while (1) {
        json_t *update = klipper_get_message(conn);
        if (update) {
	    
	    json_t *root_params;
	    json_t *root_params_status;
	    json_t *root_params_status_toolhead;
	    json_t *root_params_status_toolhead_position;
	    root_params = json_object_get(update, "params");
	    root_params_status = json_object_get(root_params, "status");
	    root_params_status_toolhead = json_object_get(root_params_status, "toolhead");
	    root_params_status_toolhead_position = json_object_get(root_params_status_toolhead,"position");

            char *json_str = json_dumps(root_params_status_toolhead_position,JSON_COMPACT);
            if (json_str) {
                printf("Received update:%s\n", json_str);
                free(json_str);
            }
            json_decref(update);
        }
        usleep(100000);  // Sleep for 100ms to avoid busy-waiting
    }

    // Cleanup
    klipper_cleanup(conn);
    return 0;
}
