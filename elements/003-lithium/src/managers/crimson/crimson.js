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
 *
 * @module managers/crimson
 */

import { processIcons } from '../../core/icons.js';
import { log, Subsystems, Status } from '../../core/log.js';
import './crimson.css';

// Singleton instance tracking
let crimsonInstance = null;

// Global keyboard shortcut handler
let globalKeyHandler = null;

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
  button.innerHTML = '<fa fa-robot></fa>';

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

    // Initialize
    this.init();
  }

  /**
   * Initialize the Crimson popup (create DOM elements)
   */
  init() {
    if (this.popup) return; // Already initialized

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
            <fa fa-robot></fa>
            <span>Chat with Crimson</span>
          </button>
          <button type="button" class="crimson-header-close" title="Close (ESC)">
            <fa fa-xmark></fa>
          </button>
        </div>
      </div>
      <div class="crimson-conversation">
        <div class="crimson-welcome">
          <div class="crimson-welcome-icon">
            <fa fa-robot></fa>
          </div>
          <div class="crimson-welcome-text">Hello, <span class="crimson-username">User</span>!</div>
          <div class="crimson-welcome-hint">How can I help you today?</div>
        </div>
      </div>
      <div class="crimson-input-area">
        <textarea class="crimson-input" placeholder="Type your message..." rows="1"></textarea>
        <button type="button" class="crimson-send-btn" title="Send message">
          <fa fa-paper-plane></fa>
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
    const header = this.popup.querySelector('.crimson-header');
    const closeBtn = this.popup.querySelector('.crimson-header-close');
    const resizeHandleBR = this.popup.querySelector('.crimson-resize-handle-br');
    const resizeHandleBL = this.popup.querySelector('.crimson-resize-handle-bl');
    const resizeHandleTR = this.popup.querySelector('.crimson-resize-handle-tr');
    const resizeHandleTL = this.popup.querySelector('.crimson-resize-handle-tl');

    // Set up event listeners
    header.addEventListener('mousedown', this.handleDragStart);
    closeBtn.addEventListener('click', () => this.hide());
    this.sendBtn.addEventListener('click', this.handleSend);
    this.input.addEventListener('keydown', this.handleInputKeydown);
    this.input.addEventListener('input', () => this.autoResizeInput());
    
    // Resize handles for all four corners
    resizeHandleBR?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'br'));
    resizeHandleBL?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'bl'));
    resizeHandleTR?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'tr'));
    resizeHandleTL?.addEventListener('mousedown', (e) => this.handleResizeStart(e, 'tl'));

    // Process icons
    processIcons(this.popup);

    // Position popup initially (centered)
    this.centerPopup();

    log(Subsystems.MANAGER, Status.INFO, '[Crimson] Initialized');
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
    if (!this.popup) this.init();

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
   * Handle send button click
   */
  handleSend() {
    const message = this.input.value.trim();
    if (!message) return;

    // Add user message
    this.addMessage('user', message);

    // Clear input
    this.input.value = '';
    this.autoResizeInput();

    // Simulate Crimson response (placeholder)
    this.simulateCrimsonResponse();
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
    const newHeight = Math.min(150, Math.max(44, this.input.scrollHeight));
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
      ? '<div class="crimson-message-avatar"><fa fa-robot></fa></div>'
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
      <div class="crimson-message-avatar"><fa fa-robot></fa></div>
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
   * Simulate Crimson response (placeholder for future AI integration)
   */
  simulateCrimsonResponse() {
    // Disable send button
    this.sendBtn.disabled = true;

    // Show typing indicator after a short delay
    setTimeout(() => {
      const indicator = this.addTypingIndicator();

      // Simulate response time (1-2 seconds)
      const responseTime = 1000 + Math.random() * 1000;

      setTimeout(() => {
        this.removeTypingIndicator();

        // Placeholder response
        const responses = [
          "I'm here to help! What would you like to know?",
          "That's an interesting question. Let me think about that...",
          "I can assist you with that. Could you provide more details?",
          "Thanks for your message! I'm still learning, but I'll do my best to help.",
          "I understand. How else can I assist you today?",
        ];
        const response = responses[Math.floor(Math.random() * responses.length)];

        this.addMessage('agent', response);

        // Re-enable send button
        this.sendBtn.disabled = false;
        this.input.focus();
      }, responseTime);
    }, 300);
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

    // Remove DOM elements
    this.overlay?.remove();
    this.popup?.remove();

    // Clear references
    this.overlay = null;
    this.popup = null;
    this.conversation = null;
    this.input = null;
    this.sendBtn = null;
    crimsonInstance = null;
    globalKeyHandler = null;

    log(Subsystems.MANAGER, Status.INFO, '[Crimson] Destroyed');
  }
}

// Initialize global shortcut on module load
initCrimsonShortcut();
