#include <src/hydrogen.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "mdns_client.h"

int mdns_client_tcp_check(const mdns_client_endpoint_t *ep, uint16_t port, int timeout_ms)
{
    int fd;
    int flags;
    struct sockaddr_storage ss;
    socklen_t slen;
    struct pollfd pfd;
    int soerr = 0;
    socklen_t soerr_len = sizeof soerr;

    if (!ep || port == 0) {
        return -1;
    }
    memset(&ss, 0, sizeof ss);
    if (ep->family == AF_INET && ep->addrlen >= 4) {
        struct sockaddr_in *in = (struct sockaddr_in *)&ss;
        in->sin_family = AF_INET;
        in->sin_port = htons(port);
        memcpy(&in->sin_addr, ep->addr, 4);
        slen = sizeof(*in);
    } else if (ep->family == AF_INET6 && ep->addrlen >= 16) {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&ss;
        in6->sin6_family = AF_INET6;
        in6->sin6_port = htons(port);
        memcpy(&in6->sin6_addr, ep->addr, 16);
        if (IN6_IS_ADDR_LINKLOCAL(&in6->sin6_addr)) {
            in6->sin6_scope_id = ep->ifindex;
        }
        slen = sizeof(*in6);
    } else {
        return -1;
    }

    fd = socket(ep->family, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    if (connect(fd, (struct sockaddr *)&ss, slen) < 0 && errno != EINPROGRESS) {
        close(fd);
        return -1;
    }
    pfd.fd = fd;
    pfd.events = POLLOUT;
    pfd.revents = 0;
    if (poll(&pfd, 1, timeout_ms < 0 ? 0 : timeout_ms) <= 0) {
        close(fd);
        return -1;
    }
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &soerr_len) < 0 || soerr != 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

void mdns_client_health_scan(mdns_client_t *client)
{
    size_t i;

    if (!client) {
        return;
    }
    pthread_mutex_lock(&client->lock);
    for (i = 0; i < client->nservices; i++) {
        mdns_client_service_t *svc = &client->services[i];
        mdns_client_endpoint_t eps[MDNS_CLIENT_MAX_ENDPOINTS];
        size_t ne;
        uint16_t port;
        char instance[MDNS_NAME_MAX];
        int timeout;
        int retries;
        int ok = 0;
        size_t e;
        int r;
        char addrbuf[INET6_ADDRSTRLEN];

        if (!mdns_client_service_visible(client, svc) || !svc->have_srv || svc->nendpoints == 0) {
            continue;
        }
        memcpy(eps, svc->endpoints, sizeof eps);
        ne = svc->nendpoints;
        port = svc->port;
        strncpy(instance, svc->instance, sizeof instance - 1);
        instance[sizeof instance - 1] = '\0';
        timeout = client->health_check_timeout_ms;
        retries = client->health_check_retries;
        pthread_mutex_unlock(&client->lock);

        for (e = 0; e < ne && !ok; e++) {
            for (r = 0; r < retries && !ok; r++) {
                if (mdns_client_tcp_check(&eps[e], port, timeout) == 0) {
                    ok = 1;
                    if (eps[e].family == AF_INET) {
                        inet_ntop(AF_INET, eps[e].addr, addrbuf, sizeof addrbuf);
                    } else {
                        inet_ntop(AF_INET6, eps[e].addr, addrbuf, sizeof addrbuf);
                    }
                    log_this(SR_MDNS_CLIENT, "MDNS_CLIENT HEALTH ok %s %s:%u", LOG_LEVEL_DEBUG, 3,
                             instance, addrbuf, (unsigned)port);
                }
            }
        }
        if (!ok) {
            if (eps[0].family == AF_INET) {
                inet_ntop(AF_INET, eps[0].addr, addrbuf, sizeof addrbuf);
            } else {
                inet_ntop(AF_INET6, eps[0].addr, addrbuf, sizeof addrbuf);
            }
            log_this(SR_MDNS_CLIENT, "MDNS_CLIENT HEALTH fail %s %s:%u", LOG_LEVEL_DEBUG, 3,
                     instance, addrbuf, (unsigned)port);
        }

        pthread_mutex_lock(&client->lock);
        if (i < client->nservices && mdns_name_equal(client->services[i].instance, instance)) {
            client->services[i].healthy = ok ? 1 : 0;
            if (mdns_client_on_change) {
                mdns_client_on_change(MDNS_CLIENT_EVT_HEALTH, &client->services[i]);
            }
        }
    }
    pthread_mutex_unlock(&client->lock);
}
