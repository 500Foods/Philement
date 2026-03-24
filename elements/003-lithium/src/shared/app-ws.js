/**
 * Application WebSocket Client
 *
 * Provides a persistent, app-wide WebSocket connection to the Hydrogen server.
 * This connection is established after login and maintained throughout the session.
 * Used by various modules (Crimson, notifications, etc.) for real-time communication.
 *
 * Features:
 * - Persistent connection (uses WebSocket protocol-level ping/pong for keepalive)
 * - Status indicator callbacks for UI feedback
 * - Graceful disconnect on logout
 */

import { getConfigValue } from '../core/config.js';
import { log, Subsystems, Status } from '../core/log.js';

// Connection states
export const ConnectionState = {
  DISCONNECTED: 'disconnected',
  CONNECTING: 'connecting',
  CONNECTED: 'connected',
  ERROR: 'error',
};

// Retry configuration
const RETRY_SCHEDULE = [
  { interval: 60000, attempts: 10 },   // Every 1 minute for 10 minutes (10 attempts)
  { interval: 120000, attempts: 5 },   // Every 2 minutes for 10 minutes (5 attempts)
  { interval: 300000, attempts: 2 },   // Every 5 minutes for 10 minutes (2 attempts)
  { interval: 600000, attempts: Infinity }, // Every 10 minutes indefinitely
];

/**
 * App-wide WebSocket Client class
 */
export class AppWebSocket {
  constructor(options = {}) {
    this.ws = null;
    this.state = ConnectionState.DISCONNECTED;
    this.userInitiatedDisconnect = false; // Track if user explicitly disconnected
    
    // Callbacks
    this.onStateChange = options.onStateChange || null;
    this.onSendActivity = options.onSendActivity || null; // For blue flash indicator
    this.onMessage = options.onMessage || null; // General message handler
    
    // Message handlers registered by modules
    this.messageHandlers = new Map(); // type -> handler function
    
    // Retry state
    this.retryTimeout = null;
    this.retryScheduleIndex = 0;
    this.retryAttemptInCurrentPhase = 0;
    this.totalRetryAttempts = 0;
    
    // Bind methods
    this.handleMessage = this.handleMessage.bind(this);
  }

  /**
   * Get the WebSocket URL from config
   * @returns {string} WebSocket URL
   */
  getWebSocketUrl() {
    const baseUrl = getConfigValue('server.websocket_url', 'wss://lithium.philement.com/wss');
    const wsKey = getConfigValue('server.websocket_key', 'ABCDEFGHIJKLMNOP');

    if (!wsKey) {
      log(Subsystems.MANAGER, Status.WARN, '[AppWS] No WebSocket key configured');
      return baseUrl;
    }

    const separator = baseUrl.includes('?') ? '&' : '?';
    return `${baseUrl}${separator}key=${encodeURIComponent(wsKey)}`;
  }

  /**
   * Get the WebSocket protocol from config
   * @returns {string} WebSocket protocol name
   */
  getWebSocketProtocol() {
    return getConfigValue('server.websocket_protocol', 'hydrogen');
  }

  /**
   * Update connection state
   * @param {string} newState - New connection state
   */
  setState(newState) {
    if (this.state !== newState) {
      const oldState = this.state;
      this.state = newState;
      log(Subsystems.MANAGER, Status.INFO, `[AppWS] State: ${oldState} -> ${newState}`);
      if (this.onStateChange) {
        this.onStateChange(newState, oldState);
      }
    }
  }

  /**
   * Register a message handler for a specific message type
   * @param {string} type - Message type to handle
   * @param {Function} handler - Handler function
   */
  registerHandler(type, handler) {
    this.messageHandlers.set(type, handler);
    log(Subsystems.MANAGER, Status.DEBUG, `[AppWS] Registered handler for: ${type}`);
  }

  /**
   * Unregister a message handler
   * @param {string} type - Message type to unregister
   */
  unregisterHandler(type) {
    this.messageHandlers.delete(type);
  }

