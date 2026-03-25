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
import { log, Subsystems, Status } from '../../core/log.js';
import { getCrimsonWS } from '../../shared/crimson-ws.js';
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
export function createCrimsonButton(tooltip = 'Chat with Crimson (Ctrl+Shift+C)') {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close crimson-btn';
  button.title = tooltip;
  button.innerHTML = '<fa fa-fire></fa>';

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
  log(Subsystems.MANAGER, Status.INFO, '[Crimson] Global keyboard shortcut initialized (Ctrl+Shift+C)');
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

    // Debug state (always collect, just hide/show)
    this.debugMode = false;
    this.debugChunks = [];

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
      <div class="crimson-resize-handle crimson-resize-handle-tl" title="Resize"></div>
      <div class="crimson-resize-handle crimson-resize-handle-tr" title="Resize"></div>
      <div class="crimson-header">
        <div class="subpanel-header-group">
          <button type="button" class="crimson-header-primary">
            <fa fa-fire></fa>
            <span>Chat with Crimson</span>
          </button>
          <button type="button" class="crimson-streaming-btn" title="Toggle streaming">
            <fa fa-water></fa>
          </button>
          <button type="button" class="crimson-reasoning-btn" title="Toggle reasoning display">
            <fa fa-person></fa>
          </button>
          <button type="button" class="crimson-debug-btn" title="Toggle debug view">
            <fa fa-bug></fa>
          </button>
          <button type="button" class="crimson-reset-btn" title="Reset conversation">
            <fa fa-broom></fa>
          </button>
          <button type="button" class="crimson-header-close" title="Close (ESC)">
            <fa fa-xmark></fa>
          </button>
        </div>
      </div>
      <div class="crimson-reasoning-panel">
        <div class="crimson-reasoning-header">Reasoning</div>
        <div class="crimson-reasoning-content"></div>
        <div class="crimson-reasoning-splitter" title="Drag to resize"></div>
      </div>
      <div class="crimson-status-bar">
        <span class="crimson-status-indicator"></span>
        <span class="crimson-status-text">Ready</span>
      </div>
      <div class="crimson-conversation">
        <div class="crimson-welcome">
          <div class="crimson-welcome-icon">
            <fa fa-fire></fa>
          </div>
          <div class="crimson-welcome-text">Hello, <span class="crimson-username">User</span>!</div>
          <div class="crimson-welcome-hint">How can I help you today?</div>
        </div>
      </div>
      <div class="crimson-debug-panel">
        <div class="crimson-debug-splitter" title="Drag to resize"></div>
        <div class="crimson-debug-header">Debug Output</div>
        <pre class="crimson-debug-content"></pre>
      </div>
      <div class="crimson-input-area">
        <textarea class="crimson-input" placeholder="Type your message..." rows="1"></textarea>
        <button type="button" class="crimson-send-btn" title="Send message">
          <fa fa-up></fa>
        </button>
      </div>
      <div class="crimson-resize-handle crimson-resize-handle-bl" title="Resize"></div>
      <div class="crimson-resize-handle crimson-resize-handle-br" title="Resize"></div>
    `;

    document.body.appendChild(this.popup);

    // Cache element references
    this.conversation = this.popup.querySelector('.crimson-conversation');
    this.input = this.popup.querySelector('.crimson-input');
    this.sendBtn = this.popup.querySelector('.crimson-send-btn');
    this.statusBar = this.popup.querySelector('.crimson-status-bar');
    this.statusIndicator = this.popup.querySelector('.crimson-status-indicator');
    this.statusText = this.popup.querySelector('.crimson-status-text');
    this.debugPanel = this.popup.querySelector('.crimson-debug-panel');
    this.debugContent = this.popup.querySelector('.crimson-debug-content');
    this.reasoningPanel = this.popup.querySelector('.crimson-reasoning-panel');
    this.reasoningContent = this.popup.querySelector('.crimson-reasoning-content');
    this.streamingBtn = this.popup.querySelector('.crimson-streaming-btn');
    this.reasoningBtn = this.popup.querySelector('.crimson-reasoning-btn');
    const header = this.popup.querySelector('.crimson-header');
    const closeBtn = this.popup.querySelector('.crimson-header-close');
    const resetBtn = this.popup.querySelector('.crimson-reset-btn');
    const debugBtn = this.popup.querySelector('.crimson-debug-btn');
    const resizeHandleBR = this.popup.querySelector('.crimson-resize-handle-br');
    const resizeHandleBL = this.popup.querySelector('.crimson-resize-handle-bl');
    const resizeHandleTR = this.popup.querySelector('.crimson-resize-handle-tr');
    const resizeHandleTL = this.popup.querySelector('.crimson-resize-handle-tl');
    const debugSplitter = this.popup.querySelector('.crimson-debug-splitter');
    const reasoningSplitter = this.popup.querySelector('.crimson-reasoning-splitter');

    // Set up event listeners
    header.addEventListener('mousedown', this.handleDragStart);
    closeBtn.addEventListener('click', () => this.hide());
    resetBtn?.addEventListener('click', () => this.resetConversation());
    debugBtn?.addEventListener('click', () => this.toggleDebugPanel());
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

    log(Subsystems.MANAGER, Status.INFO, '[Crimson] Initialized');
  }

  /**
   * Apply persistent state to UI elements
   */
  applyPersistentState() {
    // Apply debug state
    this.popup.classList.toggle('crimson-debug-visible', this.debugMode);
    if (this.debugMode) {
      this.debugBtn?.classList.add('active');
    }
    this.updateDebugPanel();

    // Apply streaming state
    this.updateStreamingButton();

    // Apply reasoning state
    this.popup.classList.toggle('crimson-reasoning-visible', this.reasoningMode);
    if (this.reasoningMode) {
      this.reasoningBtn?.classList.add('active');
    }
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

    log(Subsystems.MANAGER, Status.INFO, '[Crimson] Shown');
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

    log(Subsystems.MANAGER, Status.INFO, '[Crimson] Hidden');
  }

  /**
   * Reset the conversation to empty state
   */
  resetConversation() {
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

    // Clear conversation and restore welcome message
    if (this.conversation) {
      this.conversation.innerHTML = `
        <div class="crimson-welcome">
          <div class="crimson-welcome-icon">
            <fa fa-fire></fa>
          </div>
          <div class="crimson-welcome-text">Hello, <span class="crimson-username">${this.escapeHtml(this.username)}</span>!</div>
          <div class="crimson-welcome-hint">How can I help you today?</div>
        </div>
      `;
      processIcons(this.conversation);
    }

    // Clear input
    if (this.input) {
      this.input.value = '';
      this.autoResizeInput();
    }

    // Clear message history
    this.messages = [];

    // Clear debug panel
    this.debugChunks = [];
    this.updateDebugPanel();

    // Reset status
    this.updateStatus('ready', 'Ready');

    log(Subsystems.MANAGER, Status.INFO, '[Crimson] Conversation reset');
  }

  /**
   * Update the status bar
   * @param {string} state - Status state: 'ready', 'connecting', 'thinking', 'streaming', 'error'
   * @param {string} text - Status text to display
   */
  updateStatus(state, text) {
    if (!this.statusIndicator || !this.statusText) return;

    this.connectionState = state;
    this.statusIndicator.className = `crimson-status-indicator crimson-status-${state}`;
    this.statusText.textContent = text;

    // Add animation for active states
    if (state === 'thinking' || state === 'streaming') {
      this.statusBar.classList.add('crimson-status-active');
    } else {
      this.statusBar.classList.remove('crimson-status-active');
    }
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
    this.streamingBtn?.setAttribute('title', `Streaming: ${this.streamingEnabled ? 'ON' : 'OFF'}`);
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
    this.reasoningBtn?.setAttribute('title', `Reasoning: ${this.reasoningMode ? 'SHOWING' : 'HIDDEN'}`);
  }

  /**
   * Add debug message to the debug panel
   * Always collects data, only displays when debugMode is true
   * @param {string} type - Message type
   * @param {string} content - Message content
   */
  addDebugMessage(type, content) {
    const timestamp = new Date().toLocaleTimeString();
    this.debugChunks.push(`[${timestamp}] ${type}: ${content}`);
    
    // Keep only last 200 messages
    if (this.debugChunks.length > 200) {
      this.debugChunks.shift();
    }

    // Only update display if debug mode is active
    if (this.debugMode) {
      this.updateDebugPanel();
    }
  }

  /**
   * Update the debug panel content
   */
  updateDebugPanel() {
    if (!this.debugContent) return;
    this.debugContent.textContent = this.debugChunks.join('\n');
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

    this.addDebugMessage('REASONING', `Chunk: ${content.substring(0, 50)}...`);
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
      this.hide();
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

    // Add user message
    this.addMessage('user', message);

    // Add to conversation history for context
    this.conversationHistory.push({ role: 'user', content: message });

    // Clear input
    this.input.value = '';
    this.autoResizeInput();

    // Send message via WebSocket
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
   */
  addMessage(sender, text) {
    // Remove welcome message if present
    const welcome = this.conversation.querySelector('.crimson-welcome');
    if (welcome) {
      welcome.remove();
    }

    const messageEl = document.createElement('div');
    messageEl.className = `crimson-message crimson-message-${sender}`;

    const avatar = sender === 'agent'
      ? '<div class="crimson-message-avatar"><fa fa-fire></fa></div>'
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
  }

  /**
   * Add typing indicator
   */
  addTypingIndicator() {
    const indicator = document.createElement('div');
    indicator.className = 'crimson-message crimson-message-agent crimson-typing-indicator';
    indicator.innerHTML = `
      <div class="crimson-message-avatar"><fa fa-fire></fa></div>
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
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] Initializing WebSocket client`);
    // Create or update the Crimson WS client (only handles messages, not connection)
    this.wsClient = getCrimsonWS({
      onChunk: (content, index, finishReason, reasoningContent) => {
        this.addDebugMessage('WS_CALLBACK', `onChunk called: content_len=${content?.length || 0}, reasoning_len=${reasoningContent?.length || 0}, index=${index}, finish=${finishReason || 'none'}`);
        this.handleStreamChunk(content, index, finishReason, reasoningContent);
      },
      onDone: (content, result) => {
        this.addDebugMessage('WS_CALLBACK', `onDone called: content len=${content?.length || 0}`);
        this.handleStreamDone(content, result);
      },
      onError: (error) => {
        this.addDebugMessage('WS_CALLBACK', `onError called: ${error}`);
        this.handleStreamError(error);
      },
    });
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] WebSocket client initialized, handlers registered`);
  }

  /**
   * Send chat message via WebSocket
   * Uses the persistent app-ws.js connection - does NOT manage connection lifecycle
   * @param {string} message - User message
   */
  async sendChatMessage(message) {
    // Initialize WebSocket client if needed (only sets up callbacks, doesn't connect)
    this.initWebSocketClient();

    // Debug: Check WebSocket state and handlers
    import('../../shared/app-ws.js').then(({ getAppWS, isAppWSConnected }) => {
      const ws = getAppWS();
      this.addDebugMessage('WS_STATE', `Connected: ${isAppWSConnected()}, handlers: ${ws.debugHandlers().join(', ')}`);
    });

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

    // Update status to thinking
    this.updateStatus('thinking', 'Thinking...');

    // Add debug message
    this.addDebugMessage('SEND', message);

    // Add streaming indicator
    this.currentStreamElement = this.addStreamingMessage();

    // Clear reasoning buffer for new response
    this.clearReasoningBuffer();

    try {
      // Send message - connection must already be established by app-ws.js
      // Use streamingEnabled state to determine if we should stream
      const history = this.conversationHistory.slice(-10); // Last 10 messages for context
      this.addDebugMessage('SEND', `Sending with ${history.length} history messages, stream=${this.streamingEnabled}`);
      
      await this.wsClient.send(message, {
        history,
        stream: this.streamingEnabled,
      });
    } catch (error) {
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
      
      log(Subsystems.MANAGER, Status.ERROR, `[Crimson] Chat error: ${error.message}`);
      this.handleStreamError(error.message);
    }
  }

  /**
   * Handle incoming stream chunk - accumulates content and displays in real-time
   */
  handleStreamChunk(content, _index, finishReason, reasoningContent) {
    // Handle reasoning content first (if present)
    if (reasoningContent) {
      this.addReasoningContent(reasoningContent);
    }

    if (!content && !finishReason) return;
    
    // Get content element
    let contentEl = null;
    if (this.currentStreamElement) {
      contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
    }
    
    if (!contentEl) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] No contentEl found, currentStreamElement: ${!!this.currentStreamElement}`);
      return;
    }

    // Update total chunk counter
    this.totalChunksReceived++;
    
    // Add debug message for every chunk
    this.addDebugMessage('CHUNK', `#${this.totalChunksReceived} len=${content?.length || 0} finish=${finishReason || 'none'}`);
    
    // Log only every 10th chunk or when finish reason is set
    if (this.totalChunksReceived % 10 === 0 || finishReason) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] Chunk #${this.totalChunksReceived}, finish: ${finishReason || 'none'}, content length: ${content?.length || 0}`);
    }

    // Update status on first content
    if (this.connectionState !== 'streaming') {
      this.updateStatus('streaming', 'Receiving response...');
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

      // Update display with current content
      const displayContent = this.conversationBuffer.replace(/\s+$/, '');
      contentEl.setAttribute('data-raw-content', displayContent);
      contentEl.innerHTML = this.formatMessageContent(displayContent);

      // Scroll to show new content
      if (this.conversation) {
        this.conversation.scrollTop = this.conversation.scrollHeight;
      }
    }

    // Handle stream completion
    if (finishReason) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] Stream complete (${finishReason})`);
      this.addDebugMessage('COMPLETE', `Stream finished with reason: ${finishReason}`);
      this.handleStreamFinished();
    }
  }

  /**
   * Handle stream completion after all chunks received
   * This is called when finish_reason arrives in the last chunk
   */
  handleStreamFinished() {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] handleStreamFinished called, chunks: ${this.totalChunksReceived}, isStreaming: ${this.isStreaming}`);
    
    try {
      // If there's a pending done result, process it now
      if (this.wsClient && this.wsClient.pendingDoneResult) {
        const { id, result } = this.wsClient.pendingDoneResult;
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] Processing pending done result for ${id}`);
        this.wsClient.pendingDoneResult = null;
        
        // Call the onDone callback to update UI
        this.handleStreamDone(result?.content, result);
      } else {
        // Finalize with current buffers (no done message received yet)
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] No pending done, finalizing with buffer length: ${this.conversationBuffer.length}`);
        this.handleStreamDone(this.conversationBuffer, null);
      }
    } catch (error) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] Error in handleStreamFinished: ${error.message}`);
    }
    
    // Always try to resolve any pending Promise and clear state
    if (this.wsClient) {
      const id = this.wsClient.currentRequestId;
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] Resolving pending request: ${id}`);
      
      if (id && this.wsClient.pendingRequests && this.wsClient.pendingRequests.has(id)) {
        const callbacks = this.wsClient.pendingRequests.get(id);
        if (callbacks && callbacks.onDone) {
          log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] Calling onDone callback to resolve Promise`);
          callbacks.onDone({ content: this.conversationBuffer });
          this.wsClient.pendingRequests.delete(id);
        }
      }
      this.wsClient.currentRequestId = null;
    }
    
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] handleStreamFinished complete, isStreaming: ${this.isStreaming}`);
  }



  /**
   * Handle stream completion
   * @param {string} content - Final complete content (may be from pending done or our buffer)
   * @param {Object} result - Full result object (may contain followUpQuestions, etc.)
   */
  handleStreamDone(content, result) {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[Crimson] handleStreamDone called, isStreaming: ${this.isStreaming}, content length: ${content?.length || 0}`);

    try {
      this.addDebugMessage('DONE', 'Stream complete');

      // For non-streaming responses, content includes the delimiter and metadata
      // We need to parse it to separate conversation text from metadata
      let conversationText = content || '';
      let metadataFromContent = null;
      
      if (!this.isStreaming && content) {
        // Non-streaming: parse delimiter from content
        const delimiterIndex = content.indexOf(this.DELIMITER);
        if (delimiterIndex !== -1) {
          conversationText = content.substring(0, delimiterIndex).replace(/\s+$/, '');
          const metadataStr = content.substring(delimiterIndex + this.DELIMITER.length);
          try {
            metadataFromContent = JSON.parse(metadataStr);
            this.addDebugMessage('META', 'Parsed from non-streaming content');
          } catch (e) {
            log(Subsystems.MANAGER, Status.WARN, `[Crimson] Failed to parse metadata from content: ${e.message}`);
          }
        } else {
          // No delimiter, use all content as conversation
          conversationText = content.replace(/\s+$/, '');
        }
      }

      // Parse metadata JSON if we have it (from chunks) or from result
      let parsedMetadata = metadataFromContent;
      
      // First try to parse from metadata buffer (chunk-based metadata)
      if (!parsedMetadata && this.metadataBuffer && this.metadataBuffer.trim()) {
        try {
          parsedMetadata = JSON.parse(this.metadataBuffer.trim());
          this.addDebugMessage('META', 'From chunks: ' + JSON.stringify(parsedMetadata).substring(0, 100));
        } catch (e) {
          log(Subsystems.MANAGER, Status.WARN, `[Crimson] Failed to parse metadata JSON: ${e.message}`);
        }
      }
      
      // If no metadata from chunks, try from result object (from done message)
      if (!parsedMetadata && result) {
        // The result might have followUpQuestions and other metadata directly
        parsedMetadata = {
          followUpQuestions: result.followUpQuestions || [],
          suggestions: result.suggestions || null,
          metadata: result.metadata || null,
        };
        this.addDebugMessage('META', 'From done message');
      }

      if (this.currentStreamElement) {
        const contentEl = this.currentStreamElement.querySelector('.crimson-message-content');
        if (contentEl) {
          // Set the final conversation content (already parsed to remove delimiter)
          contentEl.setAttribute('data-raw-content', conversationText);
          contentEl.innerHTML = this.formatMessageContent(conversationText);

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
      log(Subsystems.MANAGER, Status.ERROR, `[Crimson] Error in handleStreamDone: ${error.message}`);
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
   * @returns {Object} Parsed response with displayMessage, followUpQuestions, suggestions, metadata
   */
  parseCrimsonResponse(content) {
    const result = {
      displayMessage: content,
      followUpQuestions: [],
      suggestions: null,
      metadata: null,
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
    } catch (e) {
      // Not JSON, use raw content as message
      log(Subsystems.MANAGER, Status.DEBUG, '[Crimson] Response is plain text, not JSON');
    }

    return result;
  }

  /**
   * Add follow-up question buttons to a message element
   * @param {HTMLElement} messageEl - The message element
   * @param {string[]} questions - Array of follow-up questions
   */
  addFollowUpQuestions(messageEl, questions) {
    const container = document.createElement('div');
    container.className = 'crimson-followups';

    questions.forEach(question => {
      const btn = document.createElement('button');
      btn.className = 'crimson-followup-btn';
      btn.textContent = question;
      btn.addEventListener('click', () => {
        this.input.value = question;
        this.handleSend();
      });
      container.appendChild(btn);
    });

    messageEl.appendChild(container);
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
      log(Subsystems.MANAGER, Status.DEBUG, `[Crimson] Tours available: ${JSON.stringify(suggestions.offerTours)}`);
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
    // Remove welcome message if present
    const welcome = this.conversation.querySelector('.crimson-welcome');
    if (welcome) {
      welcome.remove();
    }

    // Reset delimiter state and buffers for new message
    this.seenDelimiter = false;
    this.conversationBuffer = '';
    this.metadataBuffer = '';
    this.partialDelimiter = '';

    const messageEl = document.createElement('div');
    messageEl.className = 'crimson-message crimson-message-agent crimson-streaming';
    messageEl.innerHTML = `
      <div class="crimson-message-avatar"><fa fa-fire></fa></div>
      <div class="crimson-message-content" data-raw-content=""></div>
    `;

    this.conversation.appendChild(messageEl);
    processIcons(messageEl);
    this.conversation.scrollTop = this.conversation.scrollHeight;

    return messageEl;
  }

  /**
   * Format message content with basic markdown support
   * @param {string} content - Raw content
   * @returns {string} Formatted HTML
   */
  formatMessageContent(content) {
    if (!content) return '';

    // Escape HTML first
    const escaped = this.escapeHtml(content);

    // Basic markdown-like formatting
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
   * Simulate Crimson response (placeholder - not used with WebSocket)
   * @deprecated Use WebSocket chat instead
   */
  simulateCrimsonResponse() {
    // This method is no longer used - chat is handled via WebSocket
    log(Subsystems.MANAGER, Status.WARN, '[Crimson] simulateCrimsonResponse called but WebSocket should be used');
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

  /**
   * Clean up the Crimson instance
   */
  destroy() {
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
    crimsonInstance = null;
    globalKeyHandler = null;

    log(Subsystems.MANAGER, Status.INFO, '[Crimson] Destroyed');
  }
}

// Initialize global shortcut on module load
initCrimsonShortcut();
