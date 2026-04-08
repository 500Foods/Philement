/**
 * Crimson Chat Module
 *
 * Handles WebSocket communication and message processing:
 * - WebSocket client initialization and message sending
 * - Streaming response handling with delimiter parsing
 * - Message creation and display
 * - Markdown formatting
 * - Follow-up questions and suggestions
 *
 * @module managers/crimson/crimson-chat
 */

import { processIcons } from '../../core/icons.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { getCrimsonWS } from '../../shared/crimson-ws.js';
import { isAppWSConnected } from '../../shared/app-ws.js';
import { CrimsonManager } from './crimson-core.js';

// ==================== WEBSOCKET INITIALIZATION ====================

/**
 * Initialize WebSocket client
 * Note: Connection lifecycle is managed by app-ws.js, not by Crimson
 */
CrimsonManager.prototype.initWebSocketClient = function() {
  this.wsClient = getCrimsonWS({
    onChunk: (content, index, finishReason, reasoningContent) => {
      this.handleStreamChunk(content, index, finishReason, reasoningContent);
    },
    onDone: (content, result) => {
      this.handleStreamDone(content, result);
    },
    onError: (error) => {
      this.handleStreamError(error);
    },
  });
};

// ==================== MESSAGE SENDING ====================

/**
 * Send chat message via WebSocket
 * Uses the persistent app-ws.js connection - does NOT manage connection lifecycle
 * @param {string} message - User message
 */
CrimsonManager.prototype.sendChatMessage = function(message) {
  this.initWebSocketClient();

  if (!isAppWSConnected()) {
    this.addDebugMessage('ERROR', 'WebSocket not connected');
    return;
  }

  // If already streaming, cancel the previous request
  if (this.isStreaming && this.wsClient) {
    this.addDebugMessage('CANCEL', 'Cancelling previous request');
    this.wsClient.abortCurrentRequest();

    if (this.currentStreamElement) {
      this.currentStreamElement.remove();
      this.currentStreamElement = null;
    }

    this.seenDelimiter = false;
    this.conversationBuffer = '';
    this.metadataBuffer = '';
    this.partialDelimiter = '';
  }

  this.isStreaming = true;
  this.chunkCount = 0;
  this.totalChunksReceived = 0;

  this.updateStatus('sending', 'Sending...');
  this.addDebugMessage('SEND', message);

  this.currentStreamElement = this.addStreamingMessage();
  this.clearReasoningBuffer();

  if (this.currentStreamElement) {
    this.currentStreamElement.classList.add('crimson-waiting');
  }

  const history = this.conversationHistory.slice(-10);
  this.addDebugMessage('SEND', `Sending with ${history.length} history messages, stream=${this.streamingEnabled}`);

  log(Subsystems.EVENTBUS, Status.INFO, `Crimson: Prompt sent (${message.length} chars)`);
  this.updateStatus('thinking', 'Thinking...');

  this.wsClient.send(message, {
    history,
    stream: this.streamingEnabled,
  }).catch((error) => {
    if (error.message.includes('abort') || error.message.includes('cancel')) {
      this.addDebugMessage('CANCELLED', 'Request was cancelled');
      return;
    }

    if (error.message.includes('not connected')) {
      this.addDebugMessage('ERROR', 'WebSocket not connected - please wait for connection');
      this.updateStatus('error', 'Connection not ready');
      this.input.focus();
      return;
    }

    log(Subsystems.CRIMSON, Status.ERROR, `Chat error: ${error.message}`);
    this.handleStreamError(error.message);
  });
};

// ==================== STREAM HANDLING ====================

/**
 * Handle incoming stream chunk - accumulates content and displays in real-time
 */
