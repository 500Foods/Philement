/**
 * Crimson Core Module
 *
 * Defines the base CrimsonManager class with:
 * - Constructor and state initialization
 * - DOM initialization and element creation
 * - Show/hide/window management
 * - State persistence (localStorage)
 * - Input history management
 *
 * @module managers/crimson/crimson-core
 */

import { processIcons } from '../../core/icons.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { getTip, initTooltips } from '../../core/tooltip-api.js';
import { registerShortcut } from '../../core/manager-ui.js';

// Singleton instance tracking
let crimsonInstance = null;

// Crimson shortcut registration flag
let _crimsonShortcutRegistered = false;

// LocalStorage keys for persistent state
const STORAGE_KEYS = {
  DEBUG_MODE: 'lithium_crimson_debug_mode',
  STREAMING_ENABLED: 'lithium_crimson_streaming_enabled',
  REASONING_MODE: 'lithium_crimson_reasoning_mode',
  CITATION_POPUP_X: 'lithium_crimson_citation_popup_x',
  CITATION_POPUP_Y: 'lithium_crimson_citation_popup_y',
  CITATION_POPUP_WIDTH: 'lithium_crimson_citation_popup_width',
  CITATION_POPUP_HEIGHT: 'lithium_crimson_citation_popup_height',
  MAIN_WINDOW_X: 'lithium_crimson_main_window_x',
  MAIN_WINDOW_Y: 'lithium_crimson_main_window_y',
  MAIN_WINDOW_WIDTH: 'lithium_crimson_main_window_width',
  MAIN_WINDOW_HEIGHT: 'lithium_crimson_main_window_height',
  INPUT_HISTORY: 'lithium_crimson_input_history',
};

const MAX_INPUT_HISTORY = 100;

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

  requestAnimationFrame(() => {
    processIcons(button);
  });

  return button;
}

/**
 * Initialize global keyboard shortcut (Ctrl+Shift+C to toggle Crimson)
 */
export function initCrimsonShortcut() {
  if (_crimsonShortcutRegistered) return;
  _crimsonShortcutRegistered = true;

  registerShortcut('crimson', 'Ctrl+Shift+C', 'Toggle Crimson', () => toggleCrimson());
  log(Subsystems.CRIMSON, Status.INFO, 'Global keyboard shortcut registered (Ctrl+Shift+C)');
}

/**
 * Crimson AI Agent Manager Class
 */
export class CrimsonManager {
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
    this.resizeCorner = 'br';

    // Track citation-to-row mapping for deduplication
    this.citationToRowMap = new Map();

    // Input history state
    this.inputHistory = [];
    this.inputHistoryIndex = -1;

    // Citation popup drag/resize state
    this.isCitationDragging = false;
    this.isCitationResizing = false;
    this.citationDragStartX = 0;
    this.citationDragStartY = 0;
    this.citationPopupStartX = 0;
    this.citationPopupStartY = 0;
    this.citationResizeStartX = 0;
    this.citationResizeStartY = 0;
    this.citationPopupStartWidth = 0;
    this.citationPopupStartHeight = 0;
    this.citationPopupStartLeft = 0;
    this.citationPopupStartTop = 0;
    this.citationResizeCorner = 'br';

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

    // Chunk counter for logging
    this.chunkCount = 0;
    this.totalChunksReceived = 0;

    // Temporarily enable showing thinking messages
    this.showThinkingMessages = true;

    // Citations state
    this.allCitations = [];
    this.citationIndex = new Map();
    this.activeCitationPopup = null;
    this.citationTable = null;
    this.citationHeaderBtn = null;
    this.citationCounterEl = null;

