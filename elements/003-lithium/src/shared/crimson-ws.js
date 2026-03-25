/**
 * Crimson WebSocket Chat Client
 *
 * Handles ONLY message submission and reception for chat.
 * Connection lifecycle is managed by app-ws.js.
 */

import { retrieveJWT, validateJWT, getClaims } from '../core/jwt.js';
import { log, Subsystems, Status } from '../core/log.js';
import { getAppWS, isAppWSConnected, registerWSHandler, unregisterWSHandler } from './app-ws.js';

const DEFAULT_ENGINE = 'Crimson';
const CHAT_MESSAGE_TYPES = ['chat_chunk', 'chat_done', 'chat_error'];

/**
 * Gather context information for the AI assistant
 * @returns {Object} Context packet with user, session, permissions, and view info
 */
function gatherContext() {
  const jwt = retrieveJWT();
  const claims = jwt ? getClaims(jwt) : null;

  // Get app state from window (exposed in app.js)
  const app = window.lithiumApp || null;
  const state = app?.getState?.() || {};

  return {
    user: {
      id: claims?.user_id || null,
      username: claims?.username || 'User',
      displayName: claims?.display_name || claims?.username || 'User',
      roles: claims?.roles || [],
      preferences: {
        theme: localStorage.getItem('lithium_theme') || 'default',
        language: navigator.language || 'en-US'
      }
    },
    session: {
      sessionId: window.lithiumLogs?.sessionId || null,
      loginTime: claims?.iat ? new Date(claims.iat * 1000).toISOString() : null,
      currentManager: state.currentManager || app?.currentManager?.id || null,
      recentActivity: []
    },
    permissions: {
      managers: claims?.managers || [],
      features: claims?.features || []
    },
    currentView: {
      managerId: app?.currentManager?.id || null,
      managerName: app?.currentManager?.name || null,
      activeTab: null,
      selectedRecord: null
    },
    lithiumVersion: state.version || 'unknown',
    buildDate: state.build || 'unknown'
  };
}

export class CrimsonWebSocket {
  constructor(options = {}) {
    this.requestId = 0;
    this.pendingRequests = new Map();
    this.currentRequestId = null;
    this.onChunk = options.onChunk || null;
    this.onDone = options.onDone || null;
    this.onError = options.onError || null;
    this.engine = options.engine || DEFAULT_ENGINE;
    this.requestTimeoutMs = 120000;

    this._registerHandlers();
  }

