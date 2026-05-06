/**
 * Crimson WebSocket Chat Client
 *
 * Handles ONLY message submission and reception for chat.
 * Connection lifecycle is managed by app-ws.js.
 */

import { retrieveJWT, validateJWT } from '../core/jwt.js';
import { log, Subsystems, Status } from '../core/log.js';
import { getAppWS, isAppWSConnected, registerWSHandler, unregisterWSHandler } from './app-ws.js';

const DEFAULT_ENGINE = 'Crimson';
const CHAT_MESSAGE_TYPES = ['chat_chunk', 'chat_done', 'chat_error'];

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
    const { id } = message;
    let { result } = message;

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

    const payload = {
      engine: this.engine,
      messages: [
        { role: 'user', content: message }
      ],
      stream,
      jwt: `Bearer ${jwt}`
    };

    if (options.history && Array.isArray(options.history)) {
      // Only include valid user/assistant messages, filter out any system/developer roles
      const validHistory = options.history.filter(msg => 
        msg.role === 'user' || msg.role === 'assistant'
      );
      payload.messages = [
        ...validHistory,
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
