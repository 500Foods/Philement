import { l as log, S as Status, a as Subsystems, i as initTooltips, p as processIcons, c as getRawLog, d as getTip, r as retrieveJWT, v as validateJWT, _ as __vitePreload } from './index-DLPCDk6c.js';
import { r as registerShortcut } from './manager-ui-UMxicChL.js';
import { f as formatLogText } from './languages-Coru4QdC.js';
import { r as registerWSHandler, u as unregisterWSHandler, i as isAppWSConnected, g as getAppWS } from './main-CwLxzNHC.js';
import { L as LithiumTable } from './lithium-table-main-BZjHbi6_.js';
import './conduit-D2QlMBxM.js';
import './codemirror-setup-lxLGSWU-.js';
import './codemirror-Bg1rM8n2.js';
/* empty css               */
import './zoom-control-CUgG5tfZ.js';
import './tabulator_esm-DsDetXL7.js';
import './scrollbar-manager-DTR0NQaT.js';

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
function getCrimson() {
  if (!crimsonInstance) {
    crimsonInstance = new CrimsonManager();
  }
  return crimsonInstance;
}

/**
 * Toggle the Crimson popup (convenience function)
 * @param {Object} options - Options for showing Crimson if opening
 */
function toggleCrimson(options = {}) {
  const crimson = getCrimson();
  if (crimson.isVisible) {
    crimson.hide();
  } else {
    crimson.show(options);
  }
}

/**
 * Initialize global keyboard shortcut (Ctrl+Shift+C to toggle Crimson)
 */
function initCrimsonShortcut() {
  if (_crimsonShortcutRegistered) return;
  _crimsonShortcutRegistered = true;

  registerShortcut('crimson', 'Ctrl+Shift+C', 'Toggle Crimson', () => toggleCrimson());
  log(Subsystems.CRIMSON, Status.INFO, 'Global keyboard shortcut registered (Ctrl+Shift+C)');
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
    this.resizeCorner = 'br';

    // Track citation-to-row mapping
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
    this.allCitations = [
      {
        number: 1,
        name: 'Intro to Lithium Course',
        url: 'https://canvas.500courses.com/courses/801',
        link: '<fa fa-graduation-cap></fa>',
        type: 'canvas',
        source: 'canvas',
        score: 1000,
        pageContent: null,
        dataSourceId: null,
      },
      {
        number: 2,
        name: 'Philement/Lithium Repository',
        url: 'https://github.com/500Foods/Philement/blob/main/elements/003-lithium/README.md',
        link: '<i class="fa-brands fa-github"></i>',
        type: 'web',
        source: 'github',
        score: 1000,
        pageContent: null,
        dataSourceId: null,
      }
    ];
    this.citationIndex = new Map();
    this.activeCitationPopup = null;
    this.citationTable = null;
    this.citationHeaderBtn = null;
    this.citationCounterEl = null;

    // Bind event handlers so document-level listeners keep the Crimson instance context
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleOverlayClick = this.handleOverlayClick.bind(this);
    this.handleDragStart = this.handleDragStart.bind(this);
    this.handleDragMove = this.handleDragMove.bind(this);
    this.handleDragEnd = this.handleDragEnd.bind(this);
    this.handleResizeMove = this.handleResizeMove.bind(this);
    this.handleResizeEnd = this.handleResizeEnd.bind(this);
    this.handleCitationDragStart = this.handleCitationDragStart.bind(this);
    this.handleCitationDragMove = this.handleCitationDragMove.bind(this);
    this.handleCitationDragEnd = this.handleCitationDragEnd.bind(this);
    this.handleCitationResizeMove = this.handleCitationResizeMove.bind(this);
    this.handleCitationResizeEnd = this.handleCitationResizeEnd.bind(this);
    this.handleDebugResizeMove = this.handleDebugResizeMove.bind(this);
    this.handleDebugResizeEnd = this.handleDebugResizeEnd.bind(this);
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
            <fa fa-thought-bubble></fa>
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
    const header = this.popup.querySelector('.crimson-header');

    closeBtn.addEventListener('click', () => this.hide());
    this.conversation.addEventListener('click', (e) => this.handleCitationLinkClick(e));
    resetBtn?.addEventListener('click', () => this.resetConversation());
    this.citationHeaderBtn?.addEventListener('click', (e) => {
      e.stopPropagation();
      this.toggleCitationPopup();
    });
    this.debugBtn?.addEventListener('click', () => this.toggleDebugPanel());
    this.streamingBtn?.addEventListener('click', () => this.toggleStreaming());
    this.reasoningBtn?.addEventListener('click', () => this.toggleReasoningPanel());

    // Drag handlers (defined in crimson-events.js)
    header?.addEventListener('mousedown', (e) => this.handleDragStart(e));

    // Resize handlers (defined in crimson-events.js)
    this.popup.querySelector('.crimson-resize-handle-br')?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'br'));
    this.popup.querySelector('.crimson-resize-handle-bl')?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'bl'));
    this.popup.querySelector('.crimson-resize-handle-tr')?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'tr'));
    this.popup.querySelector('.crimson-resize-handle-tl')?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'tl'));

    // Debug panel resize (defined in crimson-events.js)
    this.popup.querySelector('.crimson-debug-splitter')?.addEventListener('mousedown', (e) => this.handleDebugResizeStart(e));

    // Reasoning panel resize (defined in crimson-events.js)
    this.popup.querySelector('.crimson-reasoning-splitter')?.addEventListener('mousedown', (e) => this.handleReasoningResizeStart(e));

    // Send and input handling (defined in crimson-events.js)
    this.sendBtn?.addEventListener('click', () => this.handleSend());
    this.input?.addEventListener('keydown', (e) => this.handleInputKeydown(e));
    this.input?.addEventListener('input', () => this.autoResizeInput());

    // Input history navigation
    this.loadInputHistory();
    this.inputPrevBtn?.addEventListener('click', () => this.navigateInputHistory(-1));
    this.inputNextBtn?.addEventListener('click', () => this.navigateInputHistory(1));

    // Apply persistent state to UI
    this.applyPersistentState();

    // Process icons
    processIcons(this.popup);

    // Update citation counter (shows 000 if only initial citations)
    if (typeof this.updateCitationHeaderButton === 'function') {
      this.updateCitationHeaderButton();
    }

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
    this.closeCitationPopup();

    document.removeEventListener('keydown', this.handleKeyDown);

    if (this.wsClient) {
      this.wsClient.cleanup();
      this.wsClient = null;
    }

    this.overlay?.remove();
    this.popup?.remove();

    this.overlay = null;
    this.popup = null;
    this.conversation = null;
    this.input = null;
    this.sendBtn = null;
    this.currentStreamElement = null;
    this.conversationHistory = [];
    this.allCitations = [];
    this.citationIndex.clear();
    this.citationToRowMap.clear();
    this.citationHeaderBtn = null;
    this.citationCounterEl = null;
    crimsonInstance = null;

    log(Subsystems.CRIMSON, Status.INFO, 'Destroyed');
  }
}

