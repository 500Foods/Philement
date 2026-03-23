/**
 * Crimson WebSocket Chat Client
 *
 * Handles WebSocket communication with the Hydrogen server for AI chat.
 * Supports streaming responses for real-time chat experience.
 */

import { getConfigValue } from '../core/config.js';
import { retrieveJWT } from '../core/jwt.js';
import { log, Subsystems, Status } from '../core/log.js';

// Connection states
export const ConnectionState = {
  DISCONNECTED: 'disconnected',
  CONNECTING: 'connecting',
  CONNECTED: 'connected',
  AUTHENTICATED: 'authenticated',
  ERROR: 'error',
};

// Default engine for Crimson
const DEFAULT_ENGINE = 'Crimson';

/**
 * WebSocket Chat Client class
 */
export class CrimsonWebSocket {
  constructor(options = {}) {
    this.ws = null;
    this.state = ConnectionState.DISCONNECTED;
    this.requestId = 0;
    this.pendingRequests = new Map();
    this.onChunk = options.onChunk || null;
    this.onDone = options.onDone || null;
    this.onError = options.onError || null;
    this.onStateChange = options.onStateChange || null;
    this.engine = options.engine || DEFAULT_ENGINE;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = 3;
    this.reconnectDelay = 1000;
  }

  /**
   * Get the WebSocket URL from config
   * @returns {string} WebSocket URL
   */
  getWebSocketUrl() {
    return getConfigValue('server.websocket_url', 'wss://lithium.philement.com/wss');
  }

  /**
   * Update connection state
   * @param {string} newState - New connection state
   */
  setState(newState) {
    if (this.state !== newState) {
      this.state = newState;
      log(Subsystems.MANAGER, Status.INFO, `[CrimsonWS] State: ${newState}`);
      if (this.onStateChange) {
        this.onStateChange(newState);
      }
    }
  }

