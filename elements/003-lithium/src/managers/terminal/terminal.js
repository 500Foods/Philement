/**
 * Terminal Manager
 *
 * Utility Manager — a draggable, resizable popup window containing an iframe.
 * Displays a web URL in an iframe with a configurable source.
 *
 * Features:
 * - Draggable header
 * - Resizable (4 corner handles)
 * - Centered initially at 70% viewport
 * - Fullscreen toggle
 * - ESC to close
 */

import { processIcons } from '../../core/icons.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { initTooltips } from '../../core/tooltip-api.js';
import { getConfigValue } from '../../core/config.js';
import { retrieveJWT } from '../../core/jwt.js';

// Import CSS
import './terminal.css';

// Singleton instance tracking
let terminalInstance = null;

// Hydrogen terminal WebPath default (from config_terminal.c)
const HYDROGEN_TERMINAL_PATH = '/terminal';

// LocalStorage keys for persistent state
const STORAGE_KEYS = {
  TERMINAL_X: 'lithium_terminal_x',
  TERMINAL_Y: 'lithium_terminal_y',
  TERMINAL_WIDTH: 'lithium_terminal_width',
  TERMINAL_HEIGHT: 'lithium_terminal_height',
  TERMINAL_FULLSCREEN: 'lithium_terminal_fullscreen',
};

/**
 * Get the singleton Terminal instance (creates if needed)
 * @returns {TerminalManager} The Terminal manager instance
 */
export function getTerminal() {
  if (!terminalInstance) {
    terminalInstance = new TerminalManager();
  }
  return terminalInstance;
}

/**
 * Show/activate the Terminal popup (convenience function)
 * @param {Object} options - Options for showing Terminal
 * @param {string} options.url - URL to display in iframe
 */
export function showTerminal(options = {}) {
  const terminal = getTerminal();
  terminal.show(options);
}

/**
 * Hide/deactivate the Terminal popup (convenience function)
 */
export function hideTerminal() {
  if (terminalInstance) {
    terminalInstance.hide();
  }
}

/**
 * Toggle the Terminal popup (convenience function)
 * @param {Object} options - Options for showing Terminal if opening
 */
export function toggleTerminal(options = {}) {
  const terminal = getTerminal();
  if (terminal.isVisible) {
    terminal.hide();
  } else {
    terminal.show(options);
  }
}

/**
 * Create a Terminal button for manager slot headers
 * @param {string} [tooltip] - Custom tooltip text
 * @returns {HTMLButtonElement} The configured button element
 */
export function createTerminalButton(tooltip = 'Terminal') {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-terminal-btn';
  button.setAttribute('data-tooltip', tooltip);
  button.innerHTML = '<fa fa-square-terminal></fa>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    // Import and toggle terminal popup
    import('./terminal.js').then(({ toggleTerminal }) => {
      toggleTerminal();
    });
  });

  requestAnimationFrame(() => {
    processIcons(button);
  });

  return button;
}

/**
 * Terminal Manager Class
 */
export class TerminalManager {
  constructor() {
    this.popup = null;
    this.overlay = null;
    this.iframe = null;
    this.isVisible = false;
    this.isDragging = false;
    this.isResizing = false;
    this.isFullscreen = false;
    this.previousRect = null;
    this._isInitialized = false;

    // Drag state
    this.dragStartX = 0;
    this.dragStartY = 0;
    this.popupStartX = 0;
    this.popupStartY = 0;

    // Resize state
    this.resizeStartX = 0;
    this.resizeStartY = 0;
    this.popupStartWidth = 0;
    this.popupStartHeight = 0;
    this.popupStartLeft = 0;
    this.popupStartTop = 0;
    this.resizeCorner = 'br';

    // Bind event handlers
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleOverlayClick = this.handleOverlayClick.bind(this);
    this.handleDragStart = this.handleDragStart.bind(this);
    this.handleDragMove = this.handleDragMove.bind(this);
    this.handleDragEnd = this.handleDragEnd.bind(this);
    this.handleResizeStart = this.handleResizeStart.bind(this);
    this.handleResizeMove = this.handleResizeMove.bind(this);
    this.handleResizeEnd = this.handleResizeEnd.bind(this);
    this._handleIframeMessage = this._handleIframeMessage.bind(this);
  }