// Constants available to all modules
CrimsonManager.STORAGE_KEYS = STORAGE_KEYS;
CrimsonManager.MAX_INPUT_HISTORY = MAX_INPUT_HISTORY;

// Initialize global shortcut on module load
initCrimsonShortcut();

/**
 * Crimson Events Module
 *
 * Adds drag and resize event handling to CrimsonManager:
 * - Main popup drag (header)
 * - Main popup resize (4 corners)
 * - Citation popup drag and resize
 * - Debug/reasoning panel resize (splitters)
 * - Send button and input handling
 *
 * @module managers/crimson/crimson-events
 */


// ==================== MAIN POPUP DRAG ====================

/**
 * Handle drag start on header
 */
CrimsonManager.prototype.handleDragStart = function(e) {
  // Allow dragging from the title/status area, but not from actionable header controls
  if (e.target.closest([
    '.crimson-header-close',
    '.crimson-reset-btn',
    '.crimson-debug-btn',
    '.crimson-streaming-btn',
    '.crimson-reasoning-btn',
    '.crimson-citation-header-btn',
  ].join(', '))) return;

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
};

/**
 * Handle drag move
 */
CrimsonManager.prototype.handleDragMove = function(e) {
  if (!this.isDragging) return;

  const deltaX = e.clientX - this.dragStartX;
  const deltaY = e.clientY - this.dragStartY;

  let newX = this.popupStartX + deltaX;
  let newY = this.popupStartY + deltaY;

  const rect = this.popup.getBoundingClientRect();
  const minVisible = 50;

  newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
  newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

  this.popup.style.left = `${newX}px`;
  this.popup.style.top = `${newY}px`;
};

/**
 * Handle drag end
 */
CrimsonManager.prototype.handleDragEnd = function() {
  this.isDragging = false;
  this.popup.classList.remove('dragging');
  this.saveMainWindowState();

  document.removeEventListener('mousemove', this.handleDragMove);
  document.removeEventListener('mouseup', this.handleDragEnd);
};

// ==================== MAIN POPUP RESIZE ====================

/**
 * Handle resize start
 * @param {MouseEvent} e - Mouse event
 * @param {string} corner - Which corner: 'br', 'bl', 'tr', 'tl'
 */
CrimsonManager.prototype.handleResizeStart = function(e, corner = 'br') {
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
};

/**
 * Handle resize move
 */
CrimsonManager.prototype.handleResizeMove = function(e) {
  if (!this.isResizing) return;

  const deltaX = e.clientX - this.resizeStartX;
  const deltaY = e.clientY - this.resizeStartY;

  let newWidth = this.popupStartWidth;
  let newHeight = this.popupStartHeight;
  let newLeft = this.popupStartLeft;
  let newTop = this.popupStartTop;

  switch (this.resizeCorner) {
    case 'br':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth + deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight + deltaY));
      break;
    case 'bl':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth - deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight + deltaY));
      if (newWidth !== this.popupStartWidth - deltaX) {
        const actualDeltaX = this.popupStartWidth - newWidth;
        newLeft = this.popupStartLeft + actualDeltaX;
      } else {
        newLeft = this.popupStartLeft + deltaX;
      }
      break;
    case 'tr':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.popupStartWidth + deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.popupStartHeight - deltaY));
      if (newHeight !== this.popupStartHeight - deltaY) {
        const actualDeltaY = this.popupStartHeight - newHeight;
        newTop = this.popupStartTop + actualDeltaY;
      } else {
        newTop = this.popupStartTop + deltaY;
      }
      break;
    case 'tl':
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

  this.popup.style.width = `${newWidth}px`;
  this.popup.style.height = `${newHeight}px`;

  if (this.resizeCorner === 'bl' || this.resizeCorner === 'tl') {
    this.popup.style.left = `${newLeft}px`;
  }
  if (this.resizeCorner === 'tr' || this.resizeCorner === 'tl') {
    this.popup.style.top = `${newTop}px`;
  }
};

