/**
 * Crimson AI Agent Manager
 *
 * A popup chat window for the AI Agent "Crimson".
 * Features:
 * - Draggable header
 * - Resizable (bottom-right corner handle)
 * - Centered initially at 70% viewport
 * - Chat interface with conversation pane and input
 * - ESC to close, Ctrl+Shift+C to open
 * - WebSocket streaming for real-time AI responses
 *
 * @module managers/crimson
 */

import { processIcons } from '../../core/icons.js';
import { log, getRawLog, Subsystems, Status } from '../../core/log.js';
import { formatLogText } from '../../shared/log-formatter.js';
import { getCrimsonWS } from '../../shared/crimson-ws.js';
import { getAppWS, isAppWSConnected } from '../../shared/app-ws.js';
import { getTip, initTooltips } from '../../core/tooltip-api.js';
import { registerShortcut } from '../../core/manager-ui.js';
import './crimson.css';

// Singleton instance tracking
let crimsonInstance = null;

// Global keyboard shortcut handler
let globalKeyHandler = null;

// LocalStorage keys for persistent state
const STORAGE_KEYS = {
  DEBUG_MODE: 'crimson_debug_mode',
  STREAMING_ENABLED: 'crimson_streaming_enabled',
  REASONING_MODE: 'crimson_reasoning_mode',
};

/**
 * Get the singleton Crimson instance (creates if needed)
 * @returns {CrimsonManager} The Crimson manager instance
 */
export function getCrimson() {
  if (!crimsonInstance) {
    crimsonInstance = new CrimsonManager();
  }
  return crimsonInstance;
}

/**
 * Show/activate the Crimson popup (convenience function)
 * @param {Object} options - Options for showing Crimson
 * @param {string} options.username - Username to personalize greeting
 */
export function showCrimson(options = {}) {
  const crimson = getCrimson();
  crimson.show(options);
}

/**
 * Hide/deactivate the Crimson popup (convenience function)
 */
export function hideCrimson() {
  if (crimsonInstance) {
    crimsonInstance.hide();
  }
}

/**
 * Toggle the Crimson popup (convenience function)
 * @param {Object} options - Options for showing Crimson if opening
 */
export function toggleCrimson(options = {}) {
  const crimson = getCrimson();
  if (crimson.isVisible) {
    crimson.hide();
  } else {
    crimson.show(options);
  }
}

/**
 * Create a Crimson button for manager slot headers
 * @param {string} [tooltip] - Custom tooltip text
 * @returns {HTMLButtonElement} The configured button element
 */
export function createCrimsonButton(tooltip = 'Chat with Crimson') {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close crimson-btn';
  button.setAttribute('data-tooltip', tooltip);
  button.setAttribute('data-shortcut-id', 'crimson');
  button.innerHTML = '<i class="fa-kit-duotone fa-crimson"></i>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    toggleCrimson();
  });

  // Process icon after adding to DOM
  requestAnimationFrame(() => {
    processIcons(button);
  });

  return button;
}

/**
 * Initialize global keyboard shortcut (Ctrl+Shift+C to toggle Crimson)
 */
export function initCrimsonShortcut() {
  if (globalKeyHandler) return; // Already initialized

  // Register with the shortcut system so tooltips get the key combo
  registerShortcut('crimson', 'Ctrl+Shift+C', 'Toggle Crimson', () => toggleCrimson());

  globalKeyHandler = (e) => {
    // Ctrl+Shift+C to toggle Crimson
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'C') {
      // Don't trigger if user is typing in an input/textarea
      const activeElement = document.activeElement;
      const isTyping = activeElement && (
        activeElement.tagName === 'INPUT' ||
        activeElement.tagName === 'TEXTAREA' ||
        activeElement.isContentEditable
      );

      if (!isTyping) {
        e.preventDefault();
        toggleCrimson();
      }
    }
  };

  document.addEventListener('keydown', globalKeyHandler);
  log(Subsystems.CRIMSON, Status.INFO, 'Global keyboard shortcut initialized (Ctrl+Shift+C)');
}

/**
 * Crimson AI Agent Manager Class
 */
class CrimsonManager {
  constructor() {
    this.popup = null;
    this.overlay = null;
    this.conversation = null;
    this.input = null;
    this.sendBtn = null;
    this.isVisible = false;
    this.isDragging = false;
    this.isResizing = false;
    this.dragStartX = 0;
    this.dragStartY = 0;
    this.popupStartX = 0;
    this.popupStartY = 0;
    this.resizeStartX = 0;
    this.resizeStartY = 0;
    this.popupStartWidth = 0;
    this.popupStartHeight = 0;
    this.popupStartLeft = 0;
    this.popupStartTop = 0;
    this.resizeCorner = 'br'; // Default to bottom-right
    this.username = 'User';
    this.messages = [];

    // WebSocket chat state
    this.wsClient = null;
    this.isStreaming = false;
    this.currentStreamElement = null;
    this.conversationHistory = [];
    this.lastMetadata = null;
    this.lastSuggestions = null;

    // Streaming delimiter handling
    this.DELIMITER = '[LITHIUM-CRIMSON-JSON]';
    this.seenDelimiter = false;
    this.conversationBuffer = '';
    this.metadataBuffer = '';
    this.partialDelimiter = '';

    // Debug state
    this.debugMode = false;

    // Streaming toggle state
    this.streamingEnabled = true;

    // Reasoning state
    this.reasoningMode = false;
    this.reasoningChunks = [];
    this.currentReasoningBuffer = '';

    // Connection status
    this.connectionState = 'disconnected';

    // Chunk counter for logging (log every 10th chunk with numbering)
    this.chunkCount = 0;
    this.totalChunksReceived = 0;

    // Temporarily enable showing thinking messages for debugging
    this.showThinkingMessages = true;

    // Citations state
    this.citations = new Map(); // messageId -> citation[]
    this.activeCitationPopup = null;
    this.activeCitationButton = null;

    // Bind methods
    this.handleDragStart = this.handleDragStart.bind(this);
    this.handleDragMove = this.handleDragMove.bind(this);
    this.handleDragEnd = this.handleDragEnd.bind(this);
    this.handleResizeStart = this.handleResizeStart.bind(this);
    this.handleResizeMove = this.handleResizeMove.bind(this);
    this.handleResizeEnd = this.handleResizeEnd.bind(this);
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleSend = this.handleSend.bind(this);
    this.handleInputKeydown = this.handleInputKeydown.bind(this);
    this.handleOverlayClick = this.handleOverlayClick.bind(this);
    this.handleDebugResizeStart = this.handleDebugResizeStart.bind(this);
    this.handleDebugResizeMove = this.handleDebugResizeMove.bind(this);
    this.handleDebugResizeEnd = this.handleDebugResizeEnd.bind(this);
    this.handleReasoningResizeStart = this.handleReasoningResizeStart.bind(this);
    this.handleReasoningResizeMove = this.handleReasoningResizeMove.bind(this);
    this.handleReasoningResizeEnd = this.handleReasoningResizeEnd.bind(this);

    // Initialize
    this.init();
  }

  /**
   * Load persistent state from localStorage
   */
  loadPersistentState() {
    try {
      const debugMode = localStorage.getItem(STORAGE_KEYS.DEBUG_MODE);
      if (debugMode !== null) {
        this.debugMode = debugMode === 'true';
      }
      
      const streamingEnabled = localStorage.getItem(STORAGE_KEYS.STREAMING_ENABLED);
      if (streamingEnabled !== null) {
        this.streamingEnabled = streamingEnabled === 'true';
      }
      
      const reasoningMode = localStorage.getItem(STORAGE_KEYS.REASONING_MODE);
      if (reasoningMode !== null) {
        this.reasoningMode = reasoningMode === 'true';
      }
    } catch (e) {
      // localStorage may not be available
    }
  }

  /**
   * Save persistent state to localStorage
   */
  savePersistentState() {
    try {
      localStorage.setItem(STORAGE_KEYS.DEBUG_MODE, String(this.debugMode));
      localStorage.setItem(STORAGE_KEYS.STREAMING_ENABLED, String(this.streamingEnabled));
      localStorage.setItem(STORAGE_KEYS.REASONING_MODE, String(this.reasoningMode));
    } catch (e) {
      // localStorage may not be available
    }
  }

