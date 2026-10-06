/*
 * NATS reconnect delay and the single retry thread.
 *
 * MaxRetries < 0 retries forever. MaxRetries 0 is one attempt and no wait.
 * A drop after the link is up still reconnects, and does not consume a retry.
 * The wait wakes when shutdown is requested.
 */

#include <src/hydrogen.h>

#include <src/config/config_nats.h>
#include <src/nats/nats_internal.h>

#include <errno.h>
#include <time.h>

static pthread_mutex_t nats_wait_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t nats_wait_cv = PTHREAD_COND_INITIALIZER;

int nats_reconnect_delay_seconds(int failure_number) {
    const NATSReconnectConfig *reconnect;
    size_t index;

    if (failure_number < 1) {
        failure_number = 1;
    }
    if (!app_config) {
        return 30;
    }
    reconnect = &app_config->nats.Reconnect;
    if (reconnect->DelayCount == 0 || reconnect->DelayCount > NATS_MAX_DELAYS) {
        if (reconnect->SteadyDelaySeconds > 0) {
            return reconnect->SteadyDelaySeconds;
        }
        return 30;
    }
    index = (size_t)failure_number - 1;
    if (index >= reconnect->DelayCount) {
        if (reconnect->SteadyDelaySeconds > 0) {
            return reconnect->SteadyDelaySeconds;
        }
        return reconnect->Delays[reconnect->DelayCount - 1];
    }
    return reconnect->Delays[index];
}

bool nats_reconnect_should_retry(int failure_number) {
    int max_retries;

    if (!app_config || failure_number < 1) {
        return false;
    }
    max_retries = app_config->nats.Reconnect.MaxRetries;
    if (max_retries < 0) {
        return true;
    }
    return failure_number <= max_retries;
}

void nats_reconnect_wake(void) {
    if (pthread_mutex_lock(&nats_wait_mu) != 0) {
        nats_system_shutdown = 1;
        return;
    }
    nats_system_shutdown = 1;
    pthread_cond_broadcast(&nats_wait_cv);
    pthread_mutex_unlock(&nats_wait_mu);
}

void nats_reconnect_wait(int seconds) {
    struct timespec ts;

    if (seconds < 1) {
        seconds = 1;
    }
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return;
    }
    ts.tv_sec += seconds;
    if (pthread_mutex_lock(&nats_wait_mu) != 0) {
        return;
    }
    while (!nats_system_shutdown) {
        if (pthread_cond_timedwait(&nats_wait_cv, &nats_wait_mu, &ts) == ETIMEDOUT) {
            break;
        }
    }
    pthread_mutex_unlock(&nats_wait_mu);
}

void *nats_reconnect_thread(void *arg) {
    int failures = 0;

    (void)arg;
    while (!nats_system_shutdown) {
        int rc = nats_session_once();

        if (nats_system_shutdown) {
            break;
        }
        if (rc > 0) {
            failures = 0;
            nats_reconnect_wait(nats_reconnect_delay_seconds(1));
            continue;
        }
        if (rc == 0) {
            break;
        }
        failures++;
        if (!nats_reconnect_should_retry(failures)) {
            log_this(SR_NATS, "NATS reconnect stopped after %d failures",
                     LOG_LEVEL_ERROR, 1, failures);
            break;
        }
        nats_reconnect_wait(nats_reconnect_delay_seconds(failures));
    }
    return NULL;
}