/**
 * Handle resize end
 */
CrimsonManager.prototype.handleResizeEnd = function() {
  this.isResizing = false;
  this.popup.classList.remove('resizing');
  this.saveMainWindowState();

  document.removeEventListener('mousemove', this.handleResizeMove);
  document.removeEventListener('mouseup', this.handleResizeEnd);
};

// ==================== CITATION POPUP DRAG & RESIZE ====================

/**
 * Handle citation popup drag start
 */
CrimsonManager.prototype.handleCitationDragStart = function(e) {
  if (e.target.closest('.crimson-citation-popup-close')) return;

  this.isCitationDragging = true;
  this.activeCitationPopup.classList.add('dragging');

  this.citationDragStartX = e.clientX;
  this.citationDragStartY = e.clientY;

  const rect = this.activeCitationPopup.getBoundingClientRect();
  this.citationPopupStartX = rect.left;
  this.citationPopupStartY = rect.top;

  document.addEventListener('mousemove', this.handleCitationDragMove);
  document.addEventListener('mouseup', this.handleCitationDragEnd);

  e.preventDefault();
};

/**
 * Handle citation popup drag move
 */
CrimsonManager.prototype.handleCitationDragMove = function(e) {
  if (!this.isCitationDragging || !this.activeCitationPopup) return;

  const deltaX = e.clientX - this.citationDragStartX;
  const deltaY = e.clientY - this.citationDragStartY;

  let newX = this.citationPopupStartX + deltaX;
  let newY = this.citationPopupStartY + deltaY;

  const rect = this.activeCitationPopup.getBoundingClientRect();
  const minVisible = 50;

  newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
  newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

  this.activeCitationPopup.style.left = `${newX}px`;
  this.activeCitationPopup.style.top = `${newY}px`;
};

/**
 * Handle citation popup drag end
 */
CrimsonManager.prototype.handleCitationDragEnd = function() {
  this.isCitationDragging = false;
  if (this.activeCitationPopup) {
    this.activeCitationPopup.classList.remove('dragging');
    this.saveCitationPopupState(this.activeCitationPopup);
  }

  document.removeEventListener('mousemove', this.handleCitationDragMove);
  document.removeEventListener('mouseup', this.handleCitationDragEnd);
};

/**
 * Handle citation popup resize start
 * @param {MouseEvent} e - Mouse event
 * @param {string} corner - Which corner: 'br', 'bl', 'tr', 'tl'
 */
CrimsonManager.prototype.handleCitationResizeStart = function(e, corner = 'br') {
  this.isCitationResizing = true;
  this.citationResizeCorner = corner;
  if (this.activeCitationPopup) {
    this.activeCitationPopup.classList.add('resizing');
  }

  this.citationResizeStartX = e.clientX;
  this.citationResizeStartY = e.clientY;

  const rect = this.activeCitationPopup.getBoundingClientRect();
  this.citationPopupStartWidth = rect.width;
  this.citationPopupStartHeight = rect.height;
  this.citationPopupStartLeft = rect.left;
  this.citationPopupStartTop = rect.top;

  document.addEventListener('mousemove', this.handleCitationResizeMove);
  document.addEventListener('mouseup', this.handleCitationResizeEnd);

  e.preventDefault();
  e.stopPropagation();
};

/**
 * Handle citation popup resize move
 */