    // Bind core methods
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleOverlayClick = this.handleOverlayClick.bind(this);

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
    if (this.popup) return;

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
          <button type="button" class="crimson-citation-header-btn" data-tooltip="Citations">
            <fa fa-book-open-lines></fa>
            <span class="crimson-citation-counter"></span>
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
      <div class="crimson-conversation crimson-conversation-empty">
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
        <div class="crimson-input-nav">
          <button type="button" class="crimson-input-prev" data-tooltip="Previous input" disabled>
            <fa fa-chevron-up></fa>
          </button>
          <button type="button" class="crimson-input-next" data-tooltip="Next input" disabled>
            <fa fa-chevron-down></fa>
          </button>
        </div>
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
    this.citationHeaderBtn = this.popup.querySelector('.crimson-citation-header-btn');
    this.citationCounterEl = this.popup.querySelector('.crimson-citation-counter');
    this.inputPrevBtn = this.popup.querySelector('.crimson-input-prev');
    this.inputNextBtn = this.popup.querySelector('.crimson-input-next');
    this.streamingBtn = this.popup.querySelector('.crimson-streaming-btn');
    this.reasoningBtn = this.popup.querySelector('.crimson-reasoning-btn');
    this.debugBtn = this.popup.querySelector('.crimson-debug-btn');

    // Wire events - event methods defined in crimson-events.js
    const closeBtn = this.popup.querySelector('.crimson-header-close');
    const resetBtn = this.popup.querySelector('.crimson-reset-btn');

    closeBtn.addEventListener('click', () => this.hide());
    this.conversation.addEventListener('click', (e) => this.handleCitationLinkClick?.(e));
    resetBtn?.addEventListener('click', () => this.resetConversation?.());
    this.citationHeaderBtn?.addEventListener('click', (e) => {
      e.stopPropagation();
      this.toggleCitationPopup?.();
    });
    this.debugBtn?.addEventListener('click', () => this.toggleDebugPanel?.());
    this.streamingBtn?.addEventListener('click', () => this.toggleStreaming?.());
    this.reasoningBtn?.addEventListener('click', () => this.toggleReasoningPanel?.());

    // Input history navigation
    this.loadInputHistory();
    this.inputPrevBtn?.addEventListener('click', () => this.navigateInputHistory?.(-1));
    this.inputNextBtn?.addEventListener('click', () => this.navigateInputHistory?.(1));

    // Apply persistent state to UI
    this.applyPersistentState?.();

    // Process icons
    processIcons(this.popup);

    // Position popup initially (centered)
    this.centerPopup();

