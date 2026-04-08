/**
 * Crimson UI Module
 *
 * Adds UI helper methods to CrimsonManager:
 * - Debug panel management
 * - Reasoning panel management
 * - Input history persistence
 * - Status updates
 * - Welcome message
 * - Reset conversation
 *
 * @module managers/crimson/crimson-ui
 */

import { processIcons } from '../../core/icons.js';
import { log, getRawLog, Subsystems, Status } from '../../core/log.js';
import { getTip } from '../../core/tooltip-api.js';
import { formatLogText } from '../../shared/log-formatter.js';
import { CrimsonManager } from './crimson-core.js';

// ==================== STATUS ====================

/**
 * Update the status display in the header placeholder
 * @param {string} state - Status state: 'ready', 'sending', 'thinking', 'reasoning', 'responding', 'error'
 * @param {string} text - Status text to display
 */
CrimsonManager.prototype.updateStatus = function(state, text) {
  if (!this.statusIndicator || !this.statusText) return;

  this.connectionState = state;
  this.statusIndicator.className = `crimson-status-indicator crimson-status-${state}`;
  this.statusText.textContent = text;
};

// ==================== DEBUG PANEL ====================

/**
 * Toggle the debug panel visibility with animation
 */
CrimsonManager.prototype.toggleDebugPanel = function() {
  if (!this.debugPanel) return;

  this.debugMode = !this.debugMode;
  this.popup.classList.toggle('crimson-debug-visible', this.debugMode);
  this.debugBtn?.classList.toggle('active', this.debugMode);
  this.savePersistentState();
  this.updateDebugPanel();
};

/**
 * Add debug message to the session log
 * @param {string} type - Message type
 * @param {string} content - Message content
 */
CrimsonManager.prototype.addDebugMessage = function(type, content) {
  log(Subsystems.CRIMSON, Status.DEBUG, `[${type}] ${content}`);

  if (this.debugMode) {
    this.updateDebugPanel();
  }
};

/**
 * Update the debug panel content with session log format
 */
CrimsonManager.prototype.updateDebugPanel = function() {
  if (!this.debugContent) return;

  const allEntries = getRawLog();
  const crimsonEntries = allEntries.filter(e =>
    e.subsystem === 'Crimson' ||
    (e.subsystem && e.subsystem.startsWith('[EventBus]') && e.description && e.description.includes('Crimson'))
  );

  const logText = formatLogText(crimsonEntries);
  this.debugContent.textContent = logText;
  this.debugContent.scrollTop = this.debugContent.scrollHeight;
};

// ==================== STREAMING ====================

/**
 * Toggle streaming mode
 */
CrimsonManager.prototype.toggleStreaming = function() {
  this.streamingEnabled = !this.streamingEnabled;
  this.updateStreamingButton();
  this.addDebugMessage('CONFIG', `Streaming ${this.streamingEnabled ? 'enabled' : 'disabled'}`);
  this.savePersistentState();
};

/**
 * Update streaming button icon
 */
CrimsonManager.prototype.updateStreamingButton = function() {
  if (!this.streamingBtn) return;
  const icon = this.streamingBtn.querySelector('fa');
  if (icon) {
    icon.setAttribute('fa', this.streamingEnabled ? 'water' : 'lines-leaning');
    processIcons(this.streamingBtn);
  }
  this.streamingBtn?.classList.toggle('active', !this.streamingEnabled);
  this.streamingBtn?.setAttribute('data-tooltip', `Streaming: ${this.streamingEnabled ? 'ON' : 'OFF'}`);
  if (this.streamingBtn) {
    const t = getTip(this.streamingBtn);
    if (t) t.updateContent(`Streaming: ${this.streamingEnabled ? 'ON' : 'OFF'}`);
  }
};

// ==================== REASONING PANEL ====================

/**
 * Toggle reasoning panel visibility with slide animation
 */