CrimsonManager.prototype.handleCitationResizeMove = function(e) {
  if (!this.isCitationResizing || !this.activeCitationPopup) return;

  const deltaX = e.clientX - this.citationResizeStartX;
  const deltaY = e.clientY - this.citationResizeStartY;

  let newWidth = this.citationPopupStartWidth;
  let newHeight = this.citationPopupStartHeight;
  let newLeft = this.citationPopupStartLeft;
  let newTop = this.citationPopupStartTop;

  switch (this.citationResizeCorner) {
    case 'br':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.citationPopupStartWidth + deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.citationPopupStartHeight + deltaY));
      break;
    case 'bl':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.citationPopupStartWidth - deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.citationPopupStartHeight + deltaY));
      if (newWidth !== this.citationPopupStartWidth - deltaX) {
        const actualDeltaX = this.citationPopupStartWidth - newWidth;
        newLeft = this.citationPopupStartLeft + actualDeltaX;
      } else {
        newLeft = this.citationPopupStartLeft + deltaX;
      }
      break;
    case 'tr':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.citationPopupStartWidth + deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.citationPopupStartHeight - deltaY));
      if (newHeight !== this.citationPopupStartHeight - deltaY) {
        const actualDeltaY = this.citationPopupStartHeight - newHeight;
        newTop = this.citationPopupStartTop + actualDeltaY;
      } else {
        newTop = this.citationPopupStartTop + deltaY;
      }
      break;
    case 'tl':
      newWidth = Math.max(400, Math.min(window.innerWidth * 0.95, this.citationPopupStartWidth - deltaX));
      newHeight = Math.max(300, Math.min(window.innerHeight * 0.95, this.citationPopupStartHeight - deltaY));
      if (newWidth !== this.citationPopupStartWidth - deltaX) {
        const actualDeltaX = this.citationPopupStartWidth - newWidth;
        newLeft = this.citationPopupStartLeft + actualDeltaX;
      } else {
        newLeft = this.citationPopupStartLeft + deltaX;
      }
      if (newHeight !== this.citationPopupStartHeight - deltaY) {
        const actualDeltaY = this.citationPopupStartHeight - newHeight;
        newTop = this.citationPopupStartTop + actualDeltaY;
      } else {
        newTop = this.citationPopupStartTop + deltaY;
      }
      break;
  }

  this.activeCitationPopup.style.width = `${newWidth}px`;
  this.activeCitationPopup.style.height = `${newHeight}px`;

  if (this.citationResizeCorner === 'bl' || this.citationResizeCorner === 'tl') {
    this.activeCitationPopup.style.left = `${newLeft}px`;
  }
  if (this.citationResizeCorner === 'tr' || this.citationResizeCorner === 'tl') {
    this.activeCitationPopup.style.top = `${newTop}px`;
  }
};

/**
 * Handle citation popup resize end
 */
CrimsonManager.prototype.handleCitationResizeEnd = function() {
  this.isCitationResizing = false;
  if (this.activeCitationPopup) {
    this.activeCitationPopup.classList.remove('resizing');
    this.saveCitationPopupState(this.activeCitationPopup);
  }

  document.removeEventListener('mousemove', this.handleCitationResizeMove);
  document.removeEventListener('mouseup', this.handleCitationResizeEnd);
};

// ==================== DEBUG PANEL RESIZE ====================

/**
 * Handle debug panel resize start
 */
CrimsonManager.prototype.handleDebugResizeStart = function(e) {
  this.isResizingDebug = true;
  this.debugResizeStartY = e.clientY;
  this.debugStartHeight = this.debugPanel?.offsetHeight || 150;

  this.debugPanel?.classList.add('no-transition');

  document.addEventListener('mousemove', this.handleDebugResizeMove);
  document.addEventListener('mouseup', this.handleDebugResizeEnd);
  e.preventDefault();
  e.stopPropagation();
};

/**
 * Handle debug panel resize move
 */
CrimsonManager.prototype.handleDebugResizeMove = function(e) {
  if (!this.isResizingDebug || !this.debugPanel) return;

  const deltaY = e.clientY - this.debugResizeStartY;
  const newHeight = Math.max(50, Math.min(400, this.debugStartHeight - deltaY));
  this.debugPanel.style.flex = `0 0 ${newHeight}px`;
};

/**
 * Handle debug panel resize end
 */
CrimsonManager.prototype.handleDebugResizeEnd = function() {
  this.isResizingDebug = false;
  this.debugPanel?.classList.remove('no-transition');
  document.removeEventListener('mousemove', this.handleDebugResizeMove);
  document.removeEventListener('mouseup', this.handleDebugResizeEnd);
};

// ==================== REASONING PANEL RESIZE ====================

/**
 * Handle reasoning panel resize start
 */
CrimsonManager.prototype.handleReasoningResizeStart = function(e) {
  this.isResizingReasoning = true;
  this.reasoningResizeStartY = e.clientY;
  this.reasoningStartHeight = this.reasoningPanel?.offsetHeight || 150;

  this.reasoningPanel?.classList.add('no-transition');

  document.addEventListener('mousemove', this.handleReasoningResizeMove);
  document.addEventListener('mouseup', this.handleReasoningResizeEnd);
  e.preventDefault();
  e.stopPropagation();
};

/**
 * Handle reasoning panel resize move
 */
CrimsonManager.prototype.handleReasoningResizeMove = function(e) {
  if (!this.isResizingReasoning || !this.reasoningPanel) return;

  const deltaY = e.clientY - this.reasoningResizeStartY;
  const newHeight = Math.max(50, Math.min(400, this.reasoningStartHeight + deltaY));
  this.reasoningPanel.style.flex = `0 0 ${newHeight}px`;
};

/**
 * Handle reasoning panel resize end
 */
CrimsonManager.prototype.handleReasoningResizeEnd = function() {
  this.isResizingReasoning = false;
  this.reasoningPanel?.classList.remove('no-transition');
  document.removeEventListener('mousemove', this.handleReasoningResizeMove);
  document.removeEventListener('mouseup', this.handleReasoningResizeEnd);
};

// ==================== SEND & INPUT HANDLING ====================

/**
 * Handle send button click
 */
CrimsonManager.prototype.handleSend = function() {
  const message = this.input.value.trim();
  if (!message || this.isStreaming) return;

  this.addMessage('user', message);
  this.addInputToHistory(message);

  this.input.value = '';
  this.autoResizeInput();

  this.sendChatMessage(message);
};

/**
 * Handle input keydown
 */