  /**
   * Connect to the WebSocket server
   * @returns {Promise<void>}
   */
  connect() {
    return new Promise((resolve, reject) => {
      // If already connected, resolve immediately
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        resolve();
        return;
      }

      // Clean up any existing connection
      if (this.ws) {
        this.ws.onclose = null;
        this.ws.onerror = null;
        this.ws.onmessage = null;
        try {
          this.ws.close(1000, 'Reconnecting');
        } catch (e) {
          // Ignore errors during cleanup
        }
        this.ws = null;
      }

      this.setState(ConnectionState.CONNECTING);

      const url = this.getWebSocketUrl();
      const protocol = this.getWebSocketProtocol();
      log(Subsystems.MANAGER, Status.INFO, `[AppWS] Connecting to ${url}`);

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
        log(Subsystems.MANAGER, Status.INFO, '[AppWS] Connected successfully');
        this.resetRetryState();
        this.setState(ConnectionState.CONNECTED);
        resolve();
      };

      this.ws.onclose = (event) => {
        log(Subsystems.MANAGER, Status.INFO, `[AppWS] Disconnected: code=${event.code} reason="${event.reason}"`);
        this.ws = null;
        this.setState(ConnectionState.DISCONNECTED);
        
        // Schedule retry for unexpected disconnects (code !== 1000 means server closed or error)
        if (event.code !== 1000 && !this.userInitiatedDisconnect) {
          this.scheduleRetry();
        }
      };

      this.ws.onerror = (error) => {
        log(Subsystems.MANAGER, Status.ERROR, `[AppWS] Error: ${error.message || 'Unknown error'}`);
        if (!resolved) {
          resolved = true;
          this.ws = null;
          this.setState(ConnectionState.ERROR);
          // Schedule retry after error during connection
          this.scheduleRetry();
          reject(new Error('WebSocket connection failed'));
        } else {
          // Error after connection established - onclose will follow, which will schedule retry
          this.setState(ConnectionState.ERROR);
        }
      };

      this.ws.onmessage = (event) => {
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

  /**
   * Handle incoming WebSocket message
   * @param {string} data - Message data
   */
  handleMessage(data) {
    try {
      const message = JSON.parse(data);
      const { type } = message;

      log(Subsystems.MANAGER, Status.DEBUG, `[AppWS] Received: ${type}`);

      // Dispatch to registered handler
      const handler = this.messageHandlers.get(type);
      if (handler) {
        handler(message);
      } else if (this.onMessage) {
        this.onMessage(message);
      } else {
        log(Subsystems.MANAGER, Status.DEBUG, `[AppWS] No handler for message type: ${type}`);
      }
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[AppWS] Failed to parse message: ${error.message}`);
    }
  }

  /**
   * Send a message through the WebSocket
   * @param {Object} message - Message to send
   * @returns {boolean} True if sent successfully
   */
  send(message) {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      log(Subsystems.MANAGER, Status.WARN, '[AppWS] Cannot send - not connected');
      return false;
    }

    try {
      this.ws.send(JSON.stringify(message));
      this.notifySendActivity();
      return true;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[AppWS] Failed to send: ${error.message}`);
      return false;
    }
  }

  /**
   * Notify listeners of send activity (for blue flash indicator)
   */
  notifySendActivity() {
    if (this.onSendActivity) {
      this.onSendActivity();
    }
  }

  /**
   * Disconnect from the WebSocket server
   * @param {boolean} graceful - If true, send graceful close message
   * @param {boolean} userInitiated - If true, don't auto-reconnect
   */
  disconnect(graceful = true, userInitiated = false) {
    this.cancelRetry();
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

  /**
   * Reset retry state (call when connection is successfully established)
   */
  resetRetryState() {
    this.retryScheduleIndex = 0;
    this.retryAttemptInCurrentPhase = 0;
    this.totalRetryAttempts = 0;
    this.userInitiatedDisconnect = false;
    log(Subsystems.MANAGER, Status.DEBUG, '[AppWS] Retry state reset');
  }

  /**
   * Cancel any pending retry
   */
  cancelRetry() {
    if (this.retryTimeout) {
      clearTimeout(this.retryTimeout);
      this.retryTimeout = null;
      log(Subsystems.MANAGER, Status.DEBUG, '[AppWS] Cancelled pending retry');
    }
  }

  /**
   * Schedule a retry attempt based on the current retry schedule
   */
  scheduleRetry() {
    if (this.userInitiatedDisconnect) {
      log(Subsystems.MANAGER, Status.DEBUG, '[AppWS] Skipping retry - user initiated disconnect');
      return;
    }

    this.cancelRetry();

    const currentPhase = RETRY_SCHEDULE[this.retryScheduleIndex];
    if (!currentPhase) {
      log(Subsystems.MANAGER, Status.WARN, '[AppWS] No retry phase available');
      return;
    }

    // Check if we need to move to the next phase
    if (this.retryAttemptInCurrentPhase >= currentPhase.attempts) {
      if (this.retryScheduleIndex < RETRY_SCHEDULE.length - 1) {
        this.retryScheduleIndex++;
        this.retryAttemptInCurrentPhase = 0;
        log(Subsystems.MANAGER, Status.INFO, `[AppWS] Moving to retry phase ${this.retryScheduleIndex + 1}`);
      } else {
        // We're in the last phase (infinite), continue using it
        log(Subsystems.MANAGER, Status.DEBUG, '[AppWS] Using final retry phase (infinite)');
      }
    }

    const phase = RETRY_SCHEDULE[this.retryScheduleIndex];
    this.totalRetryAttempts++;
    this.retryAttemptInCurrentPhase++;

    log(Subsystems.MANAGER, Status.INFO, 
      `[AppWS] Scheduling retry #${this.totalRetryAttempts} in ${phase.interval / 1000}s ` +
      `(phase ${this.retryScheduleIndex + 1}, attempt ${this.retryAttemptInCurrentPhase}/${phase.attempts === Infinity ? '∞' : phase.attempts})`
    );

    this.retryTimeout = setTimeout(() => {
      this.retryTimeout = null;
      this.attemptRetry();
    }, phase.interval);
  }

  /**
   * Attempt to reconnect
   */
  async attemptRetry() {
    if (this.state === ConnectionState.CONNECTED || this.state === ConnectionState.CONNECTING) {
      log(Subsystems.MANAGER, Status.DEBUG, '[AppWS] Skipping retry - already connected or connecting');
      return;
    }

    log(Subsystems.MANAGER, Status.INFO, `[AppWS] Retry attempt #${this.totalRetryAttempts}`);
    
    try {
      await this.connect();
      log(Subsystems.MANAGER, Status.INFO, '[AppWS] Retry succeeded');
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, `[AppWS] Retry failed: ${error.message}`);
      // Schedule next retry (scheduleRetry will be called from onclose handler)
    }
  }

  /**
   * Check if connected
   * @returns {boolean} True if connected
   */
  isConnected() {
    return this.ws && this.ws.readyState === WebSocket.OPEN;
  }

  /**
   * Get current state
   * @returns {string} Current connection state
   */
  getState() {
    return this.state;
  }
}

// Singleton instance
let instance = null;

/**
 * Get the singleton AppWebSocket instance
 * @param {Object} options - Options for the client
 * @returns {AppWebSocket} WebSocket client instance
 */
export function getAppWS(options = {}) {
  if (!instance) {
    instance = new AppWebSocket(options);
  } else if (options && Object.keys(options).length > 0) {
    // Update callbacks on existing instance
    if (options.onStateChange) instance.onStateChange = options.onStateChange;
    if (options.onSendActivity) instance.onSendActivity = options.onSendActivity;
    if (options.onMessage) instance.onMessage = options.onMessage;
  }
  return instance;
}

/**
 * Initialize the WebSocket connection (call after login)
 * @param {Object} options - Callback options
 * @returns {Promise<AppWebSocket>} WebSocket client instance
 */
export async function initAppWS(options = {}) {
  const ws = getAppWS(options);
  
  try {
    await ws.connect();
    log(Subsystems.MANAGER, Status.INFO, '[AppWS] Connection established');
  } catch (error) {
    log(Subsystems.MANAGER, Status.WARN, `[AppWS] Connection failed: ${error.message}`);
    // Not fatal - connection will be retried
  }
  
  return ws;
}

/**
 * Gracefully disconnect the WebSocket
 * @param {boolean} userInitiated - If true, don't auto-reconnect (use for logout)
 */
export function disconnectAppWS(userInitiated = false) {
  if (instance) {
    instance.disconnect(true, userInitiated);
  }
}

/**
 * Destroy the singleton instance
 * @param {boolean} userInitiated - If true, don't auto-reconnect
 */
export function destroyAppWS(userInitiated = true) {
  if (instance) {
    instance.disconnect(true, userInitiated);
    instance = null;
  }
}

/**
 * Check if WebSocket is connected
 * @returns {boolean} True if connected
 */
export function isAppWSConnected() {
  return instance && instance.isConnected();
}

/**
 * Get the current WebSocket state
 * @returns {string} Current connection state
 */
export function getAppWSState() {
  return instance ? instance.getState() : ConnectionState.DISCONNECTED;
}

/**
 * Register a message handler for a specific message type
 * @param {string} type - Message type to handle
 * @param {Function} handler - Handler function
 */
export function registerWSHandler(type, handler) {
  const ws = getAppWS();
  ws.registerHandler(type, handler);
}

/**
 * Unregister a message handler
 * @param {string} type - Message type to unregister
 */
export function unregisterWSHandler(type) {
  if (instance) {
    instance.unregisterHandler(type);
  }
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