    log(Subsystems.CRIMSON, Status.INFO, 'Initialized');
  }

  /**
   * Center the popup on screen at 70% viewport size
   */
  centerPopup() {
    if (!this.popup) return;

    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    const width = Math.min(viewportWidth * 0.7, 1200);
    const height = Math.min(viewportHeight * 0.7, 800);

    this.popup.style.width = `${width}px`;
    this.popup.style.height = `${height}px`;
    this.popup.style.left = `${(viewportWidth - width) / 2}px`;
    this.popup.style.top = `${(viewportHeight - height) / 2}px`;
  }

  /**
   * Load main window position and size from localStorage
   * @returns {Object|null} Saved position/size or null
   */
  loadMainWindowState() {
    try {
      const x = localStorage.getItem(STORAGE_KEYS.MAIN_WINDOW_X);
      const y = localStorage.getItem(STORAGE_KEYS.MAIN_WINDOW_Y);
      const width = localStorage.getItem(STORAGE_KEYS.MAIN_WINDOW_WIDTH);
      const height = localStorage.getItem(STORAGE_KEYS.MAIN_WINDOW_HEIGHT);
      if (x !== null && y !== null && width !== null && height !== null) {
        return {
          x: parseInt(x, 10),
          y: parseInt(y, 10),
          width: parseInt(width, 10),
          height: parseInt(height, 10),
        };
      }
    } catch (e) {
      // localStorage may not be available
    }
    return null;
  }

  /**
   * Save main window position and size to localStorage
   */
  saveMainWindowState() {
    try {
      if (!this.popup) return;
      const rect = this.popup.getBoundingClientRect();
      localStorage.setItem(STORAGE_KEYS.MAIN_WINDOW_X, String(rect.left));
      localStorage.setItem(STORAGE_KEYS.MAIN_WINDOW_Y, String(rect.top));
      localStorage.setItem(STORAGE_KEYS.MAIN_WINDOW_WIDTH, String(rect.width));
      localStorage.setItem(STORAGE_KEYS.MAIN_WINDOW_HEIGHT, String(rect.height));
    } catch (e) {
      // localStorage may not be available
    }
  }

  /**
   * Load citation popup position and size from localStorage
   * @returns {Object|null} Saved position/size or null
   */
  loadCitationPopupState() {
    try {
      const x = localStorage.getItem(STORAGE_KEYS.CITATION_POPUP_X);
      const y = localStorage.getItem(STORAGE_KEYS.CITATION_POPUP_Y);
      const width = localStorage.getItem(STORAGE_KEYS.CITATION_POPUP_WIDTH);
      const height = localStorage.getItem(STORAGE_KEYS.CITATION_POPUP_HEIGHT);
      if (x !== null && y !== null && width !== null && height !== null) {
        return {
          x: parseInt(x, 10),
          y: parseInt(y, 10),
          width: parseInt(width, 10),
          height: parseInt(height, 10),
        };
      }
    } catch (e) {
      // localStorage may not be available
    }
    return null;
  }

  /**
   * Save citation popup position and size to localStorage
   * @param {HTMLElement} popup - The popup element
   */
  saveCitationPopupState(popup) {
    try {
      if (!popup) return;
      const rect = popup.getBoundingClientRect();
      localStorage.setItem(STORAGE_KEYS.CITATION_POPUP_X, String(rect.left));
      localStorage.setItem(STORAGE_KEYS.CITATION_POPUP_Y, String(rect.top));
      localStorage.setItem(STORAGE_KEYS.CITATION_POPUP_WIDTH, String(rect.width));
      localStorage.setItem(STORAGE_KEYS.CITATION_POPUP_HEIGHT, String(rect.height));
    } catch (e) {
      // localStorage may not be available
    }
  }

  /**
   * Show the Crimson popup
   * @param {Object} options - Show options
   * @param {string} options.username - Username for greeting
   */
  show(options = {}) {
    const isFirstShow = !this.popup;
    if (isFirstShow) this.init();

    if (options.username) {
      this.username = options.username;
      const usernameEl = this.popup.querySelector('.crimson-username');
      if (usernameEl) {
        usernameEl.textContent = this.username;
      }
    }

    if (this.isVisible) {
      this.input?.focus();
      return;
    }

    const savedState = this.loadMainWindowState();
    if (savedState) {
      this.popup.style.left = `${savedState.x}px`;
      this.popup.style.top = `${savedState.y}px`;
      this.popup.style.width = `${savedState.width}px`;
      this.popup.style.height = `${savedState.height}px`;
    } else if (isFirstShow) {
      this.centerPopup();
    }

    if (isFirstShow) {
      void this.overlay.offsetHeight;
      void this.popup.offsetHeight;
    }

    this.overlay.classList.add('visible');
    this.popup.classList.add('visible');
    this.isVisible = true;

    requestAnimationFrame(() => {
      const welcome = this.conversation?.querySelector('.crimson-welcome');
      if (welcome && !welcome.classList.contains('crimson-welcome-visible')) {
        welcome.classList.add('crimson-welcome-visible');
      }
    });

    document.addEventListener('keydown', this.handleKeyDown);

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

    this.saveMainWindowState();

    this.overlay.classList.remove('visible');
    this.popup.classList.remove('visible');
    this.isVisible = false;

    document.removeEventListener('keydown', this.handleKeyDown);

    if (document.activeElement && this.popup.contains(document.activeElement)) {
      document.activeElement.blur();
    }

    log(Subsystems.CRIMSON, Status.INFO, 'Hidden');
  }

  /**
   * Handle keyboard events (ESC to close)
   */
  handleKeyDown(e) {
    if (e.key === 'Escape') {
      e.preventDefault();
      if (this.activeCitationPopup) {
        this.closeCitationPopup?.();
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
   * Escape HTML to prevent XSS
   * @param {string} text - Text to escape
   * @returns {string} Escaped text
   */
  escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }
}

// Constants available to all modules
CrimsonManager.STORAGE_KEYS = STORAGE_KEYS;
CrimsonManager.MAX_INPUT_HISTORY = MAX_INPUT_HISTORY;

// Initialize global shortcut on module load
initCrimsonShortcut();