CrimsonManager.prototype.handleInputKeydown = function(e) {
  if (e.key === 'Enter' && !e.shiftKey) {
    e.preventDefault();
    this.handleSend();
    return;
  }

  if (e.key === 'ArrowUp' && (this.input.selectionStart === 0 || this.input.value === '')) {
    e.preventDefault();
    this.navigateInputHistory(-1);
    return;
  }

  if (e.key === 'ArrowDown' && (this.input.selectionStart === this.input.value.length || this.input.value === '')) {
    e.preventDefault();
    this.navigateInputHistory(1);
    return;
  }
};

/**
 * Auto-resize input based on content
 */
CrimsonManager.prototype.autoResizeInput = function() {
  this.input.style.height = 'auto';
  const newHeight = Math.min(150, Math.max(35, this.input.scrollHeight));
  this.input.style.height = `${newHeight}px`;
};

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

/**
 * Crimson WebSocket Chat Client
 *
 * Handles ONLY message submission and reception for chat.
 * Connection lifecycle is managed by app-ws.js.
 */


const DEFAULT_ENGINE = 'Crimson';
const CHAT_MESSAGE_TYPES = ['chat_chunk', 'chat_done', 'chat_error'];

class CrimsonWebSocket {
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

function getCrimsonWS(options = {}) {
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

  // Get previous conversation history (before adding new message)
  const history = this.conversationHistory.slice(-10);

  // Add new user message to conversation history
  this.conversationHistory.push({ role: 'user', content: message });

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
    const { marked } = await __vitePreload(async () => { const { marked } = await import('./marked.esm-CAHa9HS8.js');return { marked }},true              ?[]:void 0,import.meta.url);
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

/**
 * Crimson Citations Module
 *
 * Handles citation parsing, display, and interaction:
 * - Citation parsing and deduplication
 * - Citation popup with LithiumTable
 * - Canvas URL building
 * - Citation marker conversion
 * - Citation link handling
 *
 * @module managers/crimson/crimson-citations
 */


// ==================== CITATION PARSING ====================

/**
 * No deduplication - citations are added as they come.
 * @param {Object} _citation - Raw or normalized citation (unused)
 * @param {number} index - Citation index for unique key
 * @returns {string} Unique key
 */
CrimsonManager.prototype._citationDedup = function(_citation, index) {
  return `citation_${index}_${Date.now()}`;
};

/**
 * Parse citations from metadata, deduplicate, and collect into allCitations.
 * Returns a mapping of original per-turn indices to global citation numbers.
 * @param {string} messageId - The message element ID
 * @param {Array} citations - Array of citation objects from metadata
 * @returns {Map<number,number>} turnIndex (1-based) to global citation number
 */
CrimsonManager.prototype.parseCitations = function(messageId, citations) {
  log(Subsystems.CRIMSON, Status.DEBUG, `parseCitations called for ${messageId}: ${Array.isArray(citations) ? citations.length : 'invalid'} items`);
  if (!citations || !Array.isArray(citations) || citations.length === 0) {
    log(Subsystems.CRIMSON, Status.DEBUG, 'parseCitations: no citations to process');
    return new Map();
  }

  const turnMapping = new Map();
  const startIndex = this.allCitations.length;

  citations.forEach((citation, index) => {
    log(Subsystems.CRIMSON, Status.DEBUG, `Raw citation[${index}]: ${JSON.stringify(citation)}`);

    let filename = citation.filename || citation.name || citation.title || '';
    let url = citation.url || citation.uri || citation.source || citation.filename || '';
    const isCanvas = url.startsWith('canvas') || filename.startsWith('canvas');
    const turnIndex = index + 1;
    const globalNumber = startIndex + turnIndex;

    // Handle canvas- prefixed URLs - convert to actual course page URL
    if (url.startsWith('canvas-')) {
      const canvasUrl = this.buildCanvasUrl(url);
      if (canvasUrl) {
        url = canvasUrl;
      }
    }

    // Clean up filename: if it starts with canvas-PH-*, remove prefix and clean up
    if (filename.startsWith('canvas-PH-')) {
      // Remove the prefix up to the module number (e.g., "canvas-PH-003-I2L-mod-2-")
      const match = filename.match(/^canvas-PH-\d+-I2L-mod-\d+-(.+)$/);
      if (match) {
        let cleaned = match[1];
        // Remove .md extension
        if (cleaned.endsWith('.md')) {
          cleaned = cleaned.slice(0, -3);
        }
        // Replace dashes with spaces and title case
        filename = cleaned.replace(/-/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
      }
    }

    // Generate link icon based on processed URL
    const link = this.generateLinkIcon(url);

    const normalized = {
      number: globalNumber,
      name: filename || 'Untitled',
      url: url,
      link: link,
      type: isCanvas ? 'canvas' : (citation.type || 'web'),
      source: citation.source || citation.filename || '',
      score: citation.score != null 
        ? Math.max(-1e3, Math.min(1000, citation.score)) 
        : null,
      pageContent: citation.page_content || null,
      dataSourceId: citation.data_source_id || null,
    };

    this.allCitations.push(normalized);
    turnMapping.set(turnIndex, globalNumber);
  });

  this.updateCitationHeaderButton();

  log(Subsystems.CRIMSON, Status.DEBUG, `Citations collected: ${this.allCitations.length} total, ${citations.length} from this turn`);
  return turnMapping;
};

/**
 * Build a canvas URL from a citation filename.
 * @param {string} filename - The citation filename
 * @returns {string|null} The constructed URL or null
 */
CrimsonManager.prototype.buildCanvasUrl = function(filename) {
  if (!filename) return null;

  if (filename.includes('/')) {
    const parts = filename.split('/');
    const courseCodeIndex = parts.indexOf('courses') + 1;
    if (courseCodeIndex > 0 && courseCodeIndex < parts.length) {
      const pageName = parts[parts.length - 1];
      const courseId = '801';
      return `https://canvas.500courses.com/courses/${courseId}/pages/${pageName}`;
    }
  }

  if (filename.startsWith('canvas')) {
    const cleanFilename = filename.endsWith('.md') ? filename.slice(0, -3) : filename;

    const modMatch = cleanFilename.match(/^canvas-[^-]+-[^-]+-[^-]+-mod-(\d+)-(.+)$/);
    if (modMatch) {
      const pageName = modMatch[2];
      const courseId = '801';
      return `https://canvas.500courses.com/courses/${courseId}/pages/${pageName}`;
    }

    const parts = cleanFilename.split('-');
    if (parts.length >= 5) {
      const modIndex = parts.indexOf('mod');
      if (modIndex > 0 && modIndex < parts.length - 2) {
        const pageName = parts.slice(modIndex + 2).join('-');
        const courseId = '801';
        return `https://canvas.500courses.com/courses/${courseId}/pages/${pageName}`;
      }
    }
  }

  return null;
};

/**
 * Generate link icon based on URL.
 * @param {string} url - The URL to analyze
 * @returns {string} The FA icon HTML
 */
CrimsonManager.prototype.generateLinkIcon = function(url) {
  if (!url) return '<fa fa-link></fa>';
  
  // GitHub
  if (url.includes('github.com')) {
    return '<i class="fa-brands fa-github"></i>';
  }
  
  // Markdown (.md file)
  if (!url.startsWith('http') && url.endsWith('.md')) {
    return '<i class="fa-brands fa-markdown"></i>';
  }
  
  // JSON (.json file)
  if (url.endsWith('.json')) {
    return '<fa fa-brackets-curly></fa>';
  }
  
  // Canvas (check both canvas- prefix and canvas.500courses.com domain)
  if (url.startsWith('canvas') || url.includes('canvas/') || url.includes('canvas.500courses.com')) {
    return '<fa fa-graduation-cap></fa>';
  }
  
  // Generic web
  return '<fa fa-arrow-up-right-from-square></fa>';
};

/**
 * Open a citation URL in a new tab, translating canvas URLs if needed
 * @param {string} url - The URL to open
 */
CrimsonManager.prototype.openCitationUrl = function(url) {
  if (!url) return;
  if (url.startsWith('canvas') || url.includes('canvas/')) {
    const canvasUrl = this.buildCanvasUrl(url);
    if (canvasUrl) {
      window.open(canvasUrl, '_blank', 'noopener,noreferrer');
    } else {
      log(Subsystems.CRIMSON, Status.WARN, `Failed to build canvas URL for: ${url}`);
    }
  } else {
    window.open(url, '_blank', 'noopener,noreferrer');
  }
};

/**
 * Format a URL for display as a Reference.
 * @param {string} url - The URL to format
 * @returns {string} Formatted reference string
 */
CrimsonManager.prototype.formatReference = function(url) {
  if (!url) return '';
  return url;
};

// ==================== CITATION HEADER BUTTON ====================

/**
 * Update the citation count in the header button
 */
CrimsonManager.prototype.updateCitationHeaderButton = function() {
  const count = this.allCitations.length;

  if (this.citationCounterEl) {
    this.citationCounterEl.textContent = count > 0 ? String(count).padStart(3, '0') : '000';
  }
};

/**
 * Toggle citation popup from the header button
 */
CrimsonManager.prototype.toggleCitationPopup = function() {
  if (this.activeCitationPopup) {
    this.closeCitationPopup();
    return;
  }

  this.showCitationPopup(this.citationHeaderBtn, this.allCitations);
};

/**
 * Load citation popup position and size from localStorage
 * @returns {Object|null} Saved position/size or null
 */
CrimsonManager.prototype.loadCitationPopupState = function() {
  try {
    const x = localStorage.getItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_X);
    const y = localStorage.getItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_Y);
    const width = localStorage.getItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_WIDTH);
    const height = localStorage.getItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_HEIGHT);
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
};