CrimsonManager.prototype.handleStreamChunk = function(content, _index, finishReason, reasoningContent) {
  if (reasoningContent) {
    if (this.connectionState === 'thinking') {
      this.updateStatus('reasoning', 'Reasoning...');
    }
    this.addReasoningContent(reasoningContent);
  }

  if (!content && !finishReason) return;

  if (this.currentStreamElement && this.currentStreamElement.classList.contains('crimson-waiting')) {
    this.currentStreamElement.classList.remove('crimson-waiting');
    const waitingEl = this.currentStreamElement.querySelector('.crimson-message-waiting');
    const contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
    if (waitingEl) waitingEl.classList.add('crimson-hidden');
    if (contentEl) contentEl.classList.remove('crimson-hidden');
  }

  let contentEl = null;
  if (this.currentStreamElement) {
    contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
  }

  if (!contentEl) return;

  this.totalChunksReceived++;

  if (this.totalChunksReceived % 100 === 0 || finishReason) {
    this.addDebugMessage('CHUNK', `#${this.totalChunksReceived}${finishReason ? ` (${finishReason})` : ''}`);
  }

  if (this.connectionState !== 'responding') {
    this.updateStatus('responding', 'Responding...');
  }

  if (this.seenDelimiter) {
    this.metadataBuffer += content;
    if (this.totalChunksReceived % 100 === 0) {
      this.addDebugMessage('META', `Chunk #${this.totalChunksReceived} added to metadata buffer (${this.metadataBuffer.length} total)`);
    }
  } else {
    const fullContent = this.partialDelimiter + content;
    this.partialDelimiter = '';

    const delimiterIndex = fullContent.indexOf(this.DELIMITER);
    if (delimiterIndex !== -1) {
      this.conversationBuffer += fullContent.substring(0, delimiterIndex);
      this.metadataBuffer = fullContent.substring(delimiterIndex + this.DELIMITER.length);
      this.seenDelimiter = true;
      this.addDebugMessage('DELIMITER', 'Found LITHIUM-CRIMSON-JSON delimiter');
    } else {
      let partialLen = 0;
      for (let i = Math.min(fullContent.length, this.DELIMITER.length - 1); i > 0; i--) {
        if (this.DELIMITER.startsWith(fullContent.slice(-i))) {
          partialLen = i;
          break;
        }
      }

      if (partialLen > 0) {
        this.partialDelimiter = fullContent.slice(-partialLen);
        this.conversationBuffer += fullContent.slice(0, -partialLen);
      } else {
        this.conversationBuffer += fullContent;
      }
    }

    const displayContent = this.conversationBuffer.replace(/\s+$/, '');
    contentEl.setAttribute('data-raw-content', displayContent);
    contentEl.innerHTML = this.formatMessageContent(displayContent) + '<span class="crimson-streaming-cursor"></span>';

    if (this.conversation) {
      this.conversation.scrollTop = this.conversation.scrollHeight;
    }
  }

  if (finishReason) {
    log(Subsystems.CRIMSON, Status.DEBUG, `Stream complete (${finishReason})`);
    this.addDebugMessage('COMPLETE', `Stream finished with reason: ${finishReason}`);
    if (this._finishTimeout) clearTimeout(this._finishTimeout);
    this._finishTimeout = setTimeout(() => {
      this._finishTimeout = null;
      this.handleStreamFinished();
    }, 100);
  }
};

/**
 * Handle stream completion after all chunks received
 */
CrimsonManager.prototype.handleStreamFinished = function() {
  if (!this.isStreaming) return;

  try {
    if (this.wsClient && this.wsClient.pendingDoneResult) {
      const { result } = this.wsClient.pendingDoneResult;
      this.wsClient.pendingDoneResult = null;
      this.addDebugMessage('DONE', `chat_done result available: keys=${result ? Object.keys(result).join(',') : 'null'}`);
      this.handleStreamDone(this.conversationBuffer, result);
    } else {
      this.addDebugMessage('DONE', 'Finalizing without chat_done result (timeout)');
      this.handleStreamDone(this.conversationBuffer, null);
    }
  } catch (error) {
    log(Subsystems.CRIMSON, Status.ERROR, `Error in handleStreamFinished: ${error.message}`);
  }

  if (this.wsClient) {
    const id = this.wsClient.currentRequestId;
    if (id && this.wsClient.pendingRequests?.has(id)) {
      const callbacks = this.wsClient.pendingRequests.get(id);
      if (callbacks?.onDone) {
        callbacks.onDone({ content: this.conversationBuffer });
        this.wsClient.pendingRequests.delete(id);
      }
    }
    this.wsClient.currentRequestId = null;
  }

  log(Subsystems.CRIMSON, Status.DEBUG, 'handleStreamFinished complete');
};

/**
 * Handle stream completion
 * @param {string} content - Final complete content
 * @param {Object} result - Full result object
 */
