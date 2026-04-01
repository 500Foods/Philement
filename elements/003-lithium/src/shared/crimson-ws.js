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
 * Get the active tab name for the current manager.
 * Looks for an active tab button in the visible manager slot and reads its label.
 * @returns {string|null} Tab display name (e.g. "SQL", "JSON", "Preview") or null
 */
function getActiveTabName() {
  // Find the visible manager slot
  const slot = document.querySelector('.manager-slot.visible')
    || document.querySelector('.manager-slot');
  if (!slot) return null;

  // Look for an active tab button with a <span> label
  const activeBtn = slot.querySelector('.queries-tab-btn.active, .lookups-tab-btn.active, [data-tab].active');
  if (!activeBtn) return null;

  // Get the display name from the <span> inside the button
  const label = activeBtn.querySelector('span');
  return label?.textContent?.trim() || activeBtn.dataset?.tab || null;
}

/**
 * Format a build timestamp as "YYYY-MMM-DD" (e.g. "2026-Mar-29").
 * @param {string|number|null} buildTimestamp - ISO date string or epoch timestamp
 * @returns {string|null} Formatted date or null
 */
function formatBuildDate(buildTimestamp) {
  if (!buildTimestamp) return null;

  const MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
                  'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

  try {
    const date = typeof buildTimestamp === 'number'
      ? new Date(buildTimestamp)
      : new Date(buildTimestamp);

    if (isNaN(date.getTime())) return null;

    const year = date.getFullYear();
    const month = MONTHS[date.getMonth()];
    const day = String(date.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
  } catch (e) {
    return null;
  }
}

/**
 * Gather context information for the AI assistant
 * @returns {Object} Context packet with user, session, permissions, and view info
 */
function gatherContext() {
  const jwt = retrieveJWT();
  const claims = jwt ? getClaims(jwt) : null;

  // Get app state from window (exposed in app.js)
  const app = window.lithiumApp || null;
  const mainManager = app?._getMainManager?.() || app?.mainManagerInstance || null;
  const state = app?.getState?.() || {};

  // permissions.managers: pre-built list from MainManager, populated once
  // during menu load. Contains exactly the names the user sees in the sidebar.
  const accessibleManagerNames = mainManager?.accessibleManagerNames || [];

  // Current manager — use the instance's stored name if available
  const currentManagerId = app?.currentManager?.id || state.currentManager || null;
  const currentManagerName = app?.currentManager?.name || null;

  // Format the build date from the timestamp in version.json
  const buildDate = formatBuildDate(app?.timestamp || null);

  return {
    user: {
      id: claims?.user_id || null,
      username: claims?.username || 'User',
      displayName: claims?.display_name || claims?.username || 'User',
      roles: claims?.roles || [],
      preferences: {
        theme: localStorage.getItem('lithium_theme') || 'default',
        language: navigator.language || 'en-US'
      },
      login: {
        count: claims?.login_count || null,
        age: null
      }
    },
    session: {
      sessionId: window.lithiumLogs?.sessionId || null,
      loginTime: claims?.iat ? new Date(claims.iat * 1000).toISOString() : null,
      currentManager: currentManagerName,
      recentActivity: []
    },
    permissions: {
      managers: accessibleManagerNames,
      features: {}
    },
    currentView: {
      managerId: currentManagerId,
      managerName: currentManagerName,
      activeTab: getActiveTabName(),
      selectedRecord: null
    },
    lithiumVersion: state.version || 'unknown',
    buildDate: buildDate || 'unknown'
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
    this.chunkCount = 0; // Track chunk count for throttled logging
    this.accumulatedRetrieval = null; // Accumulate retrieval data from streaming chunks

    this._registerHandlers();
  }

  _registerHandlers() {
    log(Subsystems.CRIMSON, Status.DEBUG, 'WebSocket handlers registered');
    registerWSHandler('chat_chunk', (message) => this.handleChunk(message));
    registerWSHandler('chat_done', (message) => this.handleDone(message));
    registerWSHandler('chat_error', (message) => this.handleError(message));
  }

  _unregisterHandlers() {
    CHAT_MESSAGE_TYPES.forEach(type => unregisterWSHandler(type));
  }

  handleChunk(message) {
    const { id, chunk, content, reasoning_content, index, finish_reason } = message;
    if (id && !this.isCurrentRequest(id)) return;

    // Handle different possible chunk structures
    let chunkContent, chunkReasoning, chunkIndex, chunkFinishReason;
    let chunkRetrieval;

    if (chunk) {
      // Structure: { id, chunk: { content, reasoning_content, index, finish_reason, retrieval } }
      chunkContent = chunk.content;
      chunkReasoning = chunk.reasoning_content;
      chunkIndex = chunk.index;
      chunkFinishReason = chunk.finish_reason;
      // Check for retrieval data in chunk (provider-specific extra fields)
      chunkRetrieval = chunk.retrieval;
    } else if (content !== undefined) {
      // Structure: { id, content, reasoning_content, index, finish_reason, retrieval }
      chunkContent = content;
      chunkReasoning = reasoning_content;
      chunkIndex = index;
      chunkFinishReason = finish_reason;
      chunkRetrieval = message.retrieval;
    }

    // Accumulate retrieval data from chunks if present
    if (chunkRetrieval) {
      if (!this.accumulatedRetrieval) {
        this.accumulatedRetrieval = [];
      }
      // Retrieval might be an array or a single object
      if (Array.isArray(chunkRetrieval.retrieved_data)) {
        this.accumulatedRetrieval.push(...chunkRetrieval.retrieved_data);
      } else if (Array.isArray(chunkRetrieval)) {
        this.accumulatedRetrieval.push(...chunkRetrieval);
      } else if (chunkRetrieval.retrieved_data) {
        this.accumulatedRetrieval.push(chunkRetrieval.retrieved_data);
      } else {
        this.accumulatedRetrieval.push(chunkRetrieval);
      }
      log(Subsystems.CRIMSON, Status.DEBUG, `Accumulated retrieval from chunk: ${this.accumulatedRetrieval.length} items`);
    }

    if (this.onChunk && (chunkContent || chunkReasoning || chunkFinishReason)) {
      this.chunkCount++;
      this.onChunk(chunkContent, chunkIndex, chunkFinishReason, chunkReasoning);
    }
  }

  handleDone(message) {
    const { id, result } = message;

    // Debug: log message keys to see what we're getting (concise)
    log(Subsystems.CRIMSON, Status.DEBUG, `chat_done message keys: ${Object.keys(message).join(', ')}`);
    if (result) {
      log(Subsystems.CRIMSON, Status.DEBUG, `chat_done result keys: ${result ? Object.keys(result).join(', ') : 'null'}`);
    }

    if (id && !this.isCurrentRequest(id)) {
      return;
    }

    const callbacks = this.pendingRequests.get(id);

    // Merge accumulated retrieval data from streaming chunks into the result
    if (this.accumulatedRetrieval && this.accumulatedRetrieval.length > 0) {
      log(Subsystems.CRIMSON, Status.DEBUG, `Merging ${this.accumulatedRetrieval.length} accumulated retrieval items into result`);
      if (!result) {
        // Shouldn't happen, but handle gracefully
        result = { content: '' };
      }
      if (!result.raw_provider_response) {
        result.raw_provider_response = {};
      }
      if (!result.raw_provider_response.retrieval) {
        result.raw_provider_response.retrieval = {};
      }
      if (!result.raw_provider_response.retrieval.retrieved_data) {
        result.raw_provider_response.retrieval.retrieved_data = [];
      }
      // Merge accumulated items with existing ones (avoiding duplicates by id)
      const existingIds = new Set(result.raw_provider_response.retrieval.retrieved_data.map(r => r.id || r.filename));
      for (const item of this.accumulatedRetrieval) {
        const itemId = item.id || item.filename;
        if (!existingIds.has(itemId)) {
          result.raw_provider_response.retrieval.retrieved_data.push(item);
          existingIds.add(itemId);
        }
      }
    }

    // For streaming requests (chunkCount > 0), store the result so that
    // handleStreamFinished() in crimson.js can pick it up and finalize with
    // both the accumulated text buffer AND the result (which may contain
    // raw_provider_response with citation/retrieval data).
    // For non-streaming requests, call onDone immediately.
    if (this.chunkCount > 0) {
      this.pendingDoneResult = { id, result };
    } else if (this.onDone && result) {
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

    log(Subsystems.CRIMSON, Status.ERROR, `Error: ${error}`);

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

    // Reset chunk counter and accumulated retrieval for new request
    this.chunkCount = 0;
    this.accumulatedRetrieval = null;

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
    this.pendingDoneResult = null;
    this.chunkCount = 0;
    this.accumulatedRetrieval = null;
  }

  disconnect() {
    this.cleanup();
  }
}

let instance = null;

export function getCrimsonWS(options = {}) {
  if (!instance) {
    instance = new CrimsonWebSocket(options);
  } else {
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