  _registerHandlers() {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Registering WebSocket handlers`);
    registerWSHandler('chat_chunk', (message) => this.handleChunk(message));
    registerWSHandler('chat_done', (message) => this.handleDone(message));
    registerWSHandler('chat_error', (message) => this.handleError(message));
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Handlers registered for chat_chunk, chat_done, chat_error`);
  }

  _unregisterHandlers() {
    CHAT_MESSAGE_TYPES.forEach(type => unregisterWSHandler(type));
  }

  handleChunk(message) {
    const { id, chunk, content, reasoning_content, index, finish_reason } = message;
    if (id && !this.isCurrentRequest(id)) return;

    // Handle different possible chunk structures
    let chunkContent, chunkReasoning, chunkIndex, chunkFinishReason;

    if (chunk) {
      // Structure: { id, chunk: { content, reasoning_content, index, finish_reason } }
      chunkContent = chunk.content;
      chunkReasoning = chunk.reasoning_content;
      chunkIndex = chunk.index;
      chunkFinishReason = chunk.finish_reason;
    } else if (content !== undefined) {
      // Structure: { id, content, reasoning_content, index, finish_reason }
      chunkContent = content;
      chunkReasoning = reasoning_content;
      chunkIndex = index;
      chunkFinishReason = finish_reason;
    }

    if (this.onChunk && (chunkContent || chunkReasoning || chunkFinishReason)) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Chunk received: content_len=${chunkContent?.length || 0}, reasoning_len=${chunkReasoning?.length || 0}, index=${chunkIndex}, finish=${chunkFinishReason || 'none'}`);
      this.onChunk(chunkContent, chunkIndex, chunkFinishReason, chunkReasoning);
    }
  }

  handleDone(message) {
    const { id, result } = message;
    
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] handleDone: id=${id}, isCurrentRequest=${this.isCurrentRequest(id)}, result keys: ${Object.keys(result || {}).join(', ')}`);
    
    if (id && !this.isCurrentRequest(id)) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Done ignored - not current request (current: ${this.currentRequestId})`);
      return;
    }
    
    const callbacks = this.pendingRequests.get(id);
    
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Done received for ${id}, result content length: ${result?.content?.length || 0}, has callbacks: ${!!callbacks}`);
    
    if (this.onDone && result) {
      this.onDone(result.content, result);
    }

    if (callbacks) {
      if (callbacks.onDone) callbacks.onDone(result);
      this.pendingRequests.delete(id);
    }
    
    if (id === this.currentRequestId) {
      this.currentRequestId = null;
    }
  }

  handleError(message) {
    const { id, error } = message;
    const callbacks = this.pendingRequests.get(id);

    log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Error: ${error}`);

    if (this.onError) this.onError(error);
    if (callbacks && callbacks.onError) callbacks.onError(new Error(error));

    this.pendingRequests.delete(id);
    if (id === this.currentRequestId) {
      this.currentRequestId = null;
    }
  }

  generateRequestId() {
    this.requestId++;
    return `crimson_${Date.now()}_${this.requestId}`;
  }

  async send(message, options = {}) {
    this.abortCurrentRequest();

    if (!isAppWSConnected()) {
      throw new Error('WebSocket not connected');
    }

    const ws = getAppWS();
    const jwt = retrieveJWT();
    if (!jwt) throw new Error('Not authenticated');

    const validation = validateJWT(jwt);
    if (!validation.valid) {
      throw new Error(validation.error === 'Token expired' ? 'Session expired' : `Auth error: ${validation.error}`);
    }
    if (!validation.claims?.database) {
      throw new Error('Missing database claim');
    }

    const requestId = this.generateRequestId();
    const stream = options.stream !== false;

    // Gather context for the AI assistant
    const context = gatherContext();
    const contextMessage = `[CURRENT SESSION CONTEXT]\n${JSON.stringify(context, null, 2)}\n[/CURRENT SESSION CONTEXT]`;

    const payload = {
      engine: this.engine,
      messages: [
        { role: 'system', content: contextMessage },  // Inject context as system message
        { role: 'user', content: message }
      ],
      stream,
      jwt: `Bearer ${jwt}`
    };

    if (options.history && Array.isArray(options.history)) {
      // Insert context as first system message, then history, then user message
      payload.messages = [
        { role: 'system', content: contextMessage },
        ...options.history,
        { role: 'user', content: message }
      ];
    }

    // Debug: Log context being sent
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Context injected as system message: user=${context?.user?.username}, manager=${context?.currentView?.managerName}`);

    const request = { type: 'chat', id: requestId, payload };

    return new Promise((resolve, reject) => {
      this.currentRequestId = requestId;

      const timeout = setTimeout(() => {
        if (this.pendingRequests.has(requestId)) {
          this.pendingRequests.delete(requestId);
          reject(new Error('Request timeout'));
        }
      }, this.requestTimeoutMs);

      this.pendingRequests.set(requestId, {
        onDone: (result) => {
          clearTimeout(timeout);
          resolve(result);
        },
        onError: (error) => {
          clearTimeout(timeout);
          reject(error);
        },
      });

      const sent = ws.send(request);
      if (!sent) {
        clearTimeout(timeout);
        this.pendingRequests.delete(requestId);
        reject(new Error('Failed to send'));
      }
    });
  }

  isConnected() {
    return isAppWSConnected();
  }

  abortCurrentRequest() {
    if (this.currentRequestId) {
      this.pendingRequests.delete(this.currentRequestId);
      this.currentRequestId = null;
    }
  }

  isCurrentRequest(requestId) {
    return requestId === this.currentRequestId;
  }

  cleanup() {
    this._unregisterHandlers();
    this.pendingRequests.clear();
    this.currentRequestId = null;
  }

  disconnect() {
    this.cleanup();
  }
}

let instance = null;

export function getCrimsonWS(options = {}) {
  log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] getCrimsonWS called, instance exists: ${!!instance}`);
  
  if (!instance) {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Creating new CrimsonWebSocket instance`);
    instance = new CrimsonWebSocket(options);
  } else {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[CrimsonWS] Reusing existing instance, re-registering handlers`);
    // Always re-register handlers - they may have been unregistered by cleanup()
    instance._registerHandlers();
    
    if (options) {
      if (options.onChunk) instance.onChunk = options.onChunk;
      if (options.onDone) instance.onDone = options.onDone;
      if (options.onError) instance.onError = options.onError;
      if (options.engine) instance.engine = options.engine;
    }
  }
  return instance;
}

export function destroyCrimsonWS() {
  if (instance) {
    instance.cleanup();
    instance = null;
  }
}

export default {
  CrimsonWebSocket,
  getCrimsonWS,
  destroyCrimsonWS,
};