CrimsonManager.prototype.handleStreamDone = function(content, result) {
  log(Subsystems.CRIMSON, Status.DEBUG, `handleStreamDone called, isStreaming: ${this.isStreaming}, content length: ${content?.length || 0}`);

  if (result) {
    const resultKeys = Object.keys(result).join(', ');
    log(Subsystems.CRIMSON, Status.DEBUG, `handleStreamDone result keys: ${resultKeys}`);
    this.addDebugMessage('META', `Result keys: ${resultKeys}`);
  }

  log(Subsystems.EVENTBUS, Status.INFO, `Crimson: Response received (${content?.length || 0} chars)`);

  try {
    this.addDebugMessage('DONE', 'Stream complete');

    let conversationText = content || '';
    let metadataFromContent = null;

    if (this.metadataBuffer) {
      this.addDebugMessage('META', `Full metadata buffer (${this.metadataBuffer.length} chars): ${this.metadataBuffer}`);
    }

    if (this.conversationBuffer) {
      this.addDebugMessage('STREAM', `Conversation buffer (${this.conversationBuffer.length} chars): ${this.conversationBuffer.substring(0, 300)}...`);
    }

    if (content) {
      const delimiterIndex = content.indexOf(this.DELIMITER);
      if (delimiterIndex !== -1) {
        conversationText = content.substring(0, delimiterIndex).replace(/\s+$/, '');
        const metadataStr = content.substring(delimiterIndex + this.DELIMITER.length);
        this.addDebugMessage('META', `Raw metadata from delimiter: ${metadataStr.substring(0, 600)}`);
        try {
          metadataFromContent = JSON.parse(metadataStr);
          this.addDebugMessage('META', 'Parsed delimiter from content');
        } catch (e) {
          log(Subsystems.CRIMSON, Status.WARN, `Failed to parse metadata from content: ${e.message}`);
        }
      } else if (!this.seenDelimiter) {
        conversationText = content.replace(/\s+$/, '');
      }
    }

    if (this.isStreaming && this.conversationBuffer) {
      conversationText = this.conversationBuffer.replace(/\s+$/, '');
    }

    let parsedMetadata = metadataFromContent;

    if (!parsedMetadata && this.metadataBuffer && this.metadataBuffer.trim()) {
      try {
        parsedMetadata = JSON.parse(this.metadataBuffer.trim());
        this.addDebugMessage('META', 'Chunk metadata keys: ' + Object.keys(parsedMetadata).join(', '));
        this.addDebugMessage('META', 'From chunks: ' + JSON.stringify(parsedMetadata).substring(0, 500));
      } catch (e) {
        log(Subsystems.CRIMSON, Status.WARN, `Failed to parse metadata JSON: ${e.message}`);
      }
    }

    if (!parsedMetadata && result) {
      this.addDebugMessage('META', `Result keys: ${Object.keys(result).join(', ')}`);
      this.addDebugMessage('META', `Full result: ${JSON.stringify(result).substring(0, 800)}`);

      const rawResponse = result.raw_provider_response;
      if (rawResponse) {
        this.addDebugMessage('META', `Raw response keys: ${Object.keys(rawResponse).join(', ')}`);
      }

      parsedMetadata = {
        followUpQuestions: result.followUpQuestions || result.metadata?.followUpQuestions || rawResponse?.followUpQuestions || [],
        suggestions: result.suggestions || result.metadata?.suggestions || rawResponse?.suggestions || null,
        metadata: result.metadata || rawResponse?.metadata || null,
        citations: result.citations || result.metadata?.citations || rawResponse?.citations || rawResponse?.retrieval?.retrieved_data || null,
      };
      this.addDebugMessage('META', `Parsed metadata: ${JSON.stringify(parsedMetadata).substring(0, 300)}`);
    }

    if (result?.raw_provider_response) {
      const rawResponseJson = JSON.stringify(result.raw_provider_response, null, 2);
      log(Subsystems.CRIMSON, Status.DEBUG, `Raw provider response: ${rawResponseJson}`);
      this.addDebugMessage('RAW_RESPONSE', `Full raw response logged to session log (${rawResponseJson.length} chars)`);

      const retrievalData = result.raw_provider_response?.retrieval?.retrieved_data;
      if (retrievalData && Array.isArray(retrievalData)) {
        log(Subsystems.CRIMSON, Status.DEBUG, `Retrieval data: ${JSON.stringify(retrievalData, null, 2)}`);
        this.addDebugMessage('RETRIEVAL', `Found ${retrievalData.length} retrieval blocks`);
        retrievalData.forEach((item, idx) => {
          const filename = item.filename || item.name || item.id || 'unknown';
          const score = item.score !== undefined ? item.score : 'N/A';
          log(Subsystems.CRIMSON, Status.DEBUG, `Retrieval[${idx}]: filename="${filename}", score=${score}`);
        });
      }
    }

    if (!this.streamingEnabled && result) {
      const fullResultJson = JSON.stringify(result, null, 2);
      log(Subsystems.CRIMSON, Status.DEBUG, `Full result object (non-streaming): ${fullResultJson}`);
    }

    const modelCitations = parsedMetadata?.citations || [];
    const retrievalCitations = result?.raw_provider_response?.retrieval?.retrieved_data || [];

    if (retrievalCitations.length > 0) {
      this.addDebugMessage('META', `Retrieval citations found: ${retrievalCitations.length} items`);
      log(Subsystems.CRIMSON, Status.DEBUG, `Merging ${modelCitations.length} model citations with ${retrievalCitations.length} retrieval citations`);
    }

    const mergedCitations = [...modelCitations, ...retrievalCitations];

    if (mergedCitations.length > 0) {
      if (!parsedMetadata) {
        parsedMetadata = { followUpQuestions: [], suggestions: null, metadata: null };
      }
      parsedMetadata.citations = mergedCitations;
      this.addDebugMessage('META', `Total citations after merge: ${mergedCitations.length}`);
    }

    const messageId = `crimson-msg-${Date.now()}`;

    if (this.currentStreamElement) {
      this.currentStreamElement.id = messageId;

      if (this.currentStreamElement.classList.contains('crimson-waiting')) {
        this.currentStreamElement.classList.remove('crimson-waiting');
        const waitingEl = this.currentStreamElement.querySelector('.crimson-message-waiting');
        const contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
        if (waitingEl) waitingEl.classList.add('crimson-hidden');
        if (contentEl) contentEl.classList.remove('crimson-hidden');
      }

      const contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
      if (contentEl) {
        let turnMapping = new Map();
        this.addDebugMessage('META', `Citations check: ${JSON.stringify(parsedMetadata?.citations)}`);
        if (parsedMetadata && parsedMetadata.citations) {
          turnMapping = this.parseCitations(messageId, parsedMetadata.citations);
        }

        contentEl.setAttribute('data-raw-content', conversationText);

        this.formatMessageContentMarkdown(conversationText).then(htmlContent => {
          htmlContent = this.convertCitationMarkers(htmlContent, messageId, turnMapping);
          contentEl.innerHTML = htmlContent;
          if (this.conversation) {
            this.conversation.scrollTop = this.conversation.scrollHeight;
          }
        });

        if (parsedMetadata && parsedMetadata.followUpQuestions && parsedMetadata.followUpQuestions.length > 0) {
          this.addFollowUpQuestions(this.currentStreamElement, parsedMetadata.followUpQuestions);
        }

        if (parsedMetadata && parsedMetadata.metadata) {
          this.lastMetadata = parsedMetadata.metadata;
        }

        if (parsedMetadata && parsedMetadata.suggestions) {
          this.handleSuggestions(parsedMetadata.suggestions);
        }
      }

      this.currentStreamElement.classList.remove('crimson-streaming');
    }

    if (conversationText) {
      this.conversationHistory.push({ role: 'assistant', content: conversationText });
      this.messages.push({ sender: 'agent', text: conversationText, timestamp: Date.now() });
    }
  } catch (error) {
    log(Subsystems.CRIMSON, Status.ERROR, `Error in handleStreamDone: ${error.message}`);
    this.addDebugMessage('ERROR', error.message);
  } finally {
    this.currentStreamElement = null;
    this.isStreaming = false;
    this.seenDelimiter = false;
    this.conversationBuffer = '';
    this.metadataBuffer = '';
    this.partialDelimiter = '';
    this.input.focus();

    this.updateStatus('ready', 'Ready');

    if (this.conversation) {
      this.conversation.scrollTop = this.conversation.scrollHeight;
    }
  }
};