/**
 * Save citation popup position and size to localStorage
 * @param {HTMLElement} popup - The popup element
 */
CrimsonManager.prototype.saveCitationPopupState = function(popup) {
  try {
    if (!popup) return;
    const rect = popup.getBoundingClientRect();
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_X, String(rect.left));
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_Y, String(rect.top));
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_WIDTH, String(rect.width));
    localStorage.setItem(CrimsonManager.STORAGE_KEYS.CITATION_POPUP_HEIGHT, String(rect.height));
  } catch (e) {
    // localStorage may not be available
  }
};

/**
 * Show citation popup anchored to a button, using a LithiumTable for the list.
 * @param {HTMLElement} button - The anchor button
 * @param {Array} citations - Array of normalized citation objects
 */
CrimsonManager.prototype.showCitationPopup = async function(button, citations) {
  const popup = document.createElement('div');
  popup.className = 'crimson-citation-popup';
  this._citationPopupEl = popup;

  popup.innerHTML = `
    <div class="crimson-citation-resize-handle crimson-citation-resize-handle-tl" data-tooltip="Resize"></div>
    <div class="crimson-citation-resize-handle crimson-citation-resize-handle-tr" data-tooltip="Resize"></div>
    <div class="crimson-citation-popup-header">
      <span class="crimson-citation-popup-title">Citations</span>
      <button type="button" class="crimson-citation-popup-close" title="Close (ESC)">
        <fa fa-xmark></fa>
      </button>
    </div>
    <div class="crimson-citation-popup-content">
      <div class="crimson-citation-table-wrap lithium-table-container"></div>
      <div class="crimson-citation-nav-wrap lithium-navigator-container"></div>
    </div>
    <div class="crimson-citation-popup-arrow"></div>
    <div class="crimson-citation-resize-handle crimson-citation-resize-handle-bl" data-tooltip="Resize"></div>
    <div class="crimson-citation-resize-handle crimson-citation-resize-handle-br" data-tooltip="Resize"></div>
  `;

  document.body.appendChild(popup);
  processIcons(popup);

  const header = popup.querySelector('.crimson-citation-popup-header');
  const resizeHandleBR = popup.querySelector('.crimson-citation-resize-handle-br');
  const resizeHandleBL = popup.querySelector('.crimson-citation-resize-handle-bl');
  const resizeHandleTR = popup.querySelector('.crimson-citation-resize-handle-tr');
  const resizeHandleTL = popup.querySelector('.crimson-citation-resize-handle-tl');

  header?.addEventListener('mousedown', this.handleCitationDragStart);
  resizeHandleBR?.addEventListener('mousedown', (e) => this.handleCitationResizeStart(e, 'br'));
  resizeHandleBL?.addEventListener('mousedown', (e) => this.handleCitationResizeStart(e, 'bl'));
  resizeHandleTR?.addEventListener('mousedown', (e) => this.handleCitationResizeStart(e, 'tr'));
  resizeHandleTL?.addEventListener('mousedown', (e) => this.handleCitationResizeStart(e, 'tl'));

  const savedState = this.loadCitationPopupState();
  if (savedState) {
    popup.style.left = `${savedState.x}px`;
    popup.style.top = `${savedState.y}px`;
    popup.style.width = `${savedState.width}px`;
    popup.style.height = `${savedState.height}px`;
  } else {
    this.positionCitationPopup(popup, button);
  }

  const closeBtn = popup.querySelector('.crimson-citation-popup-close');
  closeBtn?.addEventListener('click', () => this.closeCitationPopup());

  this.activeCitationPopup = popup;
  if (button) button.classList.add('active');

  await new Promise(resolve => {
    requestAnimationFrame(() => {
      popup.classList.add('visible');
      setTimeout(resolve, 50);
    });
  });

  const tableContainer = popup.querySelector('.crimson-citation-table-wrap');
  const navContainer = popup.querySelector('.crimson-citation-nav-wrap');

  const app = window.lithiumApp || null;

  this.citationTable = new LithiumTable({
    container: tableContainer,
    navigatorContainer: navContainer,
    app: app,
    readonly: true,
    cssPrefix: 'lithium',
    storageKey: 'crimson_citations',
    tablePath: 'crimson/citations',
    lookupKeyIdx: 13,
    onRowSelected: (rowData) => {
      log(Subsystems.CRIMSON, Status.DEBUG, `Citation selected: #${rowData.number} ${rowData.name}`);
    },
    onDataLoaded: (rows) => {
      log(Subsystems.CRIMSON, Status.DEBUG, `Citations loaded: ${rows.length} rows`);
    },
  });

  await this.citationTable.init();

  this.citationTable.loadStaticData(citations, { autoSelect: false });

  this.citationTable.table.on('rowDblClick', (_e, row) => {
    const rowData = row.getData();
    // Only open URLs that start with http
    if (rowData?.url && rowData.url.startsWith('http')) {
      this.openCitationUrl(rowData.url);
    }
  });

  const urlCol = this.citationTable.table.getColumn('url');
  if (urlCol) {
    urlCol.update({
      formatter: (cell) => this.formatReference(cell.getValue()),
    });
  }

  const scoreCol = this.citationTable.table.getColumn('score');
  if (scoreCol) {
    scoreCol.update({
      formatter: (cell) => {
        const score = cell.getValue();
        if (score === null || score === undefined) return '';
        return score.toLocaleString();
      },
    });
  }

  const linkCol = this.citationTable.table.getColumn('link');
  if (linkCol) {
    linkCol.update({
      headerSort: false,
      formatter: (cell) => {
        const url = cell.getValue();
        if (!url) return '';
        if (url.startsWith('canvas') || url.includes('canvas/')) {
          return '<fa fa-graduation-cap fa-flip-horizontal></fa>';
        }
        return '<fa fa-arrow-up-right-from-square></fa>';
      },
      cellClick: (_e, cell) => {
        const url = cell.getValue();
        // Only open URLs that start with http
        if (url && url.startsWith('http')) {
          this.openCitationUrl(url);
        }
      },
    });
  }

  this._citationOutsideHandler = (e) => {
    if (!popup.contains(e.target) && !(button && button.contains(e.target))) {
      this.closeCitationPopup();
    }
  };
  document.addEventListener('click', this._citationOutsideHandler);

  log(Subsystems.CRIMSON, Status.DEBUG, `Opened citation popup with ${citations.length} citations`);
};

