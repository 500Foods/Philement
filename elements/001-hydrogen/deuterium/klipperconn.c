#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <jansson.h>
#include <dirent.h>
#include <sys/types.h>
#include <regex.h>
#include <errno.h>
#include <ctype.h>

#include "klipperconn.h"
#include "queue.h"

#define BUFFER_SIZE 4096
#define MAX_PATH 108

// Function to trim trailing whitespace and newline characters
static char* trim_whitespace(char* str) {
    char* end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

static char* find_klipper_socket() {
    DIR* dir;
    struct dirent* ent;
    char* socket_path = NULL;
    regex_t regex;

    printf("Searching for Klipper socket in /tmp\n");

    if (regcomp(&regex, "^klippy\\.sock$", REG_EXTENDED | REG_NOSUB) != 0) {
        fprintf(stderr, "Could not compile regex\n");
        return NULL;
    }

    dir = opendir("/tmp");
    if (dir == NULL) {
        perror("Could not open /tmp directory");
        regfree(&regex);
        return NULL;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (regexec(&regex, ent->d_name, 0, NULL, 0) == 0) {
            socket_path = malloc(strlen("/tmp/") + strlen(ent->d_name) + 1);
            if (!socket_path) {
                fprintf(stderr, "Memory allocation error\n");
                closedir(dir);
                regfree(&regex);
                return NULL;
            }
            sprintf(socket_path, "/tmp/%s", ent->d_name);
            printf("Found Klipper socket: %s\n", socket_path);
            break;
        }
    }

    closedir(dir);
    regfree(&regex);

    if (socket_path == NULL) {
        fprintf(stderr, "Could not find Klipper socket in /tmp\n");
    }

    return socket_path;
}

static char* find_config_file() {
    char* home_dir = getenv("HOME");
    if (home_dir == NULL) {
        fprintf(stderr, "Could not get HOME directory\n");
        return NULL;
    }

    printf("Searching for Moonraker config file\n");

    char* config_paths[] = {
        "/.config/moonraker.conf",
        "/klipper_config/moonraker.conf",
        "/printer_data/config/moonraker.conf",
        NULL
    };

    char* config_path = NULL;
    for (int i = 0; config_paths[i] != NULL; i++) {
        config_path = malloc(strlen(home_dir) + strlen(config_paths[i]) + 1);
        if (!config_path) {
            fprintf(stderr, "Memory allocation error\n");
            return NULL;
        }
        sprintf(config_path, "%s%s", home_dir, config_paths[i]);
        printf("Checking path: %s\n", config_path);
        if (access(config_path, F_OK) == 0) {
            printf("Found Moonraker config file: %s\n", config_path);
            return config_path;
        }
        free(config_path);
    }

    fprintf(stderr, "Could not find Moonraker config file\n");
    return NULL;
}

static char* parse_config_for_socket(const char* config_path) {
    FILE* file = fopen(config_path, "r");
    if (file == NULL) {
        perror("Could not open config file");
        return NULL;
    }

    printf("Parsing config file: %s\n", config_path);

    char line[256];
    char* socket_path = NULL;
    regex_t regex;

    if (regcomp(&regex, "^klippy_uds_address:\\s*(.+)$", REG_EXTENDED) != 0) {
        fprintf(stderr, "Could not compile regex\n");
        fclose(file);
        return NULL;
    }

    while (fgets(line, sizeof(line), file)) {
        regmatch_t matches[2];
        if (regexec(&regex, line, 2, matches, 0) == 0) {
            int start = matches[1].rm_so;
            int end = matches[1].rm_eo;
            socket_path = malloc(end - start + 1);
            if (!socket_path) {
                fprintf(stderr, "Memory allocation error\n");
                fclose(file);
                regfree(&regex);
                return NULL;
            }
            strncpy(socket_path, line + start, end - start);
            socket_path[end - start] = '\0';
            printf("Found klippy_uds_address: %s\n", socket_path);
            socket_path = trim_whitespace(socket_path);
            break;
        }
    }

    fclose(file);
    regfree(&regex);

    if (socket_path == NULL) {
        fprintf(stderr, "Could not find klippy_uds_address in config file\n");
    }

    return socket_path;
}

static void* sender_thread(void* arg) {
    klipper_connection* conn = (klipper_connection*)arg;
    while (!conn->should_stop) {
        json_t* command = queue_pop(conn->send_queue);
        if (command) {
            char* json_str = json_dumps(command, JSON_COMPACT);
            if (json_str) {
                size_t len = strlen(json_str);
                char* framed_str = malloc(len + 2);
                if (!framed_str) {
                    fprintf(stderr, "Memory allocation error\n");
                    free(json_str);
                    json_decref(command);
                    continue;
                }
                strcpy(framed_str, json_str);
                framed_str[len] = '\x03';
                framed_str[len + 1] = '\0';

                send(conn->socket_fd, framed_str, len + 1, 0);

                free(framed_str);
                free(json_str);
            }
            json_decref(command);
        }
        usleep(10000);
    }
    return NULL;
}

static void* receiver_thread(void* arg) {
    klipper_connection* conn = (klipper_connection*)arg;
    char buffer[BUFFER_SIZE];
    size_t buffer_pos = 0;

    while (!conn->should_stop) {
        ssize_t received = recv(conn->socket_fd, buffer + buffer_pos, BUFFER_SIZE - buffer_pos - 1, 0);
        if (received > 0) {
            buffer_pos += received;
            buffer[buffer_pos] = '\0';

            char* end;
            while ((end = strchr(buffer, '\x03')) != NULL) {
                *end = '\0';
                json_t* json = json_loads(buffer, 0, NULL);
                if (json) {
                    queue_push(conn->receive_queue, json);
                }
                memmove(buffer, end + 1, buffer_pos - (end - buffer));
                buffer_pos -= (end - buffer + 1);
            }
        }
        usleep(10000);
    }
    return NULL;
}

klipper_connection* klipper_init() {
    klipper_connection* conn = malloc(sizeof(klipper_connection));
    if (!conn) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    printf("Initializing Klipper connection\n");

    conn->socket_path = find_klipper_socket();
    if (conn->socket_path == NULL) {
        char* config_path = find_config_file();
        if (config_path != NULL) {
            conn->socket_path = parse_config_for_socket(config_path);
            free(config_path);
        }
    }

    if (conn->socket_path == NULL) {
        fprintf(stderr, "Failed to find Klipper socket path\n");
        free(conn);
        return NULL;
    }

    // Trim any trailing whitespace or newline characters from the socket path
    conn->socket_path = trim_whitespace(conn->socket_path);

    // Check path length
    if (strlen(conn->socket_path) >= sizeof(((struct sockaddr_un *)0)->sun_path)) {
        fprintf(stderr, "Socket path length exceeds limit: %s\n", conn->socket_path);
        free(conn->socket_path);
        free(conn);
        return NULL;
    }

    printf("Attempting to connect to socket: [%s]\n", conn->socket_path);

    // Additional check to verify socket file existence
    if (access(conn->socket_path, F_OK) != 0) {
        fprintf(stderr, "Socket file does not exist: [%s]\n", conn->socket_path);
        free(conn->socket_path);
        free(conn);
        return NULL;
    }

    int max_attempts = 5;
    int delay_ms = 100;

    for (int attempt = 1; attempt <= max_attempts; attempt++) {
        conn->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (conn->socket_fd == -1) {
            perror("Failed to create socket");
            free(conn->socket_path);
            free(conn);
            return NULL;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, conn->socket_path, sizeof(addr.sun_path) - 1);

        if (connect(conn->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            printf("Successfully connected to Klipper socket\n");
            break;
        }

        perror("Failed to connect to socket");
        close(conn->socket_fd);

        if (attempt == max_attempts) {
            fprintf(stderr, "Failed to connect to Klipper socket after %d attempts\n", max_attempts);
            free(conn->socket_path);
            free(conn);
            return NULL;
        }

        fprintf(stderr, "Connection attempt %d failed. Retrying in %d ms...\n", attempt, delay_ms);
        usleep(delay_ms * 1000);
    }

    conn->send_queue = queue_init();
    conn->receive_queue = queue_init();
    conn->should_stop = 0;

    return conn;
}

int klipper_start_threads(klipper_connection* conn) {
    if (pthread_create(&conn->sender_thread, NULL, sender_thread, conn) != 0) {
        fprintf(stderr, "Failed to create sender thread\n");
        return -1;
    }
    if (pthread_create(&conn->receiver_thread, NULL, receiver_thread, conn) != 0) {
        fprintf(stderr, "Failed to create receiver thread\n");
        return -1;
    }
    return 0;
}

void klipper_send_command(klipper_connection* conn, json_t* command) {
    queue_push(conn->send_queue, json_incref(command));
}

json_t* klipper_get_message(klipper_connection* conn) {
    return queue_pop(conn->receive_queue);
}

void klipper_cleanup(klipper_connection* conn) {
    conn->should_stop = 1;
    pthread_join(conn->sender_thread, NULL);
    pthread_join(conn->receiver_thread, NULL);
    close(conn->socket_fd);
    queue_free(conn->send_queue);
    queue_free(conn->receive_queue);
    free(conn->socket_path);
    free(conn);
}