/**
 * Handle stream error
 * @param {string|Error} error - Error message or Error object
 */
CrimsonManager.prototype.handleStreamError = function(error) {
  const errorMessage = error instanceof Error ? error.message : error;

  if (errorMessage.includes('abort') || errorMessage.includes('cancel') || errorMessage.includes('Connection closed')) {
    this.addDebugMessage('CANCELLED', 'Request was cancelled or connection closed');
    return;
  }

  this.addDebugMessage('ERROR', errorMessage);
  this.updateStatus('error', `Error: ${errorMessage.substring(0, 50)}...`);

  if (this.currentStreamElement) {
    this.currentStreamElement.remove();
    this.currentStreamElement = null;
  }

  this.addMessage('agent', `Sorry, I encountered an error: ${errorMessage}`);

  this.isStreaming = false;
  this.seenDelimiter = false;
  this.conversationBuffer = '';
  this.metadataBuffer = '';
  this.partialDelimiter = '';
  this.input.focus();

  setTimeout(() => {
    this.updateStatus('ready', 'Ready');
  }, 3000);
};

// ==================== MESSAGE CREATION ====================

/**
 * Add a message to the conversation
 * @param {string} sender - 'user' or 'agent'
 * @param {string} text - Message text
 * @returns {string} The message element ID
 */
