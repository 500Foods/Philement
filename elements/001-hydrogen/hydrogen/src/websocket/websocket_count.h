/*
 * Current WebSocket connection count.
 *
 * The NATS presence payload reads this. A missing server is 0.
 * The read takes the WebSocket context mutex.
 */

#ifndef WEBSOCKET_COUNT_H
#define WEBSOCKET_COUNT_H

int websocket_active_connection_count(void);

#endif /* WEBSOCKET_COUNT_H */