CrimsonManager.prototype.toggleReasoningPanel = function() {
  if (!this.reasoningPanel) return;

  this.reasoningMode = !this.reasoningMode;
  this.popup.classList.toggle('crimson-reasoning-visible', this.reasoningMode);
  this.reasoningBtn?.classList.toggle('active', this.reasoningMode);
  this.updateReasoningButton();

  if (this.reasoningMode && this.reasoningContent && this.currentReasoningBuffer) {
    this.reasoningContent.textContent = this.currentReasoningBuffer;
    this.reasoningContent.scrollTop = this.reasoningContent.scrollHeight;
  }

  this.savePersistentState();
};

/**
 * Update reasoning button icon
 */
CrimsonManager.prototype.updateReasoningButton = function() {
  if (!this.reasoningBtn) return;
  const icon = this.reasoningBtn.querySelector('fa');
  if (icon) {
    icon.setAttribute('fa', this.reasoningMode ? 'person-running' : 'person');
    processIcons(this.reasoningBtn);
  }
  this.reasoningBtn?.setAttribute('data-tooltip', `Reasoning: ${this.reasoningMode ? 'SHOWING' : 'HIDDEN'}`);
  if (this.reasoningBtn) {
    const t = getTip(this.reasoningBtn);
    if (t) t.updateContent(`Reasoning: ${this.reasoningMode ? 'SHOWING' : 'HIDDEN'}`);
  }
};

/**
 * Add reasoning content to the reasoning panel
 * @param {string} content - Reasoning content from reasoning_content field
 */
CrimsonManager.prototype.addReasoningContent = function(content) {
  if (!content) return;

  this.currentReasoningBuffer += content;
  this.reasoningChunks.push(content);

  if (this.reasoningMode && this.reasoningContent) {
    this.reasoningContent.textContent = this.currentReasoningBuffer;
    this.reasoningContent.scrollTop = this.reasoningContent.scrollHeight;
  }
};

/**
 * Clear reasoning buffer for new response
 */
CrimsonManager.prototype.clearReasoningBuffer = function() {
  this.currentReasoningBuffer = '';
  this.reasoningChunks = [];
  if (this.reasoningContent) {
    this.reasoningContent.textContent = '';
  }
};

// ==================== INPUT HISTORY ====================

/**
 * Load input history from localStorage
 */
CrimsonManager.prototype.loadInputHistory = function() {
  try {
    const stored = localStorage.getItem(CrimsonManager.STORAGE_KEYS.INPUT_HISTORY);
    if (stored) {
      this.inputHistory = JSON.parse(stored);
    }
  } catch (e) {
    this.inputHistory = [];
  }
  this.inputHistoryIndex = -1;
  this.updateInputHistoryButtons();
};

/**
 * Save input history to localStorage
 */
CrimsonManager.prototype.saveInputHistory = function() {
  try {
    const toSave = this.inputHistory.slice(-CrimsonManager.MAX_INPUT_HISTORY);
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.INPUT_HISTORY, JSON.stringify(toSave));
  } catch (e) {
    // localStorage may not be available
  }
};

/**
 * Add input to history and reset pointer
 * @param {string} text - The input text to save
 */
CrimsonManager.prototype.addInputToHistory = function(text) {
  if (!text || !text.trim()) return;

  if (this.inputHistory.length > 0 && this.inputHistory[this.inputHistory.length - 1] === text.trim()) {
    this.inputHistoryIndex = -1;
    this.updateInputHistoryButtons();
    return;
  }

  this.inputHistory.push(text.trim());
  this.inputHistoryIndex = -1;
  this.saveInputHistory();
  this.updateInputHistoryButtons();
};

/**
 * Navigate through input history
 * @param {number} direction - -1 for previous, 1 for next
 */
CrimsonManager.prototype.navigateInputHistory = function(direction) {
  if (this.inputHistory.length === 0) return;

  let newIndex;
  if (this.inputHistoryIndex === -1) {
    if (direction === -1) {
      newIndex = this.inputHistory.length - 1;
    } else {
      return;
    }
  } else {
    newIndex = this.inputHistoryIndex + direction;
  }

  if (newIndex < 0) {
    newIndex = -1;
  } else if (newIndex >= this.inputHistory.length) {
    newIndex = -1;
  }

  this.inputHistoryIndex = newIndex;

  if (this.inputHistoryIndex === -1) {
    this.input.value = '';
  } else {
    this.input.value = this.inputHistory[this.inputHistoryIndex];
  }

  this.input.selectionStart = this.input.selectionEnd = this.input.value.length;
  this.autoResizeInput();
  this.updateInputHistoryButtons();
};