CrimsonManager.prototype.addMessage = function(sender, text) {
  const welcome = this.conversation.querySelector('.crimson-welcome');
  if (welcome) {
    welcome.classList.add('crimson-welcome-fading');
    const onTransitionEnd = (e) => {
      if (e.propertyName === 'opacity') {
        welcome.remove();
        welcome.removeEventListener('transitionend', onTransitionEnd);
      }
    };
    welcome.addEventListener('transitionend', onTransitionEnd);
    this.conversation.classList.remove('crimson-conversation-empty');
  }

  const messageId = `crimson-msg-${Date.now()}`;

  const messageEl = document.createElement('div');
  messageEl.className = `crimson-message crimson-message-${sender}`;
  messageEl.id = messageId;

  const avatar = sender === 'agent'
    ? `<div class="crimson-message-avatar">
        <i class="fa-kit-duotone fa-crimson"></i>
      </div>`
    : '<div class="crimson-message-avatar"><fa fa-user></fa></div>';

  messageEl.innerHTML = `
    ${avatar}
    <div class="crimson-message-content">${this.escapeHtml(text)}</div>
  `;

  this.conversation.appendChild(messageEl);
  processIcons(messageEl);

  this.conversation.scrollTop = this.conversation.scrollHeight;

  this.messages.push({ sender, text, timestamp: Date.now() });

  return messageId;
};

/**
 * Add a streaming message placeholder
 * @returns {HTMLElement} The streaming message element
 */
CrimsonManager.prototype.addStreamingMessage = function() {
  const welcome = this.conversation.querySelector('.crimson-welcome');
  if (welcome) {
    welcome.classList.add('crimson-welcome-fading');
    const onTransitionEnd = (e) => {
      if (e.propertyName === 'opacity') {
        welcome.remove();
        welcome.removeEventListener('transitionend', onTransitionEnd);
      }
    };
    welcome.addEventListener('transitionend', onTransitionEnd);
    this.conversation.classList.remove('crimson-conversation-empty');
  }

  this.seenDelimiter = false;
  this.conversationBuffer = '';
  this.metadataBuffer = '';
  this.partialDelimiter = '';

  const messageEl = document.createElement('div');
  messageEl.className = 'crimson-message crimson-message-agent crimson-streaming crimson-waiting';
  messageEl.innerHTML = `
    <div class="crimson-message-avatar">
     <i class="fa-kit-duotone fa-crimson"></i>
    </div>
    <div class="crimson-message-waiting">
      <div class="spinner-fancy spinner-fancy-sm"></div>
    </div>
    <div class="crimson-message-content crimson-hidden" data-raw-content=""></div>
  `;

  this.conversation.appendChild(messageEl);
  processIcons(messageEl);
  this.conversation.scrollTop = this.conversation.scrollHeight;

  return messageEl;
};

/**
 * Add typing indicator
 * @returns {HTMLElement} The typing indicator element
 */
CrimsonManager.prototype.addTypingIndicator = function() {
  const indicator = document.createElement('div');
  indicator.className = 'crimson-message crimson-message-agent crimson-typing-indicator';
  indicator.innerHTML = `
    <div class="crimson-message-avatar">
      <i class="fa-kit-duotone fa-crimson"></i>
    </div>
    <div class="crimson-message-content">
      <div class="crimson-typing">
        <div class="crimson-typing-dot"></div>
        <div class="crimson-typing-dot"></div>
        <div class="crimson-typing-dot"></div>
      </div>
    </div>
  `;
  this.conversation.appendChild(indicator);
  processIcons(indicator);
  this.conversation.scrollTop = this.conversation.scrollHeight;
  return indicator;
};

/**
 * Remove typing indicator
 */
