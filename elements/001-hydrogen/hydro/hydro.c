// hydro.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "systeminfo.h"

void cleanup_handler() {
    fprintf(stderr, "Cleanup handler called\n");
}

int main() {
    pthread_t thread;
    int ret;

    ret = pthread_create(&thread, NULL, systeminfo_thread, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to create thread\n");
        exit(1);
    }

    atexit(cleanup_handler);

    pthread_join(thread, NULL);

    return 0;
}