  /**
   * Build the Hydrogen terminal iframe URL from config.
   *
   * Hydrogen serves the terminal at `server.url` + `server.terminal_path`,
   * where `terminal_path` defaults to `/terminal` (matching Hydrogen's
   * `TerminalConfig.WebPath` default from config_terminal.c). If config is
   * absent or the server URL cannot be resolved, a warning is logged and
   * the Hydrogen default path is used so the iframe never silently points
   * at an unrelated host.
   *
   * @returns {string} Fully-qualified terminal URL for the iframe src
   */
  get terminalUrl() {
    const serverUrl = getConfigValue('server.url');
    const terminalPath = getConfigValue('server.terminal_path', HYDROGEN_TERMINAL_PATH);

    if (!serverUrl) {
      log(Subsystems.MANAGER, Status.WARN, '[Terminal] No server.url configured; using Hydrogen default terminal path only.');
      return terminalPath;
    }

    const base = serverUrl.replace(/\/+$/, '');
    const path = terminalPath.startsWith('/') ? terminalPath : `/${terminalPath}`;
    return `${base}${path}`;
  }

  /**
   * Initialize the Terminal popup (create DOM elements)
   */
  init() {
    if (this._isInitialized) return;

    // Create overlay
    this.overlay = document.createElement('div');
    this.overlay.className = 'terminal-overlay';
    this.overlay.addEventListener('click', this.handleOverlayClick);
    document.body.appendChild(this.overlay);

    // Create popup with proper initial sizing
    this.popup = document.createElement('div');
    this.popup.className = 'terminal-popup';
    
    // Calculate initial 70% size
    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    const width = Math.round(viewportWidth * 0.7);
    const height = Math.round(viewportHeight * 0.7);
    const left = Math.round((viewportWidth - width) / 2);
    const top = Math.round((viewportHeight - height) / 2);
    
    // Set initial position and size via inline styles
    this.popup.style.width = `${width}px`;
    this.popup.style.height = `${height}px`;
    this.popup.style.left = `${left}px`;
    this.popup.style.top = `${top}px`;
    
    this.popup.innerHTML = `
      <div class="terminal-resize-handle terminal-resize-handle-tl" data-tooltip="Resize"></div>
      <div class="terminal-resize-handle terminal-resize-handle-tr" data-tooltip="Resize"></div>
      <div class="terminal-header">
        <div class="subpanel-header-group">
          <button type="button" class="terminal-header-primary">
            <fa fa-square-terminal></fa>
            <span>Terminal</span>
          </button>
          <button type="button" class="terminal-header-placeholder" disabled></button>
          <button type="button" class="terminal-header-minimize" data-tooltip="Minimize">
            <fa fa-minus></fa>
          </button>
          <button type="button" class="terminal-fullscreen-btn" data-tooltip="Toggle Fullscreen">
            <fa fa-maximize></fa>
          </button>
          <button type="button" class="terminal-header-close" data-tooltip="Close Session (ESC)">
            <fa fa-xmark></fa>
          </button>
        </div>
      </div>
      <div class="terminal-content">
        <iframe class="terminal-iframe" src="${this.terminalUrl}" allow="fullscreen"></iframe>
        <div class="terminal-resize-overlay"></div>
      </div>
      <div class="terminal-resize-handle terminal-resize-handle-bl" data-tooltip="Resize"></div>
      <div class="terminal-resize-handle terminal-resize-handle-br" data-tooltip="Resize"></div>
    `;

    document.body.appendChild(this.popup);
    initTooltips(this.popup);

    // Cache element references
    this.iframe = this.popup.querySelector('.terminal-iframe');
    this.fullscreenBtn = this.popup.querySelector('.terminal-fullscreen-btn');
    this.minimizeBtn = this.popup.querySelector('.terminal-header-minimize');
    this.closeBtn = this.popup.querySelector('.terminal-header-close');
    this.resizeOverlay = this.popup.querySelector('.terminal-resize-overlay');

    // Listen for terminal-config-request from the iframe
    window.addEventListener('message', this._handleIframeMessage);

    // When the iframe finishes loading, we don't need to push — the iframe
    // will request config via postMessage itself.

    // Wire events
    const closeBtn = this.closeBtn;
    const minimizeBtn = this.minimizeBtn;
    const header = this.popup.querySelector('.terminal-header');

    closeBtn.addEventListener('click', () => this.closeSession());
    minimizeBtn.addEventListener('click', () => this.hide());
    this.fullscreenBtn?.addEventListener('click', () => this.toggleFullscreen());

    // Drag handlers
    header?.addEventListener('mousedown', (e) => this.handleDragStart(e));

    // Resize handlers
    this.popup.querySelector('.terminal-resize-handle-br')?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'br'));
    this.popup.querySelector('.terminal-resize-handle-bl')?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'bl'));
    this.popup.querySelector('.terminal-resize-handle-tr')?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'tr'));
    this.popup.querySelector('.terminal-resize-handle-tl')?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'tl'));

    // Process icons
    processIcons(this.popup);

    this._isInitialized = true;
    log(Subsystems.MANAGER, Status.INFO, '[Terminal] Initialized');
  }

  /**
   * Load window position and size from localStorage
   * @returns {Object|null} Saved position/size or null
   */
  loadWindowState() {
    try {
      const x = localStorage.getItem(STORAGE_KEYS.TERMINAL_X);
      const y = localStorage.getItem(STORAGE_KEYS.TERMINAL_Y);
      const width = localStorage.getItem(STORAGE_KEYS.TERMINAL_WIDTH);
      const height = localStorage.getItem(STORAGE_KEYS.TERMINAL_HEIGHT);
      const fullscreen = localStorage.getItem(STORAGE_KEYS.TERMINAL_FULLSCREEN);
      
      if (fullscreen === 'true') {
        this.isFullscreen = true;
        return { fullscreen: true };
      }
      
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
   * Save window position and size to localStorage
   */
  saveWindowState() {
    try {
      if (!this.popup) return;
      
      if (this.isFullscreen) {
        localStorage.setItem(STORAGE_KEYS.TERMINAL_FULLSCREEN, 'true');
        return;
      }
      
      const rect = this.popup.getBoundingClientRect();
      localStorage.setItem(STORAGE_KEYS.TERMINAL_FULLSCREEN, 'false');
      localStorage.setItem(STORAGE_KEYS.TERMINAL_X, String(Math.round(rect.left)));
      localStorage.setItem(STORAGE_KEYS.TERMINAL_Y, String(Math.round(rect.top)));
      localStorage.setItem(STORAGE_KEYS.TERMINAL_WIDTH, String(Math.round(rect.width)));
      localStorage.setItem(STORAGE_KEYS.TERMINAL_HEIGHT, String(Math.round(rect.height)));
    } catch (e) {
      // localStorage may not be available
    }
  }

  /**
   * Toggle fullscreen mode
   */
  toggleFullscreen() {
    if (!this.popup) return;

    if (this.isFullscreen) {
      // Restore previous size and position
      this.isFullscreen = false;
      this.popup.classList.remove('terminal-fullscreen');
      
      if (this.previousRect) {
        this.popup.style.left = `${this.previousRect.left}px`;
        this.popup.style.top = `${this.previousRect.top}px`;
        this.popup.style.width = `${this.previousRect.width}px`;
        this.popup.style.height = `${this.previousRect.height}px`;
      } else {
        // Recenter if no previous rect
        const viewportWidth = window.innerWidth;
        const viewportHeight = window.innerHeight;
        const width = Math.round(viewportWidth * 0.7);
        const height = Math.round(viewportHeight * 0.7);
        this.popup.style.width = `${width}px`;
        this.popup.style.height = `${height}px`;
        this.popup.style.left = `${Math.round((viewportWidth - width) / 2)}px`;
        this.popup.style.top = `${Math.round((viewportHeight - height) / 2)}px`;
      }
      
      // Update fullscreen button icon
      if (this.fullscreenBtn) {
        this.fullscreenBtn.innerHTML = '<fa fa-maximize></fa>';
        processIcons(this.fullscreenBtn);
      }
    } else {
      // Store current rect before going fullscreen
      const rect = this.popup.getBoundingClientRect();
      this.previousRect = {
        left: rect.left,
        top: rect.top,
        width: rect.width,
        height: rect.height,
      };
      
      // Go fullscreen
      this.isFullscreen = true;
      this.popup.classList.add('terminal-fullscreen');
      
      // Update fullscreen button icon
      if (this.fullscreenBtn) {
        this.fullscreenBtn.innerHTML = '<fa fa-minimize></fa>';
        processIcons(this.fullscreenBtn);
      }
    }
    
    this.saveWindowState();
  }

  /**
   * Show the Terminal popup
   * @param {Object} options - Show options
   * @param {string} options.url - URL to display in iframe
   */
  show(options = {}) {
    if (!this._isInitialized) {
      this.init();
    }

    if (this.isVisible) {
      // Update URL if provided even when already visible
      if (options.url && this.iframe) {
        this.iframe.src = options.url;
      }
      return;
    }

    // Update URL if provided
    if (options.url && this.iframe) {
      this.iframe.src = options.url;
    }

    // Apply saved state or use defaults
    const savedState = this.loadWindowState();
    if (savedState) {
      if (savedState.fullscreen) {
        this.isFullscreen = true;
        this.popup.classList.add('terminal-fullscreen');
        if (this.fullscreenBtn) {
          this.fullscreenBtn.innerHTML = '<fa fa-minimize></fa>';
          processIcons(this.fullscreenBtn);
        }
      } else {
        this.popup.style.left = `${savedState.x}px`;
        this.popup.style.top = `${savedState.y}px`;
        this.popup.style.width = `${savedState.width}px`;
        this.popup.style.height = `${savedState.height}px`;
        this.popup.classList.remove('terminal-fullscreen');
        this.isFullscreen = false;
        if (this.fullscreenBtn) {
          this.fullscreenBtn.innerHTML = '<fa fa-maximize></fa>';
          processIcons(this.fullscreenBtn);
        }
      }
    }

    // Show the popup
    this.overlay.classList.add('visible');
    this.popup.classList.add('visible');
    this.isVisible = true;

    document.addEventListener('keydown', this.handleKeyDown);

    log(Subsystems.MANAGER, Status.INFO, '[Terminal] Shown');
  }

  /**
   * Hide the Terminal popup
   */
  hide() {
    if (!this.isVisible) return;

    this.saveWindowState();

    this.overlay.classList.remove('visible');
    this.popup.classList.remove('visible');
    this.isVisible = false;

    document.removeEventListener('keydown', this.handleKeyDown);

    log(Subsystems.MANAGER, Status.INFO, '[Terminal] Hidden');
  }

  /**
   * Close the Terminal session entirely — destroys the iframe and listener so
   * that reopening creates a fresh popup that re-requests the terminal key.
   * Use this instead of hide() when the WebSocket auth has failed or a new
   * key is needed (e.g. after a Hydrogen restart).
   */
  closeSession() {
    if (!this.isVisible) return;
    this.hide();
    this.destroy();
    log(Subsystems.MANAGER, Status.INFO, '[Terminal] Session closed');
  }

  /**
   * Handle keyboard events (ESC to close session, F11 for fullscreen)
   */
  handleKeyDown(e) {
    if (e.key === 'Escape') {
      e.preventDefault();
      this.closeSession();
    } else if (e.key === 'F11') {
      e.preventDefault();
      this.toggleFullscreen();
    }
  }

  /**
   * Handle overlay click (click outside to close session)
   */
  handleOverlayClick(e) {
    if (e.target === this.overlay) {
      this.closeSession();
    }
  }

  /**
   * Handle drag start
   */
  handleDragStart(e) {
    if (e.target.closest([
      '.terminal-header-close',
      '.terminal-header-minimize',
      '.terminal-fullscreen-btn',
    ].join(', '))) return;

    this.isDragging = true;
    this.dragStartX = e.clientX;
    this.dragStartY = e.clientY;

    const rect = this.popup.getBoundingClientRect();
    this.popupStartX = rect.left;
    this.popupStartY = rect.top;

    this.popup.classList.add('dragging');

    // Show overlay to prevent iframe from stealing mouse events
    if (this.resizeOverlay) {
      this.resizeOverlay.style.display = 'block';
    }

    document.addEventListener('mousemove', this.handleDragMove);
    document.addEventListener('mouseup', this.handleDragEnd);

    e.preventDefault();
  }

  /**
   * Handle drag move
   */
  handleDragMove(e) {
    if (!this.isDragging) return;

    const dx = e.clientX - this.dragStartX;
    const dy = e.clientY - this.dragStartY;

    let newX = this.popupStartX + dx;
    let newY = this.popupStartY + dy;

    // Keep some part of the popup visible
    const rect = this.popup.getBoundingClientRect();
    const minVisible = 50;
    
    newX = Math.max(minVisible - rect.width, Math.min(newX, window.innerWidth - minVisible));
    newY = Math.max(0, Math.min(newY, window.innerHeight - minVisible));

    this.popup.style.left = `${newX}px`;
    this.popup.style.top = `${newY}px`;
  }

  /**
   * Handle drag end
   */
  handleDragEnd() {
    this.isDragging = false;
    this.popup.classList.remove('dragging');

    // Hide overlay
    if (this.resizeOverlay) {
      this.resizeOverlay.style.display = 'none';
    }

    document.removeEventListener('mousemove', this.handleDragMove);
    document.removeEventListener('mouseup', this.handleDragEnd);
    this.saveWindowState();
  }

  /**
   * Handle resize start
   */
  handleResizeStart(e, corner) {
    e.preventDefault();
    e.stopPropagation();

    this.isResizing = true;
    this.resizeCorner = corner;
    this.resizeStartX = e.clientX;
    this.resizeStartY = e.clientY;

    const rect = this.popup.getBoundingClientRect();
    this.popupStartWidth = rect.width;
    this.popupStartHeight = rect.height;
    this.popupStartLeft = rect.left;
    this.popupStartTop = rect.top;

    this.popup.classList.add('resizing');

    // Show overlay to prevent iframe from stealing mouse events
    if (this.resizeOverlay) {
      this.resizeOverlay.style.display = 'block';
    }

    document.addEventListener('mousemove', this.handleResizeMove);
    document.addEventListener('mouseup', this.handleResizeEnd);
  }

  /**
   * Handle resize move
   */
  handleResizeMove(e) {
    if (!this.isResizing) return;

    const dx = e.clientX - this.resizeStartX;
    const dy = e.clientY - this.resizeStartY;

    let newWidth = this.popupStartWidth;
    let newHeight = this.popupStartHeight;
    let newLeft = this.popupStartLeft;
    let newTop = this.popupStartTop;

    // Minimum dimensions
    const minWidth = 400;
    const minHeight = 300;

    switch (this.resizeCorner) {
      case 'br': // Bottom-right
        newWidth = Math.max(minWidth, this.popupStartWidth + dx);
        newHeight = Math.max(minHeight, this.popupStartHeight + dy);
        break;
      case 'bl': // Bottom-left
        newWidth = Math.max(minWidth, this.popupStartWidth - dx);
        newHeight = Math.max(minHeight, this.popupStartHeight + dy);
        newLeft = this.popupStartLeft + (this.popupStartWidth - newWidth);
        break;
      case 'tr': // Top-right
        newWidth = Math.max(minWidth, this.popupStartWidth + dx);
        newHeight = Math.max(minHeight, this.popupStartHeight - dy);
        newTop = this.popupStartTop + (this.popupStartHeight - newHeight);
        break;
      case 'tl': // Top-left
        newWidth = Math.max(minWidth, this.popupStartWidth - dx);
        newHeight = Math.max(minHeight, this.popupStartHeight - dy);
        newLeft = this.popupStartLeft + (this.popupStartWidth - newWidth);
        newTop = this.popupStartTop + (this.popupStartHeight - newHeight);
        break;
    }

    // Apply constraints
    newWidth = Math.min(newWidth, window.innerWidth * 0.95);
    newHeight = Math.min(newHeight, window.innerHeight * 0.95);

    this.popup.style.width = `${newWidth}px`;
    this.popup.style.height = `${newHeight}px`;

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

    // Hide overlay
    if (this.resizeOverlay) {
      this.resizeOverlay.style.display = 'none';
    }

    document.removeEventListener('mousemove', this.handleResizeMove);
    document.removeEventListener('mouseup', this.handleResizeEnd);
    this.saveWindowState();
  }

  /**
   * Handle messages from the terminal iframe (postMessage).
   * Responds to terminal-config-request by passing the JWT
   * so the iframe can authenticate against /api/system/info.
   */
  _handleIframeMessage(event) {
    if (!event.data || typeof event.data !== 'object') return;

    if (event.data.type === 'terminal-config-request') {
      const jwt = retrieveJWT();
      if (jwt) {
        event.source?.postMessage(
          { type: 'terminal-config', config: { jwt } },
          '*'
        );
      } else {
        event.source?.postMessage(
          { type: 'terminal-config-error', error: 'No JWT available' },
          '*'
        );
      }
    }
  }

  /**
   * Clean up the Terminal instance
   */
  destroy() {
    document.removeEventListener('keydown', this.handleKeyDown);
    window.removeEventListener('message', this._handleIframeMessage);

    this.overlay?.remove();
    this.popup?.remove();

    this.overlay = null;
    this.popup = null;
    this.iframe = null;
    this.fullscreenBtn = null;
    this.minimizeBtn = null;
    this.closeBtn = null;
    this._isInitialized = false;
    this.isVisible = false;
    this.isFullscreen = false;

    log(Subsystems.MANAGER, Status.INFO, '[Terminal] Destroyed');
  }
}

// Constants available to all modules
TerminalManager.STORAGE_KEYS = STORAGE_KEYS;