  /**
   * Initialize the Crimson popup (create DOM elements)
   */
  init() {
    if (this.popup) return; // Already initialized

    // Load persistent state
    this.loadPersistentState();

    // Create overlay
    this.overlay = document.createElement('div');
    this.overlay.className = 'crimson-overlay';
    this.overlay.addEventListener('click', this.handleOverlayClick);
    document.body.appendChild(this.overlay);

    // Create popup
    this.popup = document.createElement('div');
    this.popup.className = 'crimson-popup';
    this.popup.innerHTML = `
      <div class="crimson-resize-handle crimson-resize-handle-tl" data-tooltip="Resize"></div>
      <div class="crimson-resize-handle crimson-resize-handle-tr" data-tooltip="Resize"></div>
      <div class="crimson-header">
        <div class="subpanel-header-group">
          <button type="button" class="crimson-header-primary">
            <i class="fa-kit-duotone fa-crimson"></i>
            <span>Chat with Crimson</span>
          </button>
          <button type="button" class="crimson-status-placeholder" disabled>
            <span class="crimson-status-indicator crimson-status-ready"></span>
            <span class="crimson-status-text">Ready</span>
          </button>
          <button type="button" class="crimson-streaming-btn" data-tooltip="Toggle streaming">
            <fa fa-water></fa>
          </button>
          <button type="button" class="crimson-reasoning-btn" data-tooltip="Toggle reasoning display">
            <fa fa-person></fa>
          </button>
          <button type="button" class="crimson-debug-btn" data-tooltip="Toggle debug view">
            <fa fa-bug></fa>
          </button>
          <button type="button" class="crimson-reset-btn" data-tooltip="Reset conversation">
            <fa fa-broom></fa>
          </button>
          <button type="button" class="crimson-header-close" data-tooltip="Close (ESC)">
            <fa fa-xmark></fa>
          </button>
        </div>
      </div>
      <div class="crimson-reasoning-panel">
        <div class="crimson-reasoning-content"></div>
        <div class="crimson-reasoning-splitter" data-tooltip="Drag to resize"></div>
      </div>
      <div class="crimson-conversation">
        <div class="crimson-welcome">
          <div class="crimson-welcome-icon">
            <i class="fa-kit-duotone fa-crimson"></i>
          </div>
          <div class="crimson-welcome-text">Hello, <span class="crimson-username">User</span>!</div>
          <div class="crimson-welcome-hint">How can I help you today?</div>
        </div>
      </div>
      <div class="crimson-debug-panel">
        <div class="crimson-debug-splitter" data-tooltip="Drag to resize"></div>
        <pre class="crimson-debug-content"></pre>
      </div>
      <div class="crimson-input-area">
        <textarea class="crimson-input" placeholder="Type your message..." rows="1"></textarea>
        <button type="button" class="crimson-send-btn" data-tooltip="Send message">
          <fa fa-up></fa>
        </button>
      </div>
      <div class="crimson-resize-handle crimson-resize-handle-bl" data-tooltip="Resize"></div>
      <div class="crimson-resize-handle crimson-resize-handle-br" data-tooltip="Resize"></div>
    `;

    document.body.appendChild(this.popup);
    initTooltips(this.popup);

    // Cache element references
    this.conversation = this.popup.querySelector('.crimson-conversation');
    this.input = this.popup.querySelector('.crimson-input');
    this.sendBtn = this.popup.querySelector('.crimson-send-btn');
    this.statusPlaceholder = this.popup.querySelector('.crimson-status-placeholder');
    this.statusIndicator = this.popup.querySelector('.crimson-status-indicator');
    this.statusText = this.popup.querySelector('.crimson-status-text');
    this.debugPanel = this.popup.querySelector('.crimson-debug-panel');
    this.debugContent = this.popup.querySelector('.crimson-debug-content');
    this.reasoningPanel = this.popup.querySelector('.crimson-reasoning-panel');
    this.reasoningContent = this.popup.querySelector('.crimson-reasoning-content');
    this.streamingBtn = this.popup.querySelector('.crimson-streaming-btn');
    this.reasoningBtn = this.popup.querySelector('.crimson-reasoning-btn');
    this.debugBtn = this.popup.querySelector('.crimson-debug-btn');
    const header = this.popup.querySelector('.crimson-header');
    const closeBtn = this.popup.querySelector('.crimson-header-close');
    const resetBtn = this.popup.querySelector('.crimson-reset-btn');
    const resizeHandleBR = this.popup.querySelector('.crimson-resize-handle-br');
    const resizeHandleBL = this.popup.querySelector('.crimson-resize-handle-bl');
    const resizeHandleTR = this.popup.querySelector('.crimson-resize-handle-tr');
    const resizeHandleTL = this.popup.querySelector('.crimson-resize-handle-tl');
    const debugSplitter = this.popup.querySelector('.crimson-debug-splitter');
    const reasoningSplitter = this.popup.querySelector('.crimson-reasoning-splitter');

    // Set up event listeners
    header.addEventListener('mousedown', this.handleDragStart);
    closeBtn.addEventListener('click', () => this.hide());

    // Wire citation link clicks (delegated)
    this.conversation.addEventListener('click', (e) => this.handleCitationLinkClick(e));
    resetBtn?.addEventListener('click', () => this.resetConversation());
    this.debugBtn?.addEventListener('click', () => this.toggleDebugPanel());
    this.streamingBtn?.addEventListener('click', () => this.toggleStreaming());
    this.reasoningBtn?.addEventListener('click', () => this.toggleReasoningPanel());
    this.sendBtn.addEventListener('click', this.handleSend);
    this.input.addEventListener('keydown', this.handleInputKeydown);
    this.input.addEventListener('input', () => this.autoResizeInput());
    
    // Resize handles for all four corners
    resizeHandleBR?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'br'));
    resizeHandleBL?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'bl'));
    resizeHandleTR?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'tr'));
    resizeHandleTL?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'tl'));

    // Debug and reasoning panel splitters
    debugSplitter?.addEventListener('mousedown', (e) => this.handleDebugResizeStart(e));
    reasoningSplitter?.addEventListener('mousedown', (e) => this.handleReasoningResizeStart(e));

    // Apply persistent state to UI
    this.applyPersistentState();

    // Process icons
    processIcons(this.popup);

    // Position popup initially (centered)
    this.centerPopup();

    log(Subsystems.CRIMSON, Status.INFO, 'Initialized');
  }

  /**
   * Apply persistent state to UI elements
   * Uses requestAnimationFrame to ensure initial render before animating panels
   */
  applyPersistentState() {
    // Apply streaming state (no animation needed)
    this.updateStreamingButton();

    // Apply debug button state
    this.debugBtn?.classList.toggle('active', this.debugMode);
    this.updateDebugPanel();

    // Apply reasoning button state
    this.reasoningBtn?.classList.toggle('active', this.reasoningMode);

    // Defer panel visibility to allow initial render, then animate
    requestAnimationFrame(() => {
      this.popup.classList.toggle('crimson-debug-visible', this.debugMode);
      this.popup.classList.toggle('crimson-reasoning-visible', this.reasoningMode);
    });
  }

  /**
   * Center the popup on screen at 70% viewport size
   */
  centerPopup() {
    if (!this.popup) return;

    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    const width = Math.min(viewportWidth * 0.7, 1200); // Max 1200px
    const height = Math.min(viewportHeight * 0.7, 800); // Max 800px

    this.popup.style.width = `${width}px`;
    this.popup.style.height = `${height}px`;
    this.popup.style.left = `${(viewportWidth - width) / 2}px`;
    this.popup.style.top = `${(viewportHeight - height) / 2}px`;
  }

  /**
   * Show the Crimson popup
   * @param {Object} options - Show options
   * @param {string} options.username - Username for greeting
   */
  show(options = {}) {
    const isFirstShow = !this.popup;
    if (isFirstShow) this.init();

    // Update username if provided
    if (options.username) {
      this.username = options.username;
      const usernameEl = this.popup.querySelector('.crimson-username');
      if (usernameEl) {
        usernameEl.textContent = this.username;
      }
    }

    // If already visible, just focus the input
    if (this.isVisible) {
      this.input?.focus();
      return;
    }

    // On first show, force a reflow to ensure initial styles are applied
    // before adding the visible class. This ensures the animation runs.
    if (isFirstShow) {
      void this.overlay.offsetHeight;
      void this.popup.offsetHeight;
    }

    // Show overlay and popup
    this.overlay.classList.add('visible');
    this.popup.classList.add('visible');
    this.isVisible = true;

    // Add keyboard listener
    document.addEventListener('keydown', this.handleKeyDown);

    // Focus the input after animation
    setTimeout(() => {
      this.input?.focus();
    }, 100);

    log(Subsystems.CRIMSON, Status.INFO, 'Shown');
  }

  /**
   * Hide the Crimson popup
   */
  hide() {
    if (!this.isVisible) return;

    this.overlay.classList.remove('visible');
    this.popup.classList.remove('visible');
    this.isVisible = false;

    // Remove keyboard listener
    document.removeEventListener('keydown', this.handleKeyDown);

    // Blur any focused element
    if (document.activeElement && this.popup.contains(document.activeElement)) {
      document.activeElement.blur();
    }

    log(Subsystems.CRIMSON, Status.INFO, 'Hidden');
  }

  /**
   * Reset the conversation to empty state
   */
  resetConversation() {
    // Animate broom icon - trigger CSS animation on SVG
    const resetBtn = this.popup?.querySelector('.crimson-reset-btn');
    if (resetBtn) {
      resetBtn.classList.add('animating');
      setTimeout(() => {
        resetBtn.classList.remove('animating');
      }, 1500);
    }

    // Cleanup WebSocket client state (does NOT disconnect - managed by app-ws)
    if (this.wsClient) {
      this.wsClient.cleanup();
      this.wsClient = null;
    }

    // Reset streaming state
    this.isStreaming = false;
    this.currentStreamElement = null;
    this.conversationHistory = [];
    this.lastMetadata = null;
    this.lastSuggestions = null;

    // Reset delimiter buffers
    this.seenDelimiter = false;
    this.conversationBuffer = '';
    this.metadataBuffer = '';
    this.partialDelimiter = '';

    // Reset chunk counters
    this.chunkCount = 0;
    this.totalChunksReceived = 0;

    // Re-enable send button
    if (this.sendBtn) {
      this.sendBtn.disabled = false;
    }

    // Clear reasoning panel
    this.clearReasoningBuffer();

    // Refresh debug panel
    this.updateDebugPanel();

    // Clear message history
    this.messages = [];

    // Clear input
    if (this.input) {
      this.input.value = '';
      this.autoResizeInput();
    }

    // Fade out existing conversation content, then show welcome
    if (this.conversation) {
      const existingContent = this.conversation.querySelector('.crimson-message, .crimson-typing-indicator');
      if (existingContent) {
        this.conversation.classList.add('crimson-conversation-fading');
        setTimeout(() => {
          this.conversation.classList.remove('crimson-conversation-fading');
          this.showWelcomeMessage();
        }, 350); // Match transition duration
      } else {
        this.showWelcomeMessage();
      }
    }

    // Reset status
    this.updateStatus('ready', 'Ready');

    log(Subsystems.CRIMSON, Status.INFO, 'Conversation reset');
  }

  /**
   * Show the welcome message in the conversation area
   */
  showWelcomeMessage() {
    if (this.conversation) {
      this.conversation.innerHTML = `
        <div class="crimson-welcome crimson-welcome-fade-in">
          <div class="crimson-welcome-icon">
            <i class="fa-kit-duotone fa-crimson"></i>
          </div>
          <div class="crimson-welcome-text">Hello, <span class="crimson-username">${this.escapeHtml(this.username)}</span>!</div>
          <div class="crimson-welcome-hint">How can I help you today?</div>
        </div>
      `;
      processIcons(this.conversation);
    }
  }

  /**
   * Update the status display in the header placeholder
   * @param {string} state - Status state: 'ready', 'sending', 'thinking', 'reasoning', 'responding', 'error'
   * @param {string} text - Status text to display
   */
  updateStatus(state, text) {
    if (!this.statusIndicator || !this.statusText) return;

    this.connectionState = state;
    this.statusIndicator.className = `crimson-status-indicator crimson-status-${state}`;
    this.statusText.textContent = text;
  }

  /**
   * Toggle the debug panel visibility with animation
   */
  toggleDebugPanel() {
    if (!this.debugPanel) return;

    this.debugMode = !this.debugMode;
    this.popup.classList.toggle('crimson-debug-visible', this.debugMode);
    this.debugBtn?.classList.toggle('active', this.debugMode);
    this.savePersistentState();
    this.updateDebugPanel();
  }

  /**
   * Toggle streaming mode
   */
  toggleStreaming() {
    this.streamingEnabled = !this.streamingEnabled;
    this.updateStreamingButton();
    this.addDebugMessage('CONFIG', `Streaming ${this.streamingEnabled ? 'enabled' : 'disabled'}`);
    this.savePersistentState();
  }

  /**
   * Update streaming button icon
   */
  updateStreamingButton() {
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
  }

  /**
   * Toggle reasoning panel visibility with slide animation
   */
  toggleReasoningPanel() {
    if (!this.reasoningPanel) return;

    this.reasoningMode = !this.reasoningMode;
    this.popup.classList.toggle('crimson-reasoning-visible', this.reasoningMode);
    this.reasoningBtn?.classList.toggle('active', this.reasoningMode);
    this.updateReasoningButton();
    
    // If turning on and we have accumulated reasoning data, display it
    if (this.reasoningMode && this.reasoningContent && this.currentReasoningBuffer) {
      this.reasoningContent.textContent = this.currentReasoningBuffer;
      this.reasoningContent.scrollTop = this.reasoningContent.scrollHeight;
    }
    
    this.savePersistentState();
  }

  /**
   * Update reasoning button icon
   */
  updateReasoningButton() {
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
  }

  /**
   * Add debug message to the session log
   * @param {string} type - Message type
   * @param {string} content - Message content
   */
  addDebugMessage(type, content) {
    // Log to session log with Crimson subsystem
    log(Subsystems.CRIMSON, Status.DEBUG, `[${type}] ${content}`);

    // Only update display if debug mode is active
    if (this.debugMode) {
      this.updateDebugPanel();
    }
  }

  /**
   * Update the debug panel content with session log format
   * Shows all Crimson subsystem entries from the session log
   */
  updateDebugPanel() {
    if (!this.debugContent) return;
    
    // Get all log entries and filter for Crimson subsystem
    const allEntries = getRawLog();
    const crimsonEntries = allEntries.filter(e => 
      e.subsystem === 'Crimson' || 
      (e.subsystem && e.subsystem.startsWith('[EventBus]') && e.description && e.description.includes('Crimson'))
    );
    
    // Format using the session log formatter
    const logText = formatLogText(crimsonEntries);
    this.debugContent.textContent = logText;
    this.debugContent.scrollTop = this.debugContent.scrollHeight;
  }

  /**
   * Add reasoning content to the reasoning panel
   * @param {string} content - Reasoning content from reasoning_content field
   */
  addReasoningContent(content) {
    if (!content) return;

    this.currentReasoningBuffer += content;

    // Always collect reasoning data
    this.reasoningChunks.push(content);

    // Only update display if reasoning mode is active
    if (this.reasoningMode && this.reasoningContent) {
      this.reasoningContent.textContent = this.currentReasoningBuffer;
      this.reasoningContent.scrollTop = this.reasoningContent.scrollHeight;
    }
  }

  /**
   * Clear reasoning buffer for new response
   */
  clearReasoningBuffer() {
    this.currentReasoningBuffer = '';
    this.reasoningChunks = [];
    if (this.reasoningContent) {
      this.reasoningContent.textContent = '';
    }
  }

  /**
   * Handle keyboard events (ESC to close)
   */
  handleKeyDown(e) {
    if (e.key === 'Escape') {
      e.preventDefault();
      // Close citation popup first if open, otherwise close Crimson
      if (this.activeCitationPopup) {
        this.closeCitationPopup();
      } else {
        this.hide();
      }
    }
  }

  /**
   * Handle overlay click (click outside to close)
   */
  handleOverlayClick(e) {
    if (e.target === this.overlay) {
      this.hide();
    }
  }

  /**
   * Handle drag start on header
   */
  handleDragStart(e) {
    // Don't drag if clicking the close button
    if (e.target.closest('.crimson-header-close')) return;

    this.isDragging = true;
    this.popup.classList.add('dragging');

    this.dragStartX = e.clientX;
    this.dragStartY = e.clientY;

    const rect = this.popup.getBoundingClientRect();
    this.popupStartX = rect.left;
    this.popupStartY = rect.top;

    document.addEventListener('mousemove', this.handleDragMove);
    document.addEventListener('mouseup', this.handleDragEnd);

    e.preventDefault();
  }

  /**
   * Handle drag move
   */
  handleDragMove(e) {
    if (!this.isDragging) return;

    const deltaX = e.clientX - this.dragStartX;
    const deltaY = e.clientY - this.dragStartY;

    let newX = this.popupStartX + deltaX;
    let newY = this.popupStartY + deltaY;

    // Constrain to viewport
    const rect = this.popup.getBoundingClientRect();
    const minVisible = 50; // Minimum visible pixels

    newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
    newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

    this.popup.style.left = `${newX}px`;
    this.popup.style.top = `${newY}px`;
  }

  /**
   * Handle drag end
   */
  handleDragEnd() {
    this.isDragging = false;
    this.popup.classList.remove('dragging');

    document.removeEventListener('mousemove', this.handleDragMove);
    document.removeEventListener('mouseup', this.handleDragEnd);
  }

  /**
   * Handle resize start
   * @param {MouseEvent} e - Mouse event
   * @param {string} corner - Which corner is being resized: 'br', 'bl', 'tr', 'tl'
   */
  handleResizeStart(e, corner = 'br') {
    this.isResizing = true;
    this.resizeCorner = corner;
    this.popup.classList.add('resizing');

    this.resizeStartX = e.clientX;
    this.resizeStartY = e.clientY;

    const rect = this.popup.getBoundingClientRect();
    this.popupStartWidth = rect.width;
    this.popupStartHeight = rect.height;
    this.popupStartLeft = rect.left;
    this.popupStartTop = rect.top;

    document.addEventListener('mousemove', this.handleResizeMove);
    document.addEventListener('mouseup', this.handleResizeEnd);

    e.preventDefault();
    e.stopPropagation();
  }

  /**
   * Handle resize move
   */
  handleResizeMove(e) {
    if (!this.isResizing) return;

    const deltaX = e.clientX - this.resizeStartX;
    const deltaY = e.clientY - this.resizeStartY;

    let newWidth = this.popupStartWidth;
    let newHeight = this.popupStartHeight;
    let newLeft = this.popupStartLeft;
    let newTop = this.popupStartTop;

    // Calculate new dimensions based on which corner is being dragged
    switch (this.resizeCorner) {
      case 'br': // Bottom-right
        newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth + deltaX));
        newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight + deltaY));
        break;
      case 'bl': // Bottom-left
        newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth - deltaX));
        newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight + deltaY));
        if (newWidth !== this.popupStartWidth - deltaX) {
          // Clamp occurred, adjust left position accordingly
          const actualDeltaX = this.popupStartWidth - newWidth;
          newLeft = this.popupStartLeft + actualDeltaX;
        } else {
          newLeft = this.popupStartLeft + deltaX;
        }
        break;
      case 'tr': // Top-right
        newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth + deltaX));
        newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight - deltaY));
        if (newHeight !== this.popupStartHeight - deltaY) {
          // Clamp occurred, adjust top position accordingly
          const actualDeltaY = this.popupStartHeight - newHeight;
          newTop = this.popupStartTop + actualDeltaY;
        } else {
          newTop = this.popupStartTop + deltaY;
        }
        break;
      case 'tl': // Top-left
        newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth - deltaX));
        newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight - deltaY));
        if (newWidth !== this.popupStartWidth - deltaX) {
          const actualDeltaX = this.popupStartWidth - newWidth;
          newLeft = this.popupStartLeft + actualDeltaX;
        } else {
          newLeft = this.popupStartLeft + deltaX;
        }
        if (newHeight !== this.popupStartHeight - deltaY) {
          const actualDeltaY = this.popupStartHeight - newHeight;
          newTop = this.popupStartTop + actualDeltaY;
        } else {
          newTop = this.popupStartTop + deltaY;
        }
        break;
    }

    // Apply new dimensions and position
    this.popup.style.width = `${newWidth}px`;
    this.popup.style.height = `${newHeight}px`;
    
    // Only update position for corners that affect it
    if (this.resizeCorner === 'bl' || this.resizeCorner === 'tl') {
      this.popup.style.left = `${newLeft}px`;
    }
    if (this.resizeCorner === 'tr' || this.resizeCorner === 'tl') {
      this.popup.style.top = `${newTop}px`;
    }
  }

  /**
   * Handle resize end
   */
  handleResizeEnd() {
    this.isResizing = false;
    this.popup.classList.remove('resizing');

    document.removeEventListener('mousemove', this.handleResizeMove);
    document.removeEventListener('mouseup', this.handleResizeEnd);
  }

  /**
   * Handle debug panel resize start
   */
  handleDebugResizeStart(e) {
    this.isResizingDebug = true;
    this.debugResizeStartY = e.clientY;
    this.debugStartHeight = this.debugPanel?.offsetHeight || 150;
    
    // Disable transition during resize for instant response
    this.debugPanel?.classList.add('no-transition');

    document.addEventListener('mousemove', this.handleDebugResizeMove);
    document.addEventListener('mouseup', this.handleDebugResizeEnd);
    e.preventDefault();
    e.stopPropagation();
  }

  /**
   * Handle debug panel resize move
   */
  handleDebugResizeMove(e) {
    if (!this.isResizingDebug || !this.debugPanel) return;

    const deltaY = e.clientY - this.debugResizeStartY;
    const newHeight = Math.max(50, Math.min(400, this.debugStartHeight - deltaY));
    this.debugPanel.style.flex = `0 0 ${newHeight}px`;
  }

  /**
   * Handle debug panel resize end
   */
  handleDebugResizeEnd() {
    this.isResizingDebug = false;
    // Re-enable transition after resize
    this.debugPanel?.classList.remove('no-transition');
    document.removeEventListener('mousemove', this.handleDebugResizeMove);
    document.removeEventListener('mouseup', this.handleDebugResizeEnd);
  }

  /**
   * Handle reasoning panel resize start
   */
  handleReasoningResizeStart(e) {
    this.isResizingReasoning = true;
    this.reasoningResizeStartY = e.clientY;
    this.reasoningStartHeight = this.reasoningPanel?.offsetHeight || 150;
    
    // Disable transition during resize for instant response
    this.reasoningPanel?.classList.add('no-transition');

    document.addEventListener('mousemove', this.handleReasoningResizeMove);
    document.addEventListener('mouseup', this.handleReasoningResizeEnd);
    e.preventDefault();
    e.stopPropagation();
  }

  /**
   * Handle reasoning panel resize move
   * Splitter is at the bottom, so dragging down increases height
   */
  handleReasoningResizeMove(e) {
    if (!this.isResizingReasoning || !this.reasoningPanel) return;

    const deltaY = e.clientY - this.reasoningResizeStartY;
    // Splitter at bottom: dragging down (+deltaY) should increase height
    const newHeight = Math.max(50, Math.min(400, this.reasoningStartHeight + deltaY));
    this.reasoningPanel.style.flex = `0 0 ${newHeight}px`;
  }

  /**
   * Handle reasoning panel resize end
   */
  handleReasoningResizeEnd() {
    this.isResizingReasoning = false;
    // Re-enable transition after resize
    this.reasoningPanel?.classList.remove('no-transition');
    document.removeEventListener('mousemove', this.handleReasoningResizeMove);
    document.removeEventListener('mouseup', this.handleReasoningResizeEnd);
  }

  /**
   * Handle send button click
   */
  handleSend() {
    const message = this.input.value.trim();
    if (!message || this.isStreaming) return;

    // Add user message to display
    this.addMessage('user', message);

    // Clear input
    this.input.value = '';
    this.autoResizeInput();

    // Send message via WebSocket
    // Note: user message is added to conversationHistory in sendChatMessage after successful send
    this.sendChatMessage(message);
  }

  /**
   * Handle input keydown (Enter to send, Shift+Enter for newline)
   */
  handleInputKeydown(e) {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      this.handleSend();
    }
  }

  /**
   * Auto-resize input based on content
   */
  autoResizeInput() {
    this.input.style.height = 'auto';
    const newHeight = Math.min(150, Math.max(35, this.input.scrollHeight));
    this.input.style.height = `${newHeight}px`;
  }

  /**
   * Add a message to the conversation
   * @param {string} sender - 'user' or 'agent'
   * @param {string} text - Message text
   * @returns {string} The message element ID
   */
  addMessage(sender, text) {
    // Fade out and remove welcome message if present
    const welcome = this.conversation.querySelector('.crimson-welcome');
    if (welcome) {
      welcome.classList.add('crimson-welcome-fading');
      setTimeout(() => welcome.remove(), 350);
    }

    // Generate unique message ID
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

    // Scroll to bottom
    this.conversation.scrollTop = this.conversation.scrollHeight;

    // Store message
    this.messages.push({ sender, text, timestamp: Date.now() });

    return messageId;
  }

  /**
   * Add typing indicator
   */
  addTypingIndicator() {
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
  }

  /**
   * Remove typing indicator
   */
  removeTypingIndicator() {
    const indicator = this.conversation.querySelector('.crimson-typing-indicator');
    if (indicator) {
      indicator.remove();
    }
  }

  /**
   * Initialize WebSocket client
   * Note: Connection lifecycle is managed by app-ws.js, not by Crimson
   */
  initWebSocketClient() {
    // Create or update the Crimson WS client (only handles messages, not connection)
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
  }

  /**
   * Send chat message via WebSocket
   * Uses the persistent app-ws.js connection - does NOT manage connection lifecycle
   * @param {string} message - User message
   */
  sendChatMessage(message) {
    // Initialize WebSocket client if needed (only sets up callbacks, doesn't connect)
    this.initWebSocketClient();

    // Check WebSocket state
    if (!isAppWSConnected()) {
      this.addDebugMessage('ERROR', 'WebSocket not connected');
      return;
    }

    // If already streaming, cancel the previous request
    if (this.isStreaming && this.wsClient) {
      this.addDebugMessage('CANCEL', 'Cancelling previous request');
      this.wsClient.abortCurrentRequest();
      
      // Remove the old streaming element
      if (this.currentStreamElement) {
        this.currentStreamElement.remove();
        this.currentStreamElement = null;
      }
      
      // Reset streaming state
      this.seenDelimiter = false;
      this.conversationBuffer = '';
      this.metadataBuffer = '';
      this.partialDelimiter = '';
    }

    this.isStreaming = true;
    this.chunkCount = 0;  // Reset chunk counter
    this.totalChunksReceived = 0;  // Reset total chunk counter

    // Update status to sending
    this.updateStatus('sending', 'Sending...');

    // Add debug message
    this.addDebugMessage('SEND', message);

    // Add streaming indicator with loading spinner
    this.currentStreamElement = this.addStreamingMessage();

    // Clear reasoning buffer for new response
    this.clearReasoningBuffer();

    // Show loading animation on the streaming message while waiting
    if (this.currentStreamElement) {
      this.currentStreamElement.classList.add('crimson-waiting');
    }

    // Use streamingEnabled state to determine if we should stream
    // Note: current message is NOT in history - crimson-ws.js adds it at the end
    const history = this.conversationHistory.slice(-10); // Last 10 messages for context
    this.addDebugMessage('SEND', `Sending with ${history.length} history messages, stream=${this.streamingEnabled}`);
    
    // Log prompt sent to event bus
    log(Subsystems.EVENTBUS, Status.INFO, `Crimson: Prompt sent (${message.length} chars)`);
    
    // Update status to thinking immediately after initiating send
    // The send call is synchronous over WebSocket, the Promise just waits for response
    this.updateStatus('thinking', 'Thinking...');
    
    // Fire-and-forget: send returns a Promise that resolves when done, but we handle
    // results via callbacks (onChunk, onDone, onError) registered in initWebSocketClient
    this.wsClient.send(message, {
      history,
      stream: this.streamingEnabled,
    }).catch((error) => {
      // Check if this was a cancelled request
      if (error.message.includes('abort') || error.message.includes('cancel')) {
        this.addDebugMessage('CANCELLED', 'Request was cancelled');
        return; // Don't show error for cancelled requests
      }
      
      // Check if connection wasn't ready
      if (error.message.includes('not connected')) {
        this.addDebugMessage('ERROR', 'WebSocket not connected - please wait for connection');
        this.updateStatus('error', 'Connection not ready');
        this.input.focus();
        return;
      }
      
      log(Subsystems.CRIMSON, Status.ERROR, `Chat error: ${error.message}`);
      this.handleStreamError(error.message);
    });
  }

  /**
   * Handle incoming stream chunk - accumulates content and displays in real-time
   */
  handleStreamChunk(content, _index, finishReason, reasoningContent) {
    // Handle reasoning content first (if present)
    if (reasoningContent) {
      // Update status to reasoning when we first receive reasoning content
      if (this.connectionState === 'thinking') {
        this.updateStatus('reasoning', 'Reasoning...');
      }
      this.addReasoningContent(reasoningContent);
    }

    if (!content && !finishReason) return;

    // Remove waiting animation and show content ONLY when we have actual response content
    if (this.currentStreamElement && this.currentStreamElement.classList.contains('crimson-waiting')) {
      this.currentStreamElement.classList.remove('crimson-waiting');
      // Hide waiting spinner and show content
      const waitingEl = this.currentStreamElement.querySelector('.crimson-message-waiting');
      const contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
      if (waitingEl) waitingEl.classList.add('crimson-hidden');
      if (contentEl) contentEl.classList.remove('crimson-hidden');
    }
    
    // Get content element
    let contentEl = null;
    if (this.currentStreamElement) {
      contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
    }
    
    if (!contentEl) {
      return;
    }

    // Update total chunk counter
    this.totalChunksReceived++;
    
    // Log to debug panel every 100 chunks only
    if (this.totalChunksReceived % 100 === 0 || finishReason) {
      this.addDebugMessage('CHUNK', `#${this.totalChunksReceived}${finishReason ? ` (${finishReason})` : ''}`);
    }

    // Update status on first content (AI is now responding)
    if (this.connectionState !== 'responding') {
      this.updateStatus('responding', 'Responding...');
    }

    // Check for delimiter to separate content from metadata
    if (this.seenDelimiter) {
      this.metadataBuffer += content;
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
        // Check for partial delimiter at end
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

      // Update display with current content (add cursor during streaming)
      const displayContent = this.conversationBuffer.replace(/\s+$/, '');
      contentEl.setAttribute('data-raw-content', displayContent);
      contentEl.innerHTML = this.formatMessageContent(displayContent) + '<span class="crimson-streaming-cursor"></span>';

      // Scroll to show new content
      if (this.conversation) {
        this.conversation.scrollTop = this.conversation.scrollHeight;
      }
    }

    // Handle stream completion
    if (finishReason) {
      log(Subsystems.CRIMSON, Status.DEBUG, `Stream complete (${finishReason})`);
      this.addDebugMessage('COMPLETE', `Stream finished with reason: ${finishReason}`);
      this.handleStreamFinished();
    }
  }

  /**
   * Handle stream completion after all chunks received
   * This is called when finish_reason arrives in the last chunk
   */
  handleStreamFinished() {
    // Skip if already completed (prevent duplicate handleStreamDone calls)
    if (!this.isStreaming) {
      return;
    }

    try {
      // If there's a pending done result, process it now
      if (this.wsClient && this.wsClient.pendingDoneResult) {
        const { id, result } = this.wsClient.pendingDoneResult;
        this.wsClient.pendingDoneResult = null;
        
        // Call the onDone callback to update UI
        this.handleStreamDone(result?.content, result);
      } else {
        // Finalize with current buffers (no done message received yet)
        this.handleStreamDone(this.conversationBuffer, null);
      }
    } catch (error) {
      log(Subsystems.CRIMSON, Status.ERROR, `Error in handleStreamFinished: ${error.message}`);
    }
    
    // Always try to resolve any pending Promise and clear state
    if (this.wsClient) {
      const id = this.wsClient.currentRequestId;
      
      if (id && this.wsClient.pendingRequests && this.wsClient.pendingRequests.has(id)) {
        const callbacks = this.wsClient.pendingRequests.get(id);
        if (callbacks && callbacks.onDone) {
          callbacks.onDone({ content: this.conversationBuffer });
          this.wsClient.pendingRequests.delete(id);
        }
      }
      this.wsClient.currentRequestId = null;
    }
    
    log(Subsystems.CRIMSON, Status.DEBUG, 'handleStreamFinished complete');
  }



  /**
   * Handle stream completion
   * @param {string} content - Final complete content (may be from pending done or our buffer)
   * @param {Object} result - Full result object (may contain followUpQuestions, etc.)
   */
  handleStreamDone(content, result) {
    log(Subsystems.CRIMSON, Status.DEBUG, `handleStreamDone called, isStreaming: ${this.isStreaming}, content length: ${content?.length || 0}`);

    // Log response received to event bus
    log(Subsystems.EVENTBUS, Status.INFO, `Crimson: Response received (${content?.length || 0} chars)`);

    try {
      this.addDebugMessage('DONE', 'Stream complete');

      // For non-streaming responses, content includes the delimiter and metadata
      // We need to parse it to separate conversation text from metadata
      let conversationText = content || '';
      let metadataFromContent = null;
      
      // Check if content contains delimiter (handles both streaming and non-streaming cases)
      // This can happen when the server sends the full response in chat_done
      if (content) {
        const delimiterIndex = content.indexOf(this.DELIMITER);
        if (delimiterIndex !== -1) {
          conversationText = content.substring(0, delimiterIndex).replace(/\s+$/, '');
          const metadataStr = content.substring(delimiterIndex + this.DELIMITER.length);
          try {
            metadataFromContent = JSON.parse(metadataStr);
            this.addDebugMessage('META', 'Parsed delimiter from content');
          } catch (e) {
            log(Subsystems.CRIMSON, Status.WARN, `Failed to parse metadata from content: ${e.message}`);
          }
        } else if (!this.seenDelimiter) {
          // No delimiter found, use all content as conversation
          conversationText = content.replace(/\s+$/, '');
        }
      }

      // If we were streaming and accumulated conversation buffer, use that instead
      if (this.isStreaming && this.conversationBuffer) {
        conversationText = this.conversationBuffer.replace(/\s+$/, '');
      }

      // Parse metadata JSON if we have it (from chunks) or from content
      let parsedMetadata = metadataFromContent;

      // First try to parse from metadata buffer (chunk-based metadata)
      if (!parsedMetadata && this.metadataBuffer && this.metadataBuffer.trim()) {
        try {
          parsedMetadata = JSON.parse(this.metadataBuffer.trim());
          this.addDebugMessage('META', 'From chunks: ' + JSON.stringify(parsedMetadata).substring(0, 200));
        } catch (e) {
          log(Subsystems.CRIMSON, Status.WARN, `Failed to parse metadata JSON: ${e.message}`);
        }
      }

      // If no metadata from chunks, try from result object (from done message)
      if (!parsedMetadata && result) {
        // Check raw_provider_response first (new transparent Hydrogen format)
        const rawResponse = result.raw_provider_response;
        this.addDebugMessage('META', `Raw provider response: ${JSON.stringify(rawResponse)?.substring(0, 300)}`);

        // Result might have followUpQuestions directly or nested in various structures
        parsedMetadata = {
          followUpQuestions: result.followUpQuestions || result.metadata?.followUpQuestions || rawResponse?.followUpQuestions || [],
          suggestions: result.suggestions || result.metadata?.suggestions || rawResponse?.suggestions || null,
          metadata: result.metadata || rawResponse?.metadata || null,
          citations: result.citations || result.metadata?.citations || rawResponse?.citations || rawResponse?.retrieval?.retrieved_data || null,
        };
        this.addDebugMessage('META', `From done message: ${JSON.stringify(parsedMetadata).substring(0, 300)}`);
      }

      // Generate unique message ID for this response
      const messageId = `crimson-msg-${Date.now()}`;

      if (this.currentStreamElement) {
        // Set message ID
        this.currentStreamElement.id = messageId;

        // Remove waiting animation and show content for non-streaming responses
        if (this.currentStreamElement.classList.contains('crimson-waiting')) {
          this.currentStreamElement.classList.remove('crimson-waiting');
          const waitingEl = this.currentStreamElement.querySelector('.crimson-message-waiting');
          const contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
          if (waitingEl) waitingEl.classList.add('crimson-hidden');
          if (contentEl) contentEl.classList.remove('crimson-hidden');
        }

        const contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
        if (contentEl) {
          // Set the final conversation content with full markdown rendering
          contentEl.setAttribute('data-raw-content', conversationText);

          // Use full markdown rendering for final content
          this.formatMessageContentMarkdown(conversationText).then(htmlContent => {
            // Convert citation markers to links
            htmlContent = this.convertCitationMarkers(htmlContent, messageId);
            contentEl.innerHTML = htmlContent;
            // Scroll to show new content
            if (this.conversation) {
              this.conversation.scrollTop = this.conversation.scrollHeight;
            }
          });

          // Parse and store citations from metadata
          this.addDebugMessage('META', `Citations check: ${JSON.stringify(parsedMetadata?.citations)}`);
          if (parsedMetadata && parsedMetadata.citations) {
            this.parseCitations(messageId, parsedMetadata.citations);
          }

          // Add follow-up questions from metadata if present
          if (parsedMetadata && parsedMetadata.followUpQuestions && parsedMetadata.followUpQuestions.length > 0) {
            this.addFollowUpQuestions(this.currentStreamElement, parsedMetadata.followUpQuestions);
          }

          // Store metadata for later use
          if (parsedMetadata && parsedMetadata.metadata) {
            this.lastMetadata = parsedMetadata.metadata;
          }

          // Handle suggestions (tours, navigation, etc.)
          if (parsedMetadata && parsedMetadata.suggestions) {
            this.handleSuggestions(parsedMetadata.suggestions);
          }
        }

        // Remove streaming indicator class
        this.currentStreamElement.classList.remove('crimson-streaming');
      }

      // Add to conversation history (conversation text only)
      if (conversationText) {
        this.conversationHistory.push({ role: 'assistant', content: conversationText });

        // Store message
        this.messages.push({ sender: 'agent', text: conversationText, timestamp: Date.now() });
      }
    } catch (error) {
      log(Subsystems.CRIMSON, Status.ERROR, `Error in handleStreamDone: ${error.message}`);
      this.addDebugMessage('ERROR', error.message);
    } finally {
      // Always reset streaming state
      this.currentStreamElement = null;
      this.isStreaming = false;
      this.seenDelimiter = false;
      this.conversationBuffer = '';
      this.metadataBuffer = '';
      this.partialDelimiter = '';
      this.input.focus();

      // Update status back to ready
      this.updateStatus('ready', 'Ready');

      // Scroll to bottom
      if (this.conversation) {
        this.conversation.scrollTop = this.conversation.scrollHeight;
      }
    }
  }

  /**
   * Parse Crimson's JSON response
   * @param {string} content - Raw response content
   * @returns {Object} Parsed response with displayMessage, followUpQuestions, suggestions, metadata, citations
   */
  parseCrimsonResponse(content) {
    const result = {
      displayMessage: content,
      followUpQuestions: [],
      suggestions: null,
      metadata: null,
      citations: null,
    };

    // Try to parse as JSON
    try {
      const json = JSON.parse(content);

      // Extract conversation message
      if (json.conversation && json.conversation.message) {
        result.displayMessage = json.conversation.message;
        result.followUpQuestions = json.conversation.followUpQuestions || [];
      }

      // Extract suggestions
      if (json.suggestions) {
        result.suggestions = json.suggestions;
      }

      // Extract metadata
      if (json.metadata) {
        result.metadata = json.metadata;
      }

      // Extract citations (can be at top level or in metadata)
      if (json.citations) {
        result.citations = json.citations;
      } else if (json.metadata?.citations) {
        result.citations = json.metadata.citations;
      }
    } catch (e) {
      // Not JSON, use raw content as message
      log(Subsystems.CRIMSON, Status.DEBUG, 'Response is plain text, not JSON');
    }

    return result;
  }

  /**
   * Add follow-up question buttons after a message element in the conversation
   * @param {HTMLElement} messageEl - The message element (follow-ups added after it)
   * @param {string[]} questions - Array of follow-up questions
   */
  addFollowUpQuestions(messageEl, questions) {
    // Remove any existing follow-ups first
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

    // Insert follow-ups after the message element in the conversation
    messageEl.parentNode.insertBefore(container, messageEl.nextSibling);
    processIcons(container);
  }

  /**
   * Handle suggestions from the AI response
   * @param {Object} suggestions - Suggestions object
   */
  handleSuggestions(suggestions) {
    // Store suggestions for later use
    this.lastSuggestions = suggestions;

    // Handle tour offers
    if (suggestions.offerTours && suggestions.offerTours.length > 0) {
      log(Subsystems.CRIMSON, Status.DEBUG, `Tours available: ${JSON.stringify(suggestions.offerTours)}`);
      // Tours can be triggered by clicking follow-up questions
    }
  }

  /**
   * Handle stream error
   * @param {string|Error} error - Error message or Error object
   */
  handleStreamError(error) {
    const errorMessage = error instanceof Error ? error.message : error;

    // Check if this was a cancelled request - don't show error
    if (errorMessage.includes('abort') || errorMessage.includes('cancel') || errorMessage.includes('Connection closed')) {
      this.addDebugMessage('CANCELLED', 'Request was cancelled or connection closed');
      return;
    }

    this.addDebugMessage('ERROR', errorMessage);
    this.updateStatus('error', `Error: ${errorMessage.substring(0, 50)}...`);

    // Remove streaming indicator
    if (this.currentStreamElement) {
      this.currentStreamElement.remove();
      this.currentStreamElement = null;
    }

    // Add error message
    this.addMessage('agent', `Sorry, I encountered an error: ${errorMessage}`);

    // Reset streaming state and buffers
    this.isStreaming = false;
    this.seenDelimiter = false;
    this.conversationBuffer = '';
    this.metadataBuffer = '';
    this.partialDelimiter = '';
    this.input.focus();

    // Reset status to ready after a delay
    setTimeout(() => {
      this.updateStatus('ready', 'Ready');
    }, 3000);
  }

  /**
   * Add a streaming message placeholder
   * @returns {HTMLElement} The streaming message element
   */
  addStreamingMessage() {
    // Fade out and remove welcome message if present
    const welcome = this.conversation.querySelector('.crimson-welcome');
    if (welcome) {
      welcome.classList.add('crimson-welcome-fading');
      setTimeout(() => welcome.remove(), 350);
    }

    // Reset delimiter state and buffers for new message
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
  }

  /**
   * Format message content with basic markdown support (for streaming)
   * @param {string} content - Raw content
   * @returns {string} Formatted HTML
   */
  formatMessageContent(content) {
    if (!content) return '';

    // Escape HTML first
    const escaped = this.escapeHtml(content);

    // Basic markdown-like formatting for streaming display
    const formatted = escaped
      // Code blocks (```code```)
      .replace(/```([\s\S]*?)```/g, '<pre class="crimson-code"><code>$1</code></pre>')
      // Inline code (`code`)
      .replace(/`([^`]+)`/g, '<code class="crimson-inline-code">$1</code>')
      // Bold (**text**)
      .replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>')
      // Italic (*text*)
      .replace(/\*([^*]+)\*/g, '<em>$1</em>')
      // Line breaks
      .replace(/\n/g, '<br>');

    return formatted;
  }

  /**
   * Format message content with full markdown support using marked (for final content)
   * @param {string} content - Raw content
   * @returns {Promise<string>} Formatted HTML
   */
  async formatMessageContentMarkdown(content) {
    if (!content) return '';

    try {
      const { marked } = await import('marked');
      // Configure marked for GFM (tables, etc.)
      marked.setOptions({
        gfm: true,
        breaks: true,
      });
      const htmlContent = await marked.parse(content);
      return htmlContent;
    } catch (error) {
      log(Subsystems.CRIMSON, Status.WARN, `Failed to render markdown: ${error.message}`);
      // Fallback to basic formatting
      return this.formatMessageContent(content);
    }
  }

  /**
   * Simulate Crimson response (placeholder - not used with WebSocket)
   * @deprecated Use WebSocket chat instead
   */
  simulateCrimsonResponse() {
    // This method is no longer used - chat is handled via WebSocket
    log(Subsystems.CRIMSON, Status.WARN, ' simulateCrimsonResponse called but WebSocket should be used');
  }

  /**
   * Escape HTML to prevent XSS
   * @param {string} text - Text to escape
   * @returns {string} Escaped text
   */
  escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }

  // ==================== CITATIONS ====================

  /**
   * Parse citations from metadata and store for a message
   * @param {string} messageId - The message element ID
   * @param {Array} citations - Array of citation objects from metadata
   */
  parseCitations(messageId, citations) {
    log(Subsystems.CRIMSON, Status.DEBUG, `parseCitations called for ${messageId}: ${JSON.stringify(citations)?.substring(0, 200)}`);
    if (!citations || !Array.isArray(citations) || citations.length === 0) {
      log(Subsystems.CRIMSON, Status.DEBUG, `parseCitations: invalid citations array`);
      return;
    }

    // Normalize citation data
    const normalizedCitations = citations.map((citation, index) => ({
      number: citation.number || index + 1,
      name: citation.name || citation.title || 'Untitled',
      url: citation.url || citation.uri || citation.source || '',
      type: citation.type || 'web',
      source: citation.source || '',
    }));

    this.citations.set(messageId, normalizedCitations);

    // Add citation button to the message
    this.addCitationButtonToMessage(messageId, normalizedCitations.length);
  }

  /**
   * Add a citation button under the Crimson avatar for a message
   * @param {string} messageId - The message element ID
   * @param {number} count - Number of citations
   */
  addCitationButtonToMessage(messageId, count) {
    log(Subsystems.CRIMSON, Status.DEBUG, `addCitationButtonToMessage: ${messageId}, count=${count}`);
    const messageEl = document.getElementById(messageId);
    if (!messageEl) {
      log(Subsystems.CRIMSON, Status.WARN, `addCitationButtonToMessage: message element not found: ${messageId}`);
      return;
    }

    // Check if button already exists
    if (messageEl.querySelector('.crimson-citation-btn')) {
      log(Subsystems.CRIMSON, Status.DEBUG, `addCitationButtonToMessage: button already exists`);
      return;
    }

    const avatarEl = messageEl.querySelector('.crimson-message-avatar');
    if (!avatarEl) {
      log(Subsystems.CRIMSON, Status.WARN, `addCitationButtonToMessage: avatar not found`);
      return;
    }

    // Create citation button - double height with icon and counter
    const citationBtn = document.createElement('button');
    citationBtn.className = 'crimson-citation-btn';
    citationBtn.setAttribute('data-citation-count', count);
    citationBtn.setAttribute('data-message-id', messageId);
    citationBtn.innerHTML = `
      <fa fa-book-open></fa>
      <span class="crimson-citation-count">${count.toString().padStart(2, '0')}</span>
    `;

    citationBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.toggleCitationPopup(messageId, citationBtn);
    });

    // Insert after avatar
    avatarEl.after(citationBtn);
    processIcons(citationBtn);

    log(Subsystems.CRIMSON, Status.DEBUG, `Added citation button with ${count} citations to message ${messageId}`);
  }

  /**
   * Toggle citation popup for a message
   * @param {string} messageId - The message element ID
   * @param {HTMLElement} button - The citation button
   */
  toggleCitationPopup(messageId, button) {
    // Close existing popup if different
    if (this.activeCitationPopup && this.activeCitationButton !== button) {
      this.closeCitationPopup();
    }

    // Toggle off if same button
    if (this.activeCitationPopup && this.activeCitationButton === button) {
      this.closeCitationPopup();
      return;
    }

    const citations = this.citations.get(messageId);
    if (!citations || citations.length === 0) return;

    this.showCitationPopup(button, citations);
  }

  /**
   * Show citation popup anchored to button
   * @param {HTMLElement} button - The citation button
   * @param {Array} citations - Array of citation objects
   */
  showCitationPopup(button, citations) {
    // Create popup
    const popup = document.createElement('div');
    popup.className = 'crimson-citation-popup';
    this._citationPopupEl = popup;

    // Build citation table rows
    const rowsHtml = citations.map((citation) => {
      const isCanvas = citation.url.startsWith('canvas') || citation.source.startsWith('canvas');
      const canvasIcon = isCanvas ? '<fa fa-graduation-cap></fa>' : '';
      const displayUrl = citation.url.length > 50 ? citation.url.substring(0, 50) + '...' : citation.url;

      return `
        <tr class="crimson-citation-row" data-citation-number="${citation.number}">
          <td class="crimson-citation-cell crimson-citation-number">${citation.number}</td>
          <td class="crimson-citation-cell crimson-citation-name" title="${this.escapeHtml(citation.name)}">${this.escapeHtml(citation.name)}</td>
          <td class="crimson-citation-cell crimson-citation-url" title="${this.escapeHtml(citation.url)}">${this.escapeHtml(displayUrl)}</td>
          <td class="crimson-citation-cell crimson-citation-type">${canvasIcon}</td>
          <td class="crimson-citation-cell crimson-citation-actions">
            <button type="button" class="crimson-citation-action-btn crimson-citation-open-btn" data-url="${this.escapeHtml(citation.url)}" title="Open in new tab">
              <fa fa-arrow-up-right-from-square></fa>
            </button>
          </td>
        </tr>
      `;
    }).join('');

    popup.innerHTML = `
      <div class="crimson-citation-popup-header">
        <span class="crimson-citation-popup-title">Citations</span>
        <button type="button" class="crimson-citation-popup-close" title="Close (ESC)">
          <fa fa-xmark></fa>
        </button>
      </div>
      <div class="crimson-citation-popup-content">
        <table class="crimson-citation-table">
          <thead>
            <tr>
              <th class="crimson-citation-header">#</th>
              <th class="crimson-citation-header">Name</th>
              <th class="crimson-citation-header">URL</th>
              <th class="crimson-citation-header">Type</th>
              <th class="crimson-citation-header">Actions</th>
            </tr>
          </thead>
          <tbody>
            ${rowsHtml}
          </tbody>
        </table>
      </div>
      <div class="crimson-citation-popup-arrow"></div>
    `;

    document.body.appendChild(popup);
    processIcons(popup);

    // Position popup
    this.positionCitationPopup(popup, button);

    // Wire close button
    const closeBtn = popup.querySelector('.crimson-citation-popup-close');
    closeBtn?.addEventListener('click', () => this.closeCitationPopup());

    // Wire open buttons
    popup.querySelectorAll('.crimson-citation-open-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        e.stopPropagation();
        const url = btn.getAttribute('data-url');
        if (url) {
          // For canvas URLs, we'll handle them specially later
          if (url.startsWith('canvas')) {
            // TODO: Open canvas content in training panel
            log(Subsystems.CRIMSON, Status.INFO, `Opening canvas citation: ${url}`);
          } else {
            window.open(url, '_blank', 'noopener,noreferrer');
          }
        }
      });
    });

    // Store active popup
    this.activeCitationPopup = popup;
    this.activeCitationButton = button;
    button.classList.add('active');

    // Animate in
    requestAnimationFrame(() => {
      popup.classList.add('visible');
    });

    // Handle clicks outside to close
    this._citationOutsideHandler = (e) => {
      if (!popup.contains(e.target) && !button.contains(e.target)) {
        this.closeCitationPopup();
      }
    };
    document.addEventListener('click', this._citationOutsideHandler);

    log(Subsystems.CRIMSON, Status.DEBUG, `Opened citation popup with ${citations.length} citations`);
  }

  /**
   * Position citation popup relative to button
   * @param {HTMLElement} popup - The popup element
   * @param {HTMLElement} button - The anchor button
   */
  positionCitationPopup(popup, button) {
    const buttonRect = button.getBoundingClientRect();
    const popupRect = popup.getBoundingClientRect();

    // Default: position to the right of the button
    let left = buttonRect.right + 10;
    let top = buttonRect.top;

    // Check if popup goes off right edge
    if (left + popupRect.width > window.innerWidth - 20) {
      // Position to the left instead
      left = buttonRect.left - popupRect.width - 10;
    }

    // Check vertical bounds
    if (top + popupRect.height > window.innerHeight - 20) {
      top = window.innerHeight - popupRect.height - 20;
    }
    if (top < 20) {
      top = 20;
    }

    popup.style.left = `${left}px`;
    popup.style.top = `${top}px`;

    // Position arrow
    const arrow = popup.querySelector('.crimson-citation-popup-arrow');
    if (arrow) {
      const arrowOffset = buttonRect.top - top + buttonRect.height / 2;
      arrow.style.top = `${Math.max(10, Math.min(arrowOffset, popupRect.height - 10))}px`;

      // Point arrow left or right depending on popup position
      if (left < buttonRect.left) {
        arrow.classList.add('arrow-right');
      } else {
        arrow.classList.add('arrow-left');
      }
    }
  }

  /**
   * Close citation popup
   */
  closeCitationPopup() {
    // Remove outside click handler
    if (this._citationOutsideHandler) {
      document.removeEventListener('click', this._citationOutsideHandler);
      this._citationOutsideHandler = null;
    }

    if (this.activeCitationPopup) {
      this.activeCitationPopup.classList.remove('visible');
      setTimeout(() => {
        this.activeCitationPopup?.remove();
        this.activeCitationPopup = null;
        this._citationPopupEl = null;
      }, 200);
    }

    if (this.activeCitationButton) {
      this.activeCitationButton.classList.remove('active');
      this.activeCitationButton = null;
    }
  }

  /**
   * Highlight a specific citation in the popup
   * @param {number} citationNumber - The citation number to highlight
   */
  highlightCitation(citationNumber) {
    if (!this.activeCitationPopup) return;

    // Remove existing highlight
    this.activeCitationPopup.querySelectorAll('.crimson-citation-row').forEach(row => {
      row.classList.remove('highlighted');
    });

    // Add highlight to target row
    const targetRow = this.activeCitationPopup.querySelector(`[data-citation-number="${citationNumber}"]`);
    if (targetRow) {
      targetRow.classList.add('highlighted');
      targetRow.scrollIntoView({ behavior: 'smooth', block: 'center' });
    }
  }

  /**
   * Convert citation markers [[C1]] in text to clickable links
   * @param {string} content - The message content
   * @param {string} messageId - The message element ID
   * @returns {string} Content with citation links
   */
  convertCitationMarkers(content, messageId) {
    if (!content) return '';

    // Replace [[C1]], [[C2]], etc. with clickable links
    return content.replace(/\[\[C(\d+)\]\]/g, (match, num) => {
      const citationNum = parseInt(num, 10);
      return `<a href="#" class="crimson-citation-link" data-message-id="${messageId}" data-citation="${citationNum}" title="View citation ${citationNum}">${match}</a>`;
    });
  }

  /**
   * Handle citation link click
   * @param {Event} e - Click event
   */
  handleCitationLinkClick(e) {
    const link = e.target.closest('.crimson-citation-link');
    if (!link) return;

    e.preventDefault();
    e.stopPropagation();

    const messageId = link.getAttribute('data-message-id');
    const citationNum = parseInt(link.getAttribute('data-citation'), 10);

    if (!messageId || !citationNum) return;

    // Find the citation button for this message
    const messageEl = document.getElementById(messageId);
    if (!messageEl) return;

    const citationBtn = messageEl.querySelector('.crimson-citation-btn');
    if (!citationBtn) return;

    // Open popup and highlight citation
    this.toggleCitationPopup(messageId, citationBtn);
    this.highlightCitation(citationNum);
  }

  /**
   * Clean up the Crimson instance
   */
  destroy() {
    // Close citation popup if open
    this.closeCitationPopup();

    // Remove event listeners
    document.removeEventListener('keydown', this.handleKeyDown);
    document.removeEventListener('keydown', globalKeyHandler);

    // Cleanup WebSocket client state (does NOT disconnect - managed by app-ws)
    if (this.wsClient) {
      this.wsClient.cleanup();
      this.wsClient = null;
    }

    // Remove DOM elements
    this.overlay?.remove();
    this.popup?.remove();

    // Clear references
    this.overlay = null;
    this.popup = null;
    this.conversation = null;
    this.input = null;
    this.sendBtn = null;
    this.currentStreamElement = null;
    this.conversationHistory = [];
    this.citations.clear();
    crimsonInstance = null;
    globalKeyHandler = null;

    log(Subsystems.CRIMSON, Status.INFO, 'Destroyed');
  }
}

// Initialize global shortcut on module load
initCrimsonShortcut();
