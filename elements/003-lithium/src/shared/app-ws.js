/**
 * Application WebSocket Client
 *
 * Simple persistent WebSocket connection to the Hydrogen server.
 * - Created on app start (after login)
 * - Maintained until logout
 * - Auto-reconnects if connection drops for >10s
 * - Sends periodic keepalive messages to prevent load balancer timeouts
 */

import { getConfigValue } from '../core/config.js';
import { log, Subsystems, Status } from '../core/log.js';

export const ConnectionState = {
  DISCONNECTED: 'disconnected',
  CONNECTING: 'connecting',
  CONNECTED: 'connected',
  ERROR: 'error',
};

const RECONNECT_DELAY = 10000; // 10 seconds before attempting reconnect
const KEEPALIVE_INTERVAL = 25000; // Send keepalive every 25 seconds (before typical 30-60s LB timeout)

export class AppWebSocket {
  constructor(options = {}) {
    this.ws = null;
    this.state = ConnectionState.DISCONNECTED;
    this.userInitiatedDisconnect = false;
    this.reconnectTimeout = null;
    this.keepaliveInterval = null;
    
    // Callbacks
    this.onStateChange = options.onStateChange || null;
    this.onMessage = options.onMessage || null;
    
    // Message handlers registered by modules
    this.messageHandlers = new Map();
  }

  getWebSocketUrl() {
    const baseUrl = getConfigValue('server.websocket_url', 'wss://lithium.philement.com/wss');
    const wsKey = getConfigValue('server.websocket_key', 'ABCDEFGHIJKLMNOP');
    if (!wsKey) return baseUrl;
    const separator = baseUrl.includes('?') ? '&' : '?';
    return `${baseUrl}${separator}key=${encodeURIComponent(wsKey)}`;
  }

  getWebSocketProtocol() {
    return getConfigValue('server.websocket_protocol', 'hydrogen');
  }