/**
 * Position citation popup relative to button.
 * @param {HTMLElement} popup - The popup element
 * @param {HTMLElement} button - The anchor button
 */
CrimsonManager.prototype.positionCitationPopup = function(popup, button) {
  const buttonRect = button.getBoundingClientRect();
  const popupWidth = 500;

  let left = buttonRect.left;
  let top = buttonRect.bottom + 6;

  if (left + popupWidth > window.innerWidth - 20) {
    left = window.innerWidth - popupWidth - 20;
  }
  if (left < 10) left = 10;

  if (top + 400 > window.innerHeight - 20) {
    top = buttonRect.top - 400 - 6;
  }
  if (top < 10) top = 10;

  popup.style.left = `${left}px`;
  popup.style.top = `${top}px`;

  const arrow = popup.querySelector('.crimson-citation-popup-arrow');
  if (arrow) {
    const arrowLeft = buttonRect.left + buttonRect.width / 2 - left;
    arrow.style.left = `${Math.max(10, Math.min(arrowLeft, popupWidth - 10))}px`;

    if (top > buttonRect.bottom) {
      arrow.classList.add('arrow-top');
    } else {
      arrow.classList.add('arrow-bottom');
    }
  }
};

/**
 * Close citation popup
 */
CrimsonManager.prototype.closeCitationPopup = function() {
  if (this._citationOutsideHandler) {
    document.removeEventListener('click', this._citationOutsideHandler);
    this._citationOutsideHandler = null;
  }

  this.handleCitationDragEnd();
  this.handleCitationResizeEnd();

  if (this.citationTable) {
    this.citationTable.destroy();
    this.citationTable = null;
  }

  if (this.activeCitationPopup) {
    this.activeCitationPopup.classList.remove('visible');
    setTimeout(() => {
      this.activeCitationPopup?.remove();
      this.activeCitationPopup = null;
      this._citationPopupEl = null;
    }, 200);
  }

  if (this.citationHeaderBtn) {
    this.citationHeaderBtn.classList.remove('active');
  }
};