  /**
   * Connect to the WebSocket server
   * @returns {Promise<void>}
   */
  connect() {
    return new Promise((resolve, reject) => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        resolve();
        return;
      }

      this.setState(ConnectionState.CONNECTING);

      const url = this.getWebSocketUrl();
      log(Subsystems.MANAGER, Status.INFO, `[CrimsonWS] Connecting to ${url}`);

      try {
        this.ws = new WebSocket(url);
      } catch (error) {
        this.setState(ConnectionState.ERROR);
        reject(new Error(`Failed to create WebSocket: ${error.message}`));
        return;
      }

      this.ws.onopen = () => {
        log(Subsystems.MANAGER, Status.INFO, '[CrimsonWS] Connected');
        this.setState(ConnectionState.CONNECTED);
        this.reconnectAttempts = 0;
        resolve();
      };

      this.ws.onclose = (event) => {
        log(Subsystems.MANAGER, Status.INFO, `[CrimsonWS] Disconnected: ${event.code} ${event.reason}`);
        this.setState(ConnectionState.DISCONNECTED);
        this.cleanup();
      };

      this.ws.onerror = (error) => {
        log(Subsystems.MANAGER, Status.ERROR, `[CrimsonWS] Error: ${error.message || 'Unknown error'}`);
        this.setState(ConnectionState.ERROR);
        reject(new Error('WebSocket connection failed'));
      };

      this.ws.onmessage = (event) => {
        this.handleMessage(event.data);
      };
    });
  }

  /**
   * Disconnect from the WebSocket server
   */
  disconnect() {
    if (this.ws) {
      this.ws.close(1000, 'Client disconnect');
      this.ws = null;
    }
    this.setState(ConnectionState.DISCONNECTED);
    this.cleanup();
  }

  /**
   * Clean up pending requests
   */
  cleanup() {
    this.pendingRequests.forEach((callbacks) => {
      if (callbacks.onError) {
        callbacks.onError(new Error('Connection closed'));
      }
    });
    this.pendingRequests.clear();
  }

  /**
   * Handle incoming WebSocket message
   * @param {string} data - Message data
   */
  handleMessage(data) {
    try {
      const message = JSON.parse(data);
      const { type } = message;

      log(Subsystems.MANAGER, Status.DEBUG, `[CrimsonWS] Received: ${type}`);

      switch (type) {
        case 'chat_chunk':
          this.handleChunk(message);
          break;

        case 'chat_done':
          this.handleDone(message);
          break;

        case 'chat_error':
          this.handleError(message);
          break;

        default:
          log(Subsystems.MANAGER, Status.WARN, `[CrimsonWS] Unknown message type: ${type}`);
      }
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[CrimsonWS] Failed to parse message: ${error.message}`);
    }
  }

  /**
   * Handle chat chunk message
   * @param {Object} message - Chat chunk message
   */
  handleChunk(message) {
    const { chunk } = message;

    if (chunk && this.onChunk) {
      this.onChunk(chunk.content, chunk.index, chunk.finish_reason);
    }
  }

  /**
   * Handle chat done message
   * @param {Object} message - Chat done message
   */
  handleDone(message) {
    const { id, result } = message;
    const callbacks = this.pendingRequests.get(id);

    if (this.onDone && result) {
      this.onDone(result.content, result);
    }

    if (callbacks) {
      if (callbacks.onDone) {
        callbacks.onDone(result);
      }
      this.pendingRequests.delete(id);
    }
  }

  /**
   * Handle chat error message
   * @param {Object} message - Chat error message
   */
  handleError(message) {
    const { id, error } = message;
    const callbacks = this.pendingRequests.get(id);

    log(Subsystems.MANAGER, Status.ERROR, `[CrimsonWS] Chat error: ${error}`);

    if (this.onError) {
      this.onError(error);
    }

    if (callbacks && callbacks.onError) {
      callbacks.onError(new Error(error));
    }

    this.pendingRequests.delete(id);
  }

  /**
   * Generate a unique request ID
   * @returns {string} Request ID
   */
  generateRequestId() {
    this.requestId++;
    return `crimson_${Date.now()}_${this.requestId}`;
  }

  /**
   * Send a chat message
   * @param {string} message - User message
   * @param {Object} options - Additional options
   * @returns {Promise<Object>} Response promise
   */
  async send(message, options = {}) {
    // Ensure connected
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      await this.connect();
    }

    const jwt = retrieveJWT();
    if (!jwt) {
      throw new Error('Not authenticated - JWT required');
    }

    const requestId = this.generateRequestId();
    const stream = options.stream !== false; // Default to streaming

    const payload = {
      engine: this.engine,
      messages: [
        { role: 'user', content: message },
      ],
      stream,
      jwt,
    };

    // Add conversation history if provided
    if (options.history && Array.isArray(options.history)) {
      payload.messages = [...options.history, ...payload.messages];
    }

    const request = {
      type: 'chat',
      id: requestId,
      payload,
    };

    return new Promise((resolve, reject) => {
      this.pendingRequests.set(requestId, {
        onDone: resolve,
        onError: reject,
      });

      try {
        this.ws.send(JSON.stringify(request));
        log(Subsystems.MANAGER, Status.DEBUG, `[CrimsonWS] Sent chat request: ${requestId}`);
      } catch (error) {
        this.pendingRequests.delete(requestId);
        reject(new Error(`Failed to send message: ${error.message}`));
      }
    });
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

/**
 * Create a singleton instance
 */
let instance = null;

/**
 * Get the singleton WebSocket client instance
 * @param {Object} options - Options for the client
 * @returns {CrimsonWebSocket} WebSocket client instance
 */
export function getCrimsonWS(options = {}) {
  if (!instance) {
    instance = new CrimsonWebSocket(options);
  }
  return instance;
}

/**
 * Destroy the singleton instance
 */
export function destroyCrimsonWS() {
  if (instance) {
    instance.disconnect();
    instance = null;
  }
}

export default {
  CrimsonWebSocket,
  ConnectionState,
  getCrimsonWS,
  destroyCrimsonWS,
};