  setState(newState) {
    if (this.state !== newState) {
      const oldState = this.state;
      this.state = newState;
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] State: ${oldState} -> ${newState}`);
      if (this.onStateChange) {
        this.onStateChange(newState, oldState);
      }
    }
  }

  registerHandler(type, handler) {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Registering handler for type: "${type}"`);
    this.messageHandlers.set(type, handler);
  }

  unregisterHandler(type) {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Unregistering handler for type: "${type}"`);
    this.messageHandlers.delete(type);
  }

  /**
   * Debug: List all registered handlers
   */
  debugHandlers() {
    const types = Array.from(this.messageHandlers.keys());
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Registered handlers: ${types.join(', ') || 'NONE'}`);
    return types;
  }

  connect() {
    return new Promise((resolve, reject) => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        resolve();
        return;
      }

      // Clean up any existing connection
      if (this.ws) {
        this.ws.onclose = null;
        this.ws.onerror = null;
        this.ws.onmessage = null;
        try { this.ws.close(1000, 'Reconnecting'); } catch { /* ignore close errors */ }
        this.ws = null;
      }

      this.setState(ConnectionState.CONNECTING);

      const url = this.getWebSocketUrl();
      const protocol = this.getWebSocketProtocol();
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Connecting to ${url}`);

      try {
        this.ws = new WebSocket(url, protocol);
      } catch (error) {
        this.setState(ConnectionState.ERROR);
        reject(new Error(`Failed to create WebSocket: ${error.message}`));
        return;
      }

      let resolved = false;

      this.ws.onopen = () => {
        if (resolved) return;
        resolved = true;
        this.connectedAt = Date.now();
        log(Subsystems.WEBSOCKET, Status.DEBUG, '[WS] Connection ESTABLISHED');
        this.userInitiatedDisconnect = false;
        this.setState(ConnectionState.CONNECTED);
        this.startKeepalive();
        resolve();
      };

      this.ws.onclose = (event) => {
        const wasClean = event.wasClean ? 'clean' : 'unclean';
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Connection CLOSED: code=${event.code} reason="${event.reason}" wasClean=${wasClean}`);
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Connection duration was approximately ${Date.now() - (this.connectedAt || Date.now())}ms`);
        this.stopKeepalive();
        this.ws = null;
        this.setState(ConnectionState.DISCONNECTED);
        
        // Auto-reconnect after delay if not user-initiated
        if (!this.userInitiatedDisconnect && event.code !== 1000) {
          this.scheduleReconnect();
        }
      };

      this.ws.onerror = (_error) => {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Error event fired, readyState=${this.ws?.readyState}`);
        if (!resolved) {
          resolved = true;
          this.ws = null;
          this.setState(ConnectionState.ERROR);
          reject(new Error('WebSocket connection failed'));
        }
      };

      this.ws.onmessage = (event) => {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Message received (${event.data.length} bytes)`);
        this.handleMessage(event.data);
      };

      // Connection timeout
      setTimeout(() => {
        if (!resolved && this.ws && this.ws.readyState !== WebSocket.OPEN) {
          resolved = true;
          this.ws.close();
          this.setState(ConnectionState.ERROR);
          reject(new Error('WebSocket connection timeout'));
        }
      }, 10000);
    });
  }

  handleMessage(data) {
    try {
      const message = JSON.parse(data);
      const { type } = message;

      // Log keepalive responses
      if (type === 'keepalive_ok') {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive #${this.keepaliveCount || '?'} response received`);
        return;
      }

      // Log all non-keepalive messages for debugging
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Received message type: "${type}", handlers registered: ${this.messageHandlers.size}, has handler: ${this.messageHandlers.has(type)}`);

      // Dispatch to registered handler
      const handler = this.messageHandlers.get(type);
      if (handler) {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Dispatching ${type} to registered handler`);
        handler(message);
      } else if (this.onMessage) {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] No handler for ${type}, using fallback onMessage`);
        this.onMessage(message);
      } else {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] No handler and no fallback for ${type}`);
      }
    } catch (error) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Failed to parse message: ${error.message}`);
    }
  }

  send(message) {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, '[WS] Cannot send - not connected');
      return false;
    }

    try {
      this.ws.send(JSON.stringify(message));
      return true;
    } catch (error) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Failed to send: ${error.message}`);
      return false;
    }
  }

  scheduleReconnect() {
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout);
    }
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Scheduling reconnect in ${RECONNECT_DELAY/1000}s`);
    this.reconnectTimeout = setTimeout(() => {
      this.reconnectTimeout = null;
      if (this.state !== ConnectionState.CONNECTED && this.state !== ConnectionState.CONNECTING) {
        log(Subsystems.WEBSOCKET, Status.DEBUG, '[WS] Attempting reconnect...');
        this.connect().catch(err => {
          log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Reconnect failed: ${err.message}`);
        });
      }
    }, RECONNECT_DELAY);
  }

  /**
   * Start sending periodic keepalive messages to prevent load balancer timeout
   * Uses lightweight keepalive message type (returns ~50 bytes vs ~6KB for status)
   */
  startKeepalive() {
    this.stopKeepalive();
    
    this.keepaliveCount = 0;
    this.keepaliveInterval = setInterval(() => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        this.keepaliveCount++;
        const keepaliveMsg = JSON.stringify({ type: 'keepalive' });
        try {
          this.ws.send(keepaliveMsg);
          log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive #${this.keepaliveCount} sent`);
        } catch (error) {
          log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive #${this.keepaliveCount} failed: ${error.message}`);
        }
      } else {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive skipped - ws state: ${this.ws?.readyState}`);
      }
    }, KEEPALIVE_INTERVAL);
    
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive started (every ${KEEPALIVE_INTERVAL/1000}s)`);
  }

  /**
   * Stop the keepalive interval
   */
  stopKeepalive() {
    if (this.keepaliveInterval) {
      clearInterval(this.keepaliveInterval);
      this.keepaliveInterval = null;
      log(Subsystems.WEBSOCKET, Status.DEBUG, '[WS] Keepalive stopped');
    }
  }

  disconnect(graceful = true, userInitiated = false) {
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout);
      this.reconnectTimeout = null;
    }
    this.stopKeepalive();
    this.userInitiatedDisconnect = userInitiated;
    
    if (this.ws) {
      if (graceful) {
        this.ws.close(1000, 'Client disconnect');
      } else {
        this.ws.close();
      }
      this.ws = null;
    }
    this.setState(ConnectionState.DISCONNECTED);
  }

  isConnected() {
    return this.ws && this.ws.readyState === WebSocket.OPEN;
  }

  getState() {
    return this.state;
  }
}

// Singleton
let instance = null;

export function getAppWS(options = {}) {
  if (!instance) {
    instance = new AppWebSocket(options);
  }
  return instance;
}

export async function initAppWS(options = {}) {
  const ws = getAppWS(options);
  try {
    await ws.connect();
  } catch (error) {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Initial connection failed: ${error.message}`);
  }
  return ws;
}

export function disconnectAppWS(userInitiated = false) {
  if (instance) {
    instance.disconnect(true, userInitiated);
  }
}

export function destroyAppWS(userInitiated = true) {
  if (instance) {
    instance.disconnect(true, userInitiated);
    instance = null;
  }
}

export function isAppWSConnected() {
  return instance && instance.isConnected();
}

export function getAppWSState() {
  return instance ? instance.getState() : ConnectionState.DISCONNECTED;
}

export function registerWSHandler(type, handler) {
  getAppWS().registerHandler(type, handler);
}

export function unregisterWSHandler(type) {
  if (instance) instance.unregisterHandler(type);
}

export default {
  AppWebSocket,
  ConnectionState,
  getAppWS,
  initAppWS,
  disconnectAppWS,
  destroyAppWS,
  isAppWSConnected,
  getAppWSState,
  registerWSHandler,
  unregisterWSHandler,
};
