/**
 * Crimson WebSocket Chat Client
 *
 * This module provides a chat-specific interface that uses the app-wide
 * WebSocket connection (app-ws.js) for communication with the Hydrogen server.
 * Supports streaming responses for real-time AI chat experience.
 */

import { retrieveJWT, validateJWT } from '../core/jwt.js';
import { log, Subsystems, Status } from '../core/log.js';
import { getAppWS, ConnectionState, registerWSHandler, unregisterWSHandler } from './app-ws.js';

// Re-export ConnectionState for compatibility
export { ConnectionState };

// Default engine for Crimson
const DEFAULT_ENGINE = 'Crimson';

// Message types handled by this module
const CHAT_MESSAGE_TYPES = ['chat_chunk', 'chat_done', 'chat_error'];

/**
 * Crimson Chat Client - wrapper around app-wide WebSocket
 */
export class CrimsonWebSocket {
  constructor(options = {}) {
    this.requestId = 0;
    this.pendingRequests = new Map();
    this.requestTimeouts = new Map();
    this.currentRequestId = null;
    this.onChunk = options.onChunk || null;
    this.onDone = options.onDone || null;
    this.onError = options.onError || null;
    this.onStateChange = options.onStateChange || null;
    this.engine = options.engine || DEFAULT_ENGINE;
    this.requestTimeoutMs = 120000; // 2 minute timeout for requests

    // Register handlers for chat message types
    this._registerHandlers();
  }

  /**
   * Register message handlers with the app-wide WebSocket
   */
  _registerHandlers() {
    registerWSHandler('chat_chunk', (message) => this.handleChunk(message));
    registerWSHandler('chat_done', (message) => this.handleDone(message));
    registerWSHandler('chat_error', (message) => this.handleError(message));
  }

  /**
   * Unregister message handlers
   */
  _unregisterHandlers() {
    CHAT_MESSAGE_TYPES.forEach(type => unregisterWSHandler(type));
  }

  /**
   * Handle chat chunk message
   * @param {Object} message - Chat chunk message
   */
  handleChunk(message) {
    const { id, chunk } = message;

    // Only process chunks for the current request
    if (id && !this.isCurrentRequest(id)) {
      log(Subsystems.MANAGER, Status.DEBUG, `[CrimsonWS] Ignoring chunk for cancelled request: ${id}`);
      return;
    }

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

    // Only process done for the current request
    if (id && !this.isCurrentRequest(id)) {
      log(Subsystems.MANAGER, Status.DEBUG, `[CrimsonWS] Ignoring done for cancelled request: ${id}`);
      return;
    }

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

    // Clear current request ID
    if (id === this.currentRequestId) {
      this.currentRequestId = null;
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
    // Abort any previous request before starting a new one
    this.abortCurrentRequest();

    const ws = getAppWS();
    
    // Check connection state
    if (!ws.isConnected()) {
      log(Subsystems.MANAGER, Status.INFO, '[CrimsonWS] Not connected, attempting to connect...');
      await ws.connect();
    }

    // Validate JWT before sending
    const jwt = retrieveJWT();
    if (!jwt) {
      throw new Error('Not authenticated - please log in to use the chat');
    }

    // Check JWT validity
    const validation = validateJWT(jwt);
    if (!validation.valid) {
      if (validation.error === 'Token expired') {
        throw new Error('Your session has expired - please log in again');
      }
      throw new Error(`Authentication error: ${validation.error}`);
    }

    // Check for required database claim
    if (!validation.claims || !validation.claims.database) {
      throw new Error('Authentication incomplete - missing database claim');
    }

    const requestId = this.generateRequestId();
    const stream = options.stream !== false; // Default to streaming

    const payload = {
      engine: this.engine,
      messages: [
        { role: 'user', content: message },
      ],
      stream,
      jwt: `Bearer ${jwt}`,
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
      // Track this as the current request
      this.currentRequestId = requestId;

      // Set up request timeout
      const timeout = setTimeout(() => {
        if (this.pendingRequests.has(requestId) && this.currentRequestId === requestId) {
          log(Subsystems.MANAGER, Status.WARN, `[CrimsonWS] Request timeout: ${requestId}`);
          this.pendingRequests.delete(requestId);
          this.requestTimeouts.delete(requestId);
          this.currentRequestId = null;
          reject(new Error('Request timeout - server did not respond in time'));
        }
      }, this.requestTimeoutMs);

      this.requestTimeouts.set(requestId, timeout);

      this.pendingRequests.set(requestId, {
        onDone: (result) => {
          if (this.currentRequestId === requestId) {
            clearTimeout(timeout);
            this.requestTimeouts.delete(requestId);
            this.currentRequestId = null;
            resolve(result);
          }
        },
        onError: (error) => {
          if (this.currentRequestId === requestId) {
            clearTimeout(timeout);
            this.requestTimeouts.delete(requestId);
            this.currentRequestId = null;
            reject(error);
          }
        },
      });

      // Send via app-wide WebSocket
      const sent = ws.send(request);
      if (!sent) {
        clearTimeout(timeout);
        this.requestTimeouts.delete(requestId);
        this.pendingRequests.delete(requestId);
        reject(new Error('Failed to send message - WebSocket not connected'));
      } else {
        log(Subsystems.MANAGER, Status.DEBUG, `[CrimsonWS] Sent chat request: ${requestId}`);
      }
    });
  }