/**
 * Update the state of input history navigation buttons
 */
CrimsonManager.prototype.updateInputHistoryButtons = function() {
  if (this.inputPrevBtn) {
    this.inputPrevBtn.disabled = this.inputHistory.length === 0 || this.inputHistoryIndex === 0;
  }
  if (this.inputNextBtn) {
    this.inputNextBtn.disabled = this.inputHistoryIndex === -1;
  }
};

// ==================== WELCOME & RESET ====================

/**
 * Apply persistent state to UI elements
 */
CrimsonManager.prototype.applyPersistentState = function() {
  this.updateStreamingButton();

  this.debugBtn?.classList.toggle('active', this.debugMode);
  this.updateDebugPanel();

  this.reasoningBtn?.classList.toggle('active', this.reasoningMode);

  requestAnimationFrame(() => {
    this.popup.classList.toggle('crimson-debug-visible', this.debugMode);
    this.popup.classList.toggle('crimson-reasoning-visible', this.reasoningMode);
  });
};

/**
 * Show the welcome message in the conversation area
 */
CrimsonManager.prototype.showWelcomeMessage = function() {
  if (this.conversation) {
    this.conversation.classList.add('crimson-conversation-empty');
    this.conversation.innerHTML = `
      <div class="crimson-welcome">
        <div class="crimson-welcome-icon">
          <i class="fa-kit-duotone fa-crimson"></i>
        </div>
        <div class="crimson-welcome-text">Hello, <span class="crimson-username">${this.escapeHtml(this.username)}</span>!</div>
        <div class="crimson-welcome-hint">How can I help you today?</div>
      </div>
    `;
    processIcons(this.conversation);
    requestAnimationFrame(() => {
      const welcome = this.conversation.querySelector('.crimson-welcome');
      if (welcome) {
        welcome.classList.add('crimson-welcome-visible');
      }
    });
  }
};

/**
 * Reset the conversation to empty state
 */
CrimsonManager.prototype.resetConversation = function() {
  const resetBtn = this.popup?.querySelector('.crimson-reset-btn');
  if (resetBtn) {
    resetBtn.classList.add('animating');
    setTimeout(() => {
      resetBtn.classList.remove('animating');
    }, 1500);
  }

  if (this.wsClient) {
    this.wsClient.cleanup();
    this.wsClient = null;
  }

  if (this._finishTimeout) {
    clearTimeout(this._finishTimeout);
    this._finishTimeout = null;
  }
  this.isStreaming = false;
  this.currentStreamElement = null;
  this.conversationHistory = [];
  this.lastMetadata = null;
  this.lastSuggestions = null;

  this.seenDelimiter = false;
  this.conversationBuffer = '';
  this.metadataBuffer = '';
  this.partialDelimiter = '';

  this.chunkCount = 0;
  this.totalChunksReceived = 0;

  if (this.sendBtn) {
    this.sendBtn.disabled = false;
  }

  this.closeCitationPopup?.();
  this.allCitations = [];
  this.citationIndex.clear();
  this.citationToRowMap.clear();
  this.updateCitationHeaderButton?.();

  this.clearReasoningBuffer();
  this.updateDebugPanel();

  this.messages = [];

  if (this.input) {
    this.input.value = '';
    this.autoResizeInput();
  }

  if (this.conversation) {
    const existingContent = this.conversation.querySelector('.crimson-message, .crimson-typing-indicator');
    if (existingContent) {
      this.conversation.classList.add('crimson-conversation-fading');
      setTimeout(() => {
        this.conversation.classList.remove('crimson-conversation-fading');
        this.showWelcomeMessage();
      }, 350);
    } else {
      this.showWelcomeMessage();
    }
  }

  this.updateStatus('ready', 'Ready');

  log(Subsystems.CRIMSON, Status.INFO, 'Conversation reset');
};