/**
 * Highlight a specific citation in the popup via LithiumTable row selection
 * @param {number} citationNumber - The global citation number to highlight
 */
CrimsonManager.prototype.highlightCitation = function(citationNumber) {
  if (!this.citationTable?.table) return;

  this.citationTable.table.deselectRow();

  const rows = this.citationTable.table.getRows();
  for (const row of rows) {
    const data = row.getData();
    if (data.number === citationNumber) {
      row.select();
      row.scrollTo();
      break;
    }
  }
};

// ==================== CITATION MARKERS ====================

/**
 * Convert citation markers [[C1]] in text to clickable icons.
 * @param {string} content - The message content (HTML)
 * @param {string} messageId - The message element ID
 * @param {Map<number,number>} turnMapping - Per-turn index to global number mapping
 * @returns {string} Content with citation links
 */
CrimsonManager.prototype.convertCitationMarkers = function(content, messageId, turnMapping) {
  if (!content) return '';

  return content.replace(/\[\[C(\d+)\]\]/g, (match, num) => {
    const turnIndex = parseInt(num, 10);
    const globalNum = (turnMapping && turnMapping.has(turnIndex))
      ? turnMapping.get(turnIndex)
      : turnIndex;
    const citation = this.allCitations.find(c => c.number === globalNum);
    let tooltipText;
    if (citation?.name && citation.name !== 'Untitled') {
      tooltipText = citation.name;
    } else if (citation?.url) {
      tooltipText = this.formatReference(citation.url);
    } else {
      tooltipText = `Citation ${globalNum}`;
    }
    return `<a href="#" class="crimson-citation-link" data-citation="${globalNum}" title="${this.escapeHtml(tooltipText)}"><fa fa-book-open-lines class="crimson-citation-link-icon"></fa></a>`;
  });
};

/**
 * Handle citation link click (delegated from conversation container)
 * @param {Event} e - Click event
 */
CrimsonManager.prototype.handleCitationLinkClick = function(e) {
  const link = e.target.closest('.crimson-citation-link');
  if (!link) return;

  e.preventDefault();
  e.stopPropagation();

  const citationNum = parseInt(link.getAttribute('data-citation'), 10);
  if (!citationNum || this.allCitations.length === 0) return;

  if (!this.activeCitationPopup) {
    this.showCitationPopup(this.citationHeaderBtn, this.allCitations).then(() => {
      setTimeout(() => this.highlightCitation(citationNum), 100);
    });
  } else {
    this.highlightCitation(citationNum);
  }
};

export { CrimsonManager, getCrimson, initCrimsonShortcut, toggleCrimson };