CrimsonManager.prototype.removeTypingIndicator = function() {
  const indicator = this.conversation.querySelector('.crimson-typing-indicator');
  if (indicator) {
    indicator.remove();
  }
};

/**
 * Add follow-up question buttons after a message element
 * @param {HTMLElement} messageEl - The message element
 * @param {string[]} questions - Array of follow-up questions
 */
CrimsonManager.prototype.addFollowUpQuestions = function(messageEl, questions) {
  const existingFollowups = messageEl.nextElementSibling;
  if (existingFollowups?.classList.contains('crimson-followups')) {
    existingFollowups.remove();
  }

  const container = document.createElement('div');
  container.className = 'crimson-followups';

  questions.forEach(question => {
    const btn = document.createElement('button');
    btn.className = 'crimson-followup-btn';
    btn.innerHTML = `<span class="crimson-followup-icon"><fa fa-up></fa></span><span class="crimson-followup-text">${this.escapeHtml(question)}</span>`;
    btn.addEventListener('click', () => {
      this.input.value = question;
      this.handleSend();
    });
    container.appendChild(btn);
  });

  messageEl.parentNode.insertBefore(container, messageEl.nextSibling);
  processIcons(container);
};

// ==================== MESSAGE FORMATTING ====================

/**
 * Format message content with basic markdown support (for streaming)
 * @param {string} content - Raw content
 * @returns {string} Formatted HTML
 */
CrimsonManager.prototype.formatMessageContent = function(content) {
  if (!content) return '';

  const escaped = this.escapeHtml(content);

  const formatted = escaped
    .replace(/```([\s\S]*?)```/g, '<pre class="crimson-code"><code>$1</code></pre>')
    .replace(/`([^`]+)`/g, '<code class="crimson-inline-code">$1</code>')
    .replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>')
    .replace(/\*([^*]+)\*/g, '<em>$1</em>')
    .replace(/\n/g, '<br>');

  return formatted;
};

/**
 * Format message content with full markdown support using marked (for final content)
 * @param {string} content - Raw content
 * @returns {Promise<string>} Formatted HTML
 */
CrimsonManager.prototype.formatMessageContentMarkdown = async function(content) {
  if (!content) return '';

  try {
    const { marked } = await import('marked');
    marked.setOptions({
      gfm: true,
      breaks: true,
    });
    const htmlContent = await marked.parse(content);
    return htmlContent;
  } catch (error) {
    log(Subsystems.CRIMSON, Status.WARN, `Failed to render markdown: ${error.message}`);
    return this.formatMessageContent(content);
  }
};

// ==================== RESPONSE PARSING ====================

/**
 * Parse Crimson's JSON response
 * @param {string} content - Raw response content
 * @returns {Object} Parsed response
 */
CrimsonManager.prototype.parseCrimsonResponse = function(content) {
  const result = {
    displayMessage: content,
    followUpQuestions: [],
    suggestions: null,
    metadata: null,
    citations: null,
  };

  try {
    const json = JSON.parse(content);

    if (json.conversation && json.conversation.message) {
      result.displayMessage = json.conversation.message;
      result.followUpQuestions = json.conversation.followUpQuestions || [];
    }

    if (json.suggestions) {
      result.suggestions = json.suggestions;
    }

    if (json.metadata) {
      result.metadata = json.metadata;
    }

    if (json.citations) {
      result.citations = json.citations;
    } else if (json.metadata?.citations) {
      result.citations = json.metadata.citations;
    }
  } catch (e) {
    log(Subsystems.CRIMSON, Status.DEBUG, 'Response is plain text, not JSON');
  }

  return result;
};

/**
 * Handle suggestions from the AI response
 * @param {Object} suggestions - Suggestions object
 */
CrimsonManager.prototype.handleSuggestions = function(suggestions) {
  this.lastSuggestions = suggestions;

  if (suggestions.offerTours && suggestions.offerTours.length > 0) {
    log(Subsystems.CRIMSON, Status.DEBUG, `Tours available: ${JSON.stringify(suggestions.offerTours)}`);
  }
};

// ==================== DEPRECATED ====================

/**
 * Simulate Crimson response (placeholder - not used with WebSocket)
 * @deprecated Use WebSocket chat instead
 */
CrimsonManager.prototype.simulateCrimsonResponse = function() {
  log(Subsystems.CRIMSON, Status.WARN, 'simulateCrimsonResponse called but WebSocket should be used');
};