  /**
   * Check if connected
   * @returns {boolean} True if connected
   */
  isConnected() {
    const ws = getAppWS();
    return ws.isConnected();
  }

  /**
   * Get current state
   * @returns {string} Current connection state
   */
  getState() {
    const ws = getAppWS();
    return ws.getState();
  }

  /**
   * Abort the current request (if any)
   */
  abortCurrentRequest() {
    if (this.currentRequestId) {
      log(Subsystems.MANAGER, Status.INFO, `[CrimsonWS] Aborting current request: ${this.currentRequestId}`);
      
      const timeout = this.requestTimeouts.get(this.currentRequestId);
      if (timeout) {
        clearTimeout(timeout);
        this.requestTimeouts.delete(this.currentRequestId);
      }
      
      this.pendingRequests.delete(this.currentRequestId);
      this.currentRequestId = null;
    }
  }

  /**
   * Check if a request ID is the current (non-cancelled) request
   * @param {string} requestId - Request ID to check
   * @returns {boolean} True if this is the current request
   */
  isCurrentRequest(requestId) {
    return requestId === this.currentRequestId;
  }

  /**
   * Cleanup - unregister handlers and clear pending requests
   */
  cleanup() {
    this._unregisterHandlers();
    this.requestTimeouts.forEach((timeout) => clearTimeout(timeout));
    this.requestTimeouts.clear();
    this.pendingRequests.forEach((callbacks) => {
      if (callbacks.onError) {
        callbacks.onError(new Error('Connection closed'));
      }
    });
    this.pendingRequests.clear();
  }

  /**
   * Disconnect (no-op for wrapper, connection is managed by app-ws)
   */
  disconnect() {
    // Connection is managed by app-ws, just cleanup our state
    this.cleanup();
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
  } else if (options && Object.keys(options).length > 0) {
    // Update callbacks on existing instance
    if (options.onChunk) instance.onChunk = options.onChunk;
    if (options.onDone) instance.onDone = options.onDone;
    if (options.onError) instance.onError = options.onError;
    if (options.onStateChange) instance.onStateChange = options.onStateChange;
    if (options.engine) instance.engine = options.engine;
  }
  return instance;
}

/**
 * Destroy the singleton instance
 */
export function destroyCrimsonWS() {
  if (instance) {
    instance.cleanup();
    instance = null;
  }
}

/**
 * Initialize page unload handler to clean up
 */
export function initWSUnloadHandler() {
  window.addEventListener('beforeunload', () => {
    destroyCrimsonWS();
  });
}

export default {
  CrimsonWebSocket,
  ConnectionState,
  getCrimsonWS,
  destroyCrimsonWS,
};
